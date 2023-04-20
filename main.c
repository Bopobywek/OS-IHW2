#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>

int main()
{
    const int COLUMNS = 10;
    const int ROWS = 10;
    const int BIG_COLUMNS = 5;
    const int BIG_ROWS = 5;
    const int field_size = ROWS * COLUMNS;
    const int pID = 0;
    const char *shar_object = "posix-shar-object";
    char *sempahore_template_name = "garden-semaphore-id-";
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

    for (int k = 0; k < COLUMNS * ROWS / 4; ++k)
    {
        char sem_name[200];
        sprintf(sem_name, "%s%d", sempahore_template_name, k);

        sem_t *sem;
        if ((sem = sem_open(sem_name, O_CREAT, 0666, 1)) == 0)
        {
            perror("sem_open: Can not create admin semaphore");
            exit(-1);
        };

        int val;
        sem_getvalue(sem, &val);
        printf("Val_%d: %d\n", k, val);
    }

    // first worker

    pid_t chpid1 = fork();
    if (chpid1 == 0)
    {
        sem_t *semaphores[25];
        for (int k = 0; k < COLUMNS * ROWS / 4; ++k)
        {
            char sem_name[200];
            sprintf(sem_name, "%s%d", sempahore_template_name, k);

            if ((semaphores[k] = sem_open(sem_name, 0)) == 0)
            {
                perror("sem_open: Can not open semaphore");
                exit(-1);
            };
        }

        int i = 0;
        int j = 0;
        while (i < ROWS)
        {
            while (j < COLUMNS)
            {
                sem_wait(semaphores[i / 2 * BIG_COLUMNS + j / 2]);
                printf("First gardener takes %d plot\n", i * COLUMNS + j);
                sem_post(semaphores[i / 2 * BIG_COLUMNS + j / 2]);
                ++j;
            }

            ++i;
            --j;

            while (j >= 0)
            {
                sem_wait(semaphores[i / 2 * BIG_COLUMNS + j / 2]);
                printf("First gardener takes %d plot\n", i * COLUMNS + j);
                sem_post(semaphores[i / 2 * BIG_COLUMNS + j / 2]);
                --j;
            }

            ++i;
            ++j;
        }

        exit(0);
    }
    pid_t chpid2 = fork();

    // second worker
    if (chpid2 == 0)
    {
        sem_t *semaphores[25];
        for (int k = 0; k < COLUMNS * ROWS / 4; ++k)
        {
            char sem_name[200];
            sprintf(sem_name, "%s%d", sempahore_template_name, k);

            if ((semaphores[k] = sem_open(sem_name, 0)) == SEM_FAILED)
            {
                perror("sem_open: Can not open semaphore");
                exit(-1);
            };
        }

        int i = ROWS - 1;
        int j = COLUMNS - 1;
        while (j >= 0)
        {
            while (i >= 0)
            {
                int val;
                sem_wait(semaphores[i / 2 * BIG_COLUMNS + j / 2]);
                printf("Second gardener takes %d plot\n", i * COLUMNS + j);
                sem_post(semaphores[i / 2 * BIG_COLUMNS + j / 2]);
                --i;
            }

            --j;
            ++i;

            while (i < ROWS)
            {
                sem_wait(semaphores[i / 2 * BIG_COLUMNS + j / 2]);
                printf("Second gardener takes %d plot\n", i * COLUMNS + j);
                sem_post(semaphores[i / 2 * BIG_COLUMNS + j / 2]);
                ++i;
            }

            --i;
            --j;
        }

        exit(0);
    }

    int status = 0;
    waitpid(chpid1, &status, 0);
    waitpid(chpid2, &status, 0);

    for (int k = 0; k < COLUMNS * ROWS / 4; ++k)
    {
        char sem_name[200];
        sprintf(sem_name, "%s%d", sempahore_template_name, k);

        sem_close(sem_open(sem_name, 0));
        if (sem_unlink(sem_name) < 0)
        {
            perror("Can not unlink semaphore");
            exit(-1);
        };
    }
}