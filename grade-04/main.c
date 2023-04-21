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

char *sempahore_template_name = "/garden-semaphore-id-";
const char *shared_object = "/posix-shared-object";
int main_shmid;

int columns;
int rows;
const int MAX_OF_SEMAPHORES = 1024;
const int EMPTY_PLOT_COEFFICIENT = 2;

void runFirstGardener(int columns, int rows, int workingTimeMilliseconds)
{
    int big_columns = columns / 2;
    int field_size = columns * rows;
    int *field;
    int shmid;

    sem_t *semaphores[MAX_OF_SEMAPHORES];
    for (int k = 0; k < columns * rows / 4; ++k)
    {
        char sem_name[200];
        sprintf(sem_name, "%s%d", sempahore_template_name, k);

        if ((semaphores[k] = sem_open(sem_name, 0)) == 0)
        {
            perror("sem_open: Can not open semaphore");
            exit(-1);
        };
    }

    if ((shmid = shm_open(shared_object, O_RDWR | O_NONBLOCK, 0666)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if ((field = mmap(0, field_size * sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)) < 0)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
        printf("First gardener open shared Memory\n");
        fflush(stdout);
    }

    int i = 0;
    int j = 0;
    while (i < rows)
    {
        while (j < columns)
        {
            sem_wait(semaphores[i / 2 * big_columns + j / 2]);
            printf("First gardener takes (row: %d, col: %d) plot\n", i, j);
            fflush(stdout);
            if (field[i * columns + j] == 0)
            {
                field[i * columns + j] = 1;
                usleep(workingTimeMilliseconds * 1000);
            }
            else
            {
                usleep(workingTimeMilliseconds / EMPTY_PLOT_COEFFICIENT * 1000);
            }
            sem_post(semaphores[i / 2 * big_columns + j / 2]);
            ++j;
        }

        ++i;
        --j;

        while (j >= 0)
        {
            sem_wait(semaphores[i / 2 * big_columns + j / 2]);
            printf("First gardener takes (row: %d, col: %d) plot\n", i, j);
            fflush(stdout);
            if (field[i * columns + j] == 0)
            {
                field[i * columns + j] = 1;
                usleep(workingTimeMilliseconds * 1000);
            }
            else
            {
                usleep(workingTimeMilliseconds / EMPTY_PLOT_COEFFICIENT * 1000);
            }
            sem_post(semaphores[i / 2 * big_columns + j / 2]);
            --j;
        }

        ++i;
        ++j;
    }
    printf("First gardener finish work\n");
    exit(0);
}

void runSecondGardener(int columns, int rows, int workingTimeMilliseconds)
{
    int big_columns = columns / 2;
    int field_size = columns * rows;
    int *field;
    int shmid;

    sem_t *semaphores[MAX_OF_SEMAPHORES];
    for (int k = 0; k < columns * rows / 4; ++k)
    {
        char sem_name[200];
        sprintf(sem_name, "%s%d", sempahore_template_name, k);

        if ((semaphores[k] = sem_open(sem_name, 0)) == 0)
        {
            perror("sem_open: Can not open semaphore");
            exit(-1);
        };
    }

    if ((shmid = shm_open(shared_object, O_RDWR | O_NONBLOCK, 0666)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if ((field = mmap(0, field_size * sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)) < 0)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
        printf("Second gardener open shared Memory\n");
        fflush(stdout);
    }

    int i = rows - 1;
    int j = columns - 1;
    while (j >= 0)
    {
        while (i >= 0)
        {
            sem_wait(semaphores[i / 2 * big_columns + j / 2]);
            printf("Second gardener takes (row: %d, col: %d) plot\n", i, j);
            fflush(stdout);
            if (field[i * columns + j] == 0)
            {
                field[i * columns + j] = 2;
                usleep(workingTimeMilliseconds * 1000);
            }
            else
            {
                usleep(workingTimeMilliseconds / EMPTY_PLOT_COEFFICIENT * 1000);
            }
            sem_post(semaphores[i / 2 * big_columns + j / 2]);
            --i;
        }

        --j;
        ++i;

        while (i < rows)
        {
            sem_wait(semaphores[i / 2 * big_columns + j / 2]);
            printf("Second gardener takes (row: %d, col: %d) plot\n", i, j);
            fflush(stdout);
            if (field[i * columns + j] == 0)
            {
                field[i * columns + j] = 2;
                usleep(workingTimeMilliseconds * 1000);
            }
            else
            {
                usleep(workingTimeMilliseconds / EMPTY_PLOT_COEFFICIENT * 1000);
            }
            sem_post(semaphores[i / 2 * big_columns + j / 2]);
            ++i;
        }

        --i;
        --j;
    }
    printf("Second gardener finish work\n");
    exit(0);
}

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

void createSemaphores()
{
    for (int k = 0; k < columns * rows / 4; ++k)
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
        if (val != 1)
        {
            printf("Ooops, one of semaphores can't set initial value to 1. Please, restart program\n");
            unlink_all_semaphores_with_close(columns, rows);
            exit(-1);
        }
    }
}

pid_t chpid1, chpid2;

void keyboard_interruption_handler(int num)
{
    kill(chpid1, SIGINT);
    kill(chpid2, SIGINT);
    printf("Closing resources...\n");
    shm_unlink(shared_object);
    unlink_all_semaphores_with_close(columns, rows);
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
    int first_gardener_working_time = atoi(argv[2]);
    int second_gardener_working_time = atoi(argv[3]);

    if (first_gardener_working_time < 1 || second_gardener_working_time < 1)
    {
        printf("Working time should be greater than 0\n");
        exit(-1);
    }

    // Будем перемещаться как в шарпах: array[i, j] = array[i * columns + j]
    // field[i] = 1, если участок обработал первый садовник
    // field[i] = 2, если участок обработал второй садовник
    // field[i] = 0, если участок ещё никто не обработал
    // field[i] = -1, если участок не доступен для обработки
    int *field;

    if ((main_shmid = shm_open(shared_object, O_CREAT | O_RDWR, 0666)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if (ftruncate(main_shmid, field_size * sizeof(int)) < 0)
        {
            perror("Can't rezie shm");
            exit(-1);
        }
        if ((field = mmap(0, field_size * sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, main_shmid, 0)) < 0)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
        printf("Open shared Memory\n");
    }

    initializeField(field);
    printField(field);

    // Создаем семафоры для каждого из блоков плана
    createSemaphores();
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

    unlink_all_semaphores_with_close(columns, rows);

    printField(field);
    fflush(stdout);

    shm_unlink(shared_object);
}