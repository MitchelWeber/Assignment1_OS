#define _POSIX_C_SOURCE 200809L   // must be first

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
    (void)_;
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

// Consumer thread
void *consumer_thread(void *arg) 
{
    int id = (int)(intptr_t)arg;
    srand((unsigned)time(NULL) ^ getpid());

    int items_to_consume = *((int *)arg); // number to consume (-1 means infinite)
    int consumed = 0;

    while (keep_running && (items_to_consume == -1 || consumed < items_to_consume))
    {
        // wait for full slot
        if (sem_wait(sem_full) == -1)
            break;

        // enter critical section
        sem_wait(sem_mutex);

        // read from buffer
        int item = shm->buffer[shm->out];
        shm->out = (shm->out + 1) % BUFFER_SIZE;
        shm->consumed_count++;
        consumed++;

        printf("[consumer %d] consumed %d (consumed_total=%d)\n", id, item, shm->consumed_count);
        fflush(stdout);

        // leave critical section
        sem_post(sem_mutex);

        // signal there is an empty slot
        sem_post(sem_empty);

        // simulate processing time
        usleep(150000 + (rand() % 200000)); // 0.15s - 0.35s
    }

    return NULL;
}

int main(int argc, char *argv[]) 
{
    (void)argv;
    signal(SIGINT, handle_sigint);
    atexit(cleanup_local);

    int items_to_consume = -1; // default infinite
    if (argc > 1)
        items_to_consume = atoi(argv[1]);

    // open existing shared memory
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd < 0) { perror("shm_open"); exit(EXIT_FAILURE); }

    shm = mmap(NULL, sizeof(shm_region_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }

    // open semaphores
    sem_empty = sem_open(SEM_EMPTY_NAME, 0);
    if (sem_empty == SEM_FAILED) { perror("sem_open empty"); exit(EXIT_FAILURE); }

    sem_full = sem_open(SEM_FULL_NAME, 0);
    if (sem_full == SEM_FAILED) { perror("sem_open full"); exit(EXIT_FAILURE); }

    sem_mutex = sem_open(SEM_MUTEX_NAME, 0);
    if (sem_mutex == SEM_FAILED) { perror("sem_open mutex"); exit(EXIT_FAILURE); }

    // start consumer thread
    pthread_t tid;
    int thread_arg = items_to_consume;
    if (pthread_create(&tid, NULL, consumer_thread, &thread_arg) != 0)
    {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // wait for it to finish or Ctrl-C
    pthread_join(tid, NULL);

    printf("[consumer] exiting\n");
    return 0;
}
