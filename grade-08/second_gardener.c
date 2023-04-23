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
int exit_shmid;
int special_sem_id;

void keyboard_interruption_handler(int num)
{
    if (exit_data[0] != 1)
    {
        kill(handler_pid, SIGINT);
    }

    struct shmid_ds info;
    shmctl(exit_shmid, IPC_STAT, &info);
    if (info.shm_nattch == 1)
    {
        deleteSharedMemory(exit_shmid);
        deleteSemaphores(special_sem_id);
    }

    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Invalid count of arguments. Expected 1: working_time_milliseconds\n");
        exit(-1);
    }

    exit_data = createOrOpenExitSharedMemory(&exit_shmid);

    int workingTimeMilliseconds = atoi(argv[1]);

    int special_sem_key;
    if ((special_sem_key = ftok(special_sem_key_file_name, 1)) < 0)
    {
        printf("Can't generate special sem key\n");
        exit(-1);
    }

    special_sem_id = createOrOpenSemaphore(special_sem_key, 3, 0);
    struct sembuf waiter_buf;
    waiter_buf.sem_num = 0;
    waiter_buf.sem_op = -1;
    waiter_buf.sem_flg = 0;

    printf("Gardener 2 before wait\n");
    semop(special_sem_id, &waiter_buf, 1);
    printf("Gardener 2 after wait\n");
    signal(SIGINT, keyboard_interruption_handler);
    int *field;
    int *meta_data;
    int shmid;

    int columns;
    int rows;

    key_t shm_key;
    if ((shm_key = ftok(key_file_name, 2)) < 0)
    {
        printf("Can't generate shared memory key\n");
        exit(-1);
    }

    if ((shmid = shmget(shm_key, 5 * sizeof(int), 0)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if ((meta_data = shmat(shmid, 0, 0)) == NULL)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
    }

    handler_pid = meta_data[2];
    columns = meta_data[0];
    rows = meta_data[1];
    int big_columns = columns / 2;
    int field_size = columns * rows;

    if ((shmid = shmget(shm_key, (field_size + 5) * sizeof(int), 0)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if ((field = shmat(shmid, 0, 0)) == NULL)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
    }

    field = field + 5;
    printf("Gardener 2 open shared memory with field\n");

    meta_data[4] = getpid();

    struct sembuf pid_waiter_buf;
    pid_waiter_buf.sem_num = 1;
    pid_waiter_buf.sem_flg = 0;
    pid_waiter_buf.sem_op = 1;

    semop(special_sem_id, &pid_waiter_buf, 1);

    int sem_id;
    getSemaphores(columns, rows, &sem_id);
    fflush(stdout);

    int i = rows - 1;
    int j = columns - 1;
    struct GardenerTask task;
    task.gardener_id = 2;
    task.working_time = workingTimeMilliseconds;
    while (j >= 0)
    {
        while (i >= 0)
        {
            task.plot_i = i;
            task.plot_j = j;
            handleGardenPlot(sem_id, field, big_columns, task);
            --i;
        }

        --j;
        ++i;

        while (i < rows)
        {
            task.plot_i = i;
            task.plot_j = j;
            handleGardenPlot(sem_id, field, big_columns, task);
            ++i;
        }

        --i;
        --j;
    }
    printf("Gardener 2 finish work\n");

    struct sembuf finish_waiter_buf;
    finish_waiter_buf.sem_num = 2;
    finish_waiter_buf.sem_flg = 0;
    finish_waiter_buf.sem_op = 1;

    semop(special_sem_id, &finish_waiter_buf, 1);
    return 0;
}