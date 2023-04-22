#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>

int *exit_data;
int handler_pid;

void keyboard_interruption_handler(int num)
{
    if (exit_data[0] != 1)
    {
        kill(handler_pid, SIGINT);
    }
    if (exit_data[0] == 1 && exit_data[2] == 1)
    {
        shm_unlink(exit_shared_object);
    }
    exit_data[1] = 1;
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Invalid count of arguments. Expected 1: working_time_milliseconds\n");
        exit(-1);
    }

    exit_data = createOrOpenExitSharedMemory();

    int workingTimeMilliseconds = atoi(argv[1]);
    sem_t *init_waiter = createOrOpenSemaphore(main_waiter_sem);
    sem_t *pid_waiter = createOrOpenSemaphore(pid_waiter_sem);
    sem_t *finish_waiter = createOrOpenSemaphore(finish_sem);

    printf("Gardener 1 before wait\n");
    sem_wait(init_waiter);
    printf("Gardener 1 after wait\n");
    signal(SIGINT, keyboard_interruption_handler);
    int *field;
    int *meta_data;
    int shmid;

    int columns;
    int rows;

    if ((shmid = shm_open(shared_object, O_RDWR | O_NONBLOCK, 0666)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }

    if ((meta_data = mmap(0, 5 * sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)) < 0)
    {
        printf("Can\'t connect to shared memory for receive meta_data\n");
        exit(-1);
    };

    handler_pid = meta_data[2];
    columns = meta_data[0];
    rows = meta_data[1];
    int big_columns = columns / 2;
    int field_size = columns * rows;

    if ((field = mmap(0, (field_size + 5) * sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)) < 0)
    {
        printf("Can\'t connect to shared memory with field\n");
        exit(-1);
    };

    field = field + 5;
    printf("Gardener 1 open shared memory with field\n");

    meta_data[3] = getpid();

    sem_post(pid_waiter);

    sem_t *semaphores[MAX_OF_SEMAPHORES];
    getSemaphores(semaphores, columns, rows);
    fflush(stdout);

    int i = 0;
    int j = 0;
    struct GardenerTask task;
    task.gardener_id = 1;
    task.working_time = workingTimeMilliseconds;
    while (i < rows)
    {
        while (j < columns)
        {
            task.plot_i = i;
            task.plot_j = j;
            handleGardenPlot(semaphores, field, big_columns, task);
            ++j;
        }

        ++i;
        --j;

        while (j >= 0)
        {
            task.plot_i = i;
            task.plot_j = j;
            handleGardenPlot(semaphores, field, big_columns, task);
            --j;
        }

        ++i;
        ++j;
    }
    printf("Gardener 1 finish work\n");
    sem_post(finish_waiter);
    return 0;
}