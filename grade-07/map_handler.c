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

void unlink_all_semaphores_with_close(int columns, int rows)
{
    for (int k = 0; k < columns * rows / 4; ++k)
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

    sem_unlink(finish_sem);
    sem_unlink(pid_waiter_sem);
    sem_unlink(main_waiter_sem);
}

void printField(int *field, int columns, int rows)
{
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < columns; ++j)
        {
            if (field[i * columns + j] < 0)
            {
                printf("X ");
            }
            else
            {
                printf("%d ", field[i * columns + j]);
            }
        }
        printf("\n");
    }
}

void initializeField(int *field, int columns, int rows)
{
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < columns; ++j)
        {
            field[i * columns + j] = 0;
        }
    }

    int percentage = 10 + random() % 20;
    int count_of_bad_plots = columns * rows * percentage / 100;
    for (int i = 0; i < count_of_bad_plots; ++i)
    {
        int row_index;
        int column_index;
        do
        {
            row_index = random() % rows;
            column_index = random() % columns;
        } while (field[row_index * columns + column_index] == -1);

        field[row_index * columns + column_index] = -1;
    }
}

void createSemaphores(int columns, int rows)
{
    for (int k = 0; k < columns * rows / 4; ++k)
    {
        char sem_name[200];
        sprintf(sem_name, "%s%d", sempahore_template_name, k);

        sem_t *sem;
        if ((sem = sem_open(sem_name, O_CREAT, 0666, 1)) == 0)
        {
            perror("sem_open: Can not create semaphore");
            exit(-1);
        };

        int val;
        sem_getvalue(sem, &val);
        if (val != 1)
        {
            printf("Ooops, one of semaphores can't set initial value to 1. Please, restart program\n");
            unlink_all_semaphores_with_close(columns, rows);
            shm_unlink(shared_object);
            exit(-1);
        }
    }
}

int global_columns;
int global_rows;
int *exit_data;
int pid1 = -1;
int pid2 = -1;
void keyboard_interruption_handler(int num)
{
    printf("Closing resources...\n");
    exit_data[0] = 1;
    shm_unlink(shared_object);
    unlink_all_semaphores_with_close(global_columns, global_rows);
    if (pid1 > 0 && exit_data[1] == 0)
    {
        kill(pid1, SIGINT);
    }
    if (pid2 > 0 && exit_data[2] == 0)
    {
        kill(pid2, SIGINT);
    }

    if (exit_data[1] == 1 && exit_data[2] == 1)
    {
        shm_unlink(exit_shared_object);
    }

    exit(0);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (argc != 2)
    {
        printf("Invalid count of arguments. Expected 1: square_side_size\n");
        exit(-1);
    }

    int square_side_size = atoi(argv[1]);
    if (square_side_size * square_side_size > MAX_OF_SEMAPHORES)
    {
        printf("Too large square_side_size\n");
        exit(-1);
    }
    else if (square_side_size < 2)
    {
        printf("Too small square_side_size\n");
        exit(-1);
    }

    int rows = 2 * square_side_size;
    int columns = 2 * square_side_size;
    global_columns = global_rows = 2 * square_side_size;

    int field_size = rows * columns;

    sem_t *waiter_sem = createOrOpenSemaphore(main_waiter_sem);
    printf("map handler gets main waiter sem\n");

    sem_t *pid_waiter = createOrOpenSemaphore(pid_waiter_sem);
    printf("map handler gets pid waiter sem\n");

    sem_t *finish_waiter = createOrOpenSemaphore(finish_sem);

    // Будем перемещаться как в шарпах: array[i, j] = array[i * columns + j]
    // field[i] = 1, если участок обработал первый садовник
    // field[i] = 2, если участок обработал второй садовник
    // field[i] = 0, если участок ещё никто не обработал
    // field[i] = -1, если участок не доступен для обработки
    int *field;
    int *meta_data;
    int *shm_data;

    int main_shmid;

    exit_data = createOrOpenExitSharedMemory();

    signal(SIGINT, keyboard_interruption_handler);

    if ((main_shmid = shm_open(shared_object, O_CREAT | O_RDWR, 0666)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        // + 5 == cols, rows, map_handler_pid, first_gardener_pid, second_gardener_pid
        if (ftruncate(main_shmid, (field_size + 5) * sizeof(int)) < 0)
        {
            perror("Can't rezie shm");
            exit(-1);
        }
        if ((shm_data = mmap(0, (field_size + 5) * sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, main_shmid, 0)) < 0)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
        printf("Open shared Memory\n");
    }

    field = shm_data + 5;
    meta_data = shm_data;

    meta_data[0] = columns;
    meta_data[1] = rows;
    meta_data[2] = getpid();

    initializeField(field, columns, rows);
    printField(field, columns, rows);

    // Создаем семафоры для каждого из блоков плана
    createSemaphores(columns, rows);
    fflush(stdout);

    sem_post(waiter_sem);
    sem_post(waiter_sem);

    sem_wait(pid_waiter);
    sem_wait(pid_waiter);

    pid1 = meta_data[3];
    pid2 = meta_data[4];

    int first_gardener_pid = meta_data[3];
    int second_gardener_pid = meta_data[4];

    sem_wait(finish_waiter);
    sem_wait(finish_waiter);

    unlink_all_semaphores_with_close(columns, rows);

    printf("\nResult:\n");
    printField(field, columns, rows);
    fflush(stdout);

    shm_unlink(shared_object);
    shm_unlink(exit_shared_object);
    printf("Unlink shared memory and semaphores\n");
    return 0;
}