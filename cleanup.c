/*
cleanup.c
Program to unlink shared memory and semaphores.
Run as root or normal user depending on permission.
*/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "shm_common.h"

int main(void) 
{
    if (shm_unlink(SHM_NAME) == 0) 
    {
        printf("shm_unlink: %s\n", SHM_NAME);
    } 
    else 
    {
        perror("shm_unlink");
    }

    if (sem_unlink(SEM_EMPTY_NAME) == 0) 
    {
        printf("sem_unlink: %s\n", SEM_EMPTY_NAME);
    } 
    else 
    {
        perror("sem_unlink empty");
    }

    if (sem_unlink(SEM_FULL_NAME) == 0) 
    {
        printf("sem_unlink: %s\n", SEM_FULL_NAME);
    } 
    else 
    {
        perror("sem_unlink full");
    }

    if (sem_unlink(SEM_MUTEX_NAME) == 0) 
    {
        printf("sem_unlink: %s\n", SEM_MUTEX_NAME);
    } 
    else 
    {
        perror("sem_unlink mutex");
    }
    return 0;
}