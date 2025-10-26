#ifndef SHM_COMMON_H
#define SHM_COMMON_H

#include <semaphore.h>

#define SHM_NAME "/pc_shm_mitchel_2025"
#define SEM_EMPTY_NAME "/pc_sem_empty_mitchel_2025"
#define SEM_FULL_NAME  "/pc_sem_full_mitchel_2025"
#define SEM_MUTEX_NAME "/pc_sem_mutex_mitchel_2025"

#define BUFFER_SIZE 2    //table can hold two items

typedef struct 
{
    int buffer[BUFFER_SIZE];
    int in;    //next write index
    int out;   //next read index
    int produced_count;
    int consumed_count;
    int initialized; //0 => not init; 1 => inited
} shm_region_t;

#endif //SHM_COMMON_H
