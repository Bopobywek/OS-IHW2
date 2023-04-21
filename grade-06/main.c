#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>

const char *key_file_name = "system_V_key";
key_t sem_key;
key_t shm_key;
int main_shmid;
int sem_main_id;

int columns;
int rows;
const int MAX_OF_SEMAPHORES = 1024;
const int EMPTY_PLOT_COEFFICIENT = 2;

struct GardenerTask
{
    int plot_i;
    int plot_j;
    int gardener_id;
    int working_time;
};

void deleteSharedMemory()
{
    if (shmctl(main_shmid, IPC_RMID, NULL) < 0)
    {
        printf("Can't delete shm\n");
        exit(-1);
    }
}

void getSemaphores(int columns, int rows, int *sem_id)
{
    if ((*sem_id = semget(sem_key, columns * rows / 4, 0)) < 0)
    {
        printf("Can't open semaphores\n");
        deleteSharedMemory();
        exit(-1);
    }
}

void getField(int **field, int field_size, int *shmid)
{
    if ((*shmid = shmget(shm_key, field_size * sizeof(int), 0)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if ((*field = shmat(*shmid, 0, 0)) == NULL)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
    }
}

void handleGardenPlot(int sem_id, int *field, int big_columns, struct GardenerTask task)
{
    struct sembuf buf;
    buf.sem_num = task.plot_i / 2 * big_columns + task.plot_j / 2;

    buf.sem_op = -1;
    buf.sem_flg = 0;
    semop(sem_id, &buf, 1);
    printf("Gardener %d takes (row: %d, col: %d) plot\n", task.gardener_id, task.plot_i, task.plot_j);
    fflush(stdout);
    if (field[task.plot_i * columns + task.plot_j] == 0)
    {
        field[task.plot_i * columns + task.plot_j] = task.gardener_id;
        usleep(task.working_time * 1000);
    }
    else
    {
        usleep(task.working_time / EMPTY_PLOT_COEFFICIENT * 1000);
    }

    buf.sem_op = 1;
    buf.sem_flg = 0;
    semop(sem_id, &buf, 1);
}

void runFirstGardener(int columns, int rows, int workingTimeMilliseconds)
{
    int big_columns = columns / 2;
    int field_size = columns * rows;
    int shmid;
    int sem_id;

    getSemaphores(columns, rows, &sem_id);
    printf("Gardener 1 open shared memory with semaphores\n");
    fflush(stdout);

    int *field;
    getField(&field, field_size, &shmid);
    printf("Gardener 1 open shared memory with field\n");
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
            handleGardenPlot(sem_id, field, big_columns, task);
            ++j;
        }

        ++i;
        --j;

        while (j >= 0)
        {
            task.plot_i = i;
            task.plot_j = j;
            handleGardenPlot(sem_id, field, big_columns, task);
            --j;
        }

        ++i;
        ++j;
    }
    printf("Gardener 1 finish work\n");
    exit(0);
}

void runSecondGardener(int columns, int rows, int workingTimeMilliseconds)
{
    int big_columns = columns / 2;
    int field_size = columns * rows;
    int shmid;
    int sem_id;

    getSemaphores(columns, rows, &sem_id);
    printf("Gardener 2 open shared memory with semaphores\n");
    fflush(stdout);

    int *field;
    getField(&field, field_size, &shmid);
    printf("Gardener 2 open shared memory with field\n");
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
    exit(0);
}

void printField(int *field)
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

void initializeField(int *field)
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

void initializeSemaphores()
{
    for (int k = 0; k < columns * rows / 4; ++k)
    {
        semctl(sem_main_id, k, SETVAL, 1);

        if (semctl(sem_main_id, k, GETVAL) != 1)
        {
            printf("Can't set initial value of semaphore to 1\n");
            exit(-1);
        }
    }
}

void deleteSemaphores()
{

    if (semctl(sem_main_id, 0, IPC_RMID) < 0)
    {
        printf("Can't delete semaphores\n");
        exit(-1);
    }
}

pid_t chpid1, chpid2;

void keyboard_interruption_handler(int num)
{
    kill(chpid1, SIGINT);
    kill(chpid2, SIGINT);
    printf("Closing resources...\n");
    deleteSharedMemory();
    deleteSemaphores();
    exit(0);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (argc != 4)
    {
        printf("Invalid count of arguments. "
               "Expected 3 arguments: square_side_size, first_gardener_speed, second_gardener_speed\n");
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

    rows = columns = 2 * square_side_size;
    int field_size = rows * columns;
    int sem_count = field_size / 4;
    int first_gardener_working_time = atoi(argv[2]);
    int second_gardener_working_time = atoi(argv[3]);

    if (first_gardener_working_time < 1 || second_gardener_working_time < 1)
    {
        printf("Working time should be greater than 0\n");
        exit(-1);
    }

    if ((shm_key = ftok(key_file_name, 0)) < 0)
    {
        printf("Can't generate shared memory key\n");
        exit(-1);
    }

    if ((sem_key = ftok(key_file_name, 1)) < 0)
    {
        printf("Can't generate sem key\n");
        exit(-1);
    }

    // Будем перемещаться как в шарпах: array[i, j] = array[i * columns + j]
    // field[i] = 1, если участок обработал первый садовник
    // field[i] = 2, если участок обработал второй садовник
    // field[i] = 0, если участок ещё никто не обработал
    // field[i] = -1, если участок не доступен для обработки
    int *field;

    if ((main_shmid = shmget(shm_key, field_size * sizeof(int), 0666 | IPC_CREAT)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if ((field = shmat(main_shmid, 0, 0)) == NULL)
        {
            printf("Can\'t get shared memory\n");
            exit(-1);
        };
        printf("Open shared Memory for field\n");
    }

    if ((sem_main_id = semget(sem_key, sem_count, 0666 | IPC_CREAT)) < 0)
    {
        printf("Can't create semaphores\n");
        deleteSharedMemory();
        exit(-1);
    }

    initializeField(field);
    printField(field);

    // Создаем семафоры для каждого из блоков плана
    initializeSemaphores();
    fflush(stdout);

    chpid1 = fork();
    if (chpid1 == 0)
    {
        runFirstGardener(columns, rows, first_gardener_working_time);
    }
    else if (chpid1 < 0)
    {
        perror("Can't run first gardener");
        exit(-1);
    }

    chpid2 = fork();
    if (chpid2 == 0)
    {
        runSecondGardener(columns, rows, second_gardener_working_time);
    }
    else if (chpid2 < 0)
    {
        perror("Can't run second gardener");
        exit(-1);
    }

    signal(SIGINT, keyboard_interruption_handler);

    int status = 0;
    waitpid(chpid1, &status, 0);
    waitpid(chpid2, &status, 0);

    printField(field);
    fflush(stdout);

    deleteSharedMemory();
    deleteSemaphores();
}