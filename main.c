#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>

int array[10][10];
sem_t *semaphores[25];

int main()
{
    const int COLUMNS = 10;
    const int ROWS = 10;
    const int field_size = ROWS * COLUMNS;
    const int pID = 0;
    const char *shar_object = "posix-shar-object";
    int shmid;
    // Будем перемещаться как в шарпах: array[i, j] = array[i * COLUMNS + j]
    int *field;

    if ((shmid = shm_open(shar_object, O_CREAT | O_RDWR, 0666)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if (ftruncate(shmid, field_size) < 0)
        {
            perror("Can't rezie shm");
            exit(-1);
        }
        if ((field = mmap(0, field_size, PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)) < 0)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
        printf("Open shared Memory\n");
    }

    for (int i = 0; i < ROWS; ++i)
    {
        for (int j = 0; j < COLUMNS; ++j)
        {
            field[i * COLUMNS + j] = i * COLUMNS + j;
        }
    }

    for (int i = 0; i < ROWS; ++i)
    {
        for (int j = 0; j < COLUMNS; ++j)
        {
            printf("%d ", field[i * COLUMNS + j]);
            fflush(stdout);
            sleep(10);
        }
    }
}