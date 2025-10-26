#define _POSIX_C_SOURCE 200809L   // MUST be first line before any includes

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // for usleep
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "shm_common.h"

static shm_region_t *shm = NULL;
static sem_t *sem_empty = NULL, *sem_full = NULL, *sem_mutex = NULL;
static int shm_fd = -1;
static volatile int keep_running = 1;

// Handle Ctrl-C
void handle_sigint(int _) 
{
    (void)_;  // mark parameter as unused
    keep_running = 0;
}

// Cleanup shared memory and semaphores
void cleanup_local(void) 
{
    if (sem_empty) sem_close(sem_empty);
    if (sem_full)  sem_close(sem_full);
    if (sem_mutex) sem_close(sem_mutex);
    if (shm) munmap(shm, sizeof(shm_region_t));
    if (shm_fd >= 0) close(shm_fd);
}

// Producer thread: produce sequential integers
void *producer_thread(void *arg) 
{
    int id = (int)(intptr_t)arg;
    int item = 0;
    srand((unsigned)time(NULL) ^ getpid());

    int items_to_produce = *((int *)arg); // number of items to produce

    while (keep_running && (items_to_produce == -1 || item < items_to_produce))
    {
        // produce an item (simple integer)
        item++;

        // wait for empty slot
        sem_wait(sem_empty);

        // enter critical section
        sem_wait(sem_mutex);

        // write into buffer
        shm->buffer[shm->in] = item;
        shm->in = (shm->in + 1) % BUFFER_SIZE;
        shm->produced_count++;

        printf("[producer %d] produced %d (produced_total=%d)\n", id, item, shm->produced_count);
        fflush(stdout);

        // leave critical section
        sem_post(sem_mutex);

        // signal there is a full slot
        sem_post(sem_full);

        // simulate production time
        usleep(100000 + (rand() % 200000)); // 0.1s - 0.3s
    }

    return NULL;
}

int main(int argc, char *argv[]) 
{
    (void)argv;  // suppress unused parameter warning
    signal(SIGINT, handle_sigint);
    atexit(cleanup_local);

    int items_to_produce = -1; // default: run until Ctrl-C
    if (argc > 1) {
        items_to_produce = atoi(argv[1]);
    }

    // Open or create shared memory
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) { perror("shm_open"); exit(EXIT_FAILURE); }

    if (ftruncate(shm_fd, sizeof(shm_region_t)) == -1) { perror("ftruncate"); exit(EXIT_FAILURE); }

    shm = mmap(NULL, sizeof(shm_region_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }

    // Open semaphores (create if missing)
    sem_empty = sem_open(SEM_EMPTY_NAME, O_CREAT, 0666, BUFFER_SIZE);
    if (sem_empty == SEM_FAILED) { perror("sem_open empty"); exit(EXIT_FAILURE); }

    sem_full = sem_open(SEM_FULL_NAME, O_CREAT, 0666, 0);
    if (sem_full == SEM_FAILED) { perror("sem_open full"); exit(EXIT_FAILURE); }

    sem_mutex = sem_open(SEM_MUTEX_NAME, O_CREAT, 0666, 1);
    if (sem_mutex == SEM_FAILED) { perror("sem_open mutex"); exit(EXIT_FAILURE); }

    // Initialize shared memory once (protected by sem_mutex)
    sem_wait(sem_mutex);
    if (shm->initialized != 1) 
    {
        shm->in = 0;
        shm->out = 0;
        shm->produced_count = 0;
        shm->consumed_count = 0;
        memset(shm->buffer, 0, sizeof(shm->buffer));
        shm->initialized = 1;
        printf("[producer] initialized shared region\n");
    }
    sem_post(sem_mutex);

    // Start one producer thread
    pthread_t tid;
    int thread_arg = items_to_produce; // pass the number of items
    if (pthread_create(&tid, NULL, producer_thread, &thread_arg) != 0) 
    {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Wait for thread to finish or Ctrl-C
    pthread_join(tid, NULL);

    printf("[producer] exiting\n");
    return 0;
}
