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

char *sempahore_template_name = "garden-semaphore-id-";
const char *shar_object = "posix-shar-object";
int main_shmid;
int field_columns;
int field_rows;

void runFirstGardener(int columns, int rows)
{
    int big_columns = columns / 2;
    int field_size = columns * rows;
    int *field;
    int shmid;

    sem_t *semaphores[100];
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

    if ((shmid = shm_open(shar_object, O_RDWR | O_NONBLOCK, 0666)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if ((field = mmap(0, field_size, PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)) < 0)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
        printf("First gardener open shared Memory\n");
    }

    int i = 0;
    int j = 0;
    while (i < rows)
    {
        while (j < columns)
        {
            sem_wait(semaphores[i / 2 * big_columns + j / 2]);
            printf("First gardener takes (row: %d, col: %d) plot\n", i, j);
            if (field[i * columns + j] == 0)
            {
                field[i * columns + j] = 1;
                usleep(100 * 1000);
            }
            else
            {
                usleep(100 * 1000);
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
            if (field[i * columns + j] == 0)
            {
                field[i * columns + j] = 1;
                usleep(50 * 1000);
            }
            else
            {
                usleep(100 * 1000);
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

void runSecondGardener(int columns, int rows)
{
    int big_columns = columns / 2;
    int field_size = columns * rows;
    int *field;
    int shmid;

    sem_t *semaphores[100];
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

    if ((shmid = shm_open(shar_object, O_RDWR | O_NONBLOCK, 0666)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if ((field = mmap(0, field_size, PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0)) < 0)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
        printf("Second gardener open shared Memory\n");
    }

    int i = rows - 1;
    int j = columns - 1;
    while (j >= 0)
    {
        while (i >= 0)
        {
            sem_wait(semaphores[i / 2 * big_columns + j / 2]);
            printf("Second gardener takes (row: %d, col: %d) plot\n", i, j);
            if (field[i * columns + j] == 0)
            {
                field[i * columns + j] = 2;
                usleep(100 * 1000);
            }
            else
            {
                usleep(100 * 1000);
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
            if (field[i * columns + j] == 0)
            {
                field[i * columns + j] = 2;
                usleep(0 * 1000);
            }
            else
            {
                usleep(0 * 1000);
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

pid_t chpid1, chpid2;

void keyboard_interruption_handler(int num)
{
    kill(chpid1, SIGINT);
    kill(chpid2, SIGINT);
    printf("Closing resources...\n");
    shm_unlink(shar_object);
    unlink_all_semaphores_with_close(field_columns, field_rows);
    exit(0);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    const int COLUMNS = 10;
    const int ROWS = 10;
    field_columns = 10;
    field_rows = 10;

    const int BIG_COLUMNS = 5;
    const int BIG_ROWS = 5;
    const int field_size = ROWS * COLUMNS;
    const int pID = 0;

    // Будем перемещаться как в шарпах: array[i, j] = array[i * COLUMNS + j]
    // field[i] = 1, если участок обработал первый садовник
    // field[i] = 2, если участок обработал второй садовник
    // field[i] = 0, если участок ещё никто не обработал
    // field[i] = -1, если участок не доступен для обработки
    int *field;

    if ((main_shmid = shm_open(shar_object, O_CREAT | O_RDWR, 0666)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if (ftruncate(main_shmid, field_size) < 0)
        {
            perror("Can't rezie shm");
            exit(-1);
        }
        if ((field = mmap(0, field_size, PROT_WRITE | PROT_READ, MAP_SHARED, main_shmid, 0)) < 0)
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
            field[i * COLUMNS + j] = 0;
        }
    }

    int percentage = 10 + random() % 20;
    int count_of_bad_plots = COLUMNS * ROWS * percentage / 100;
    for (int i = 0; i < count_of_bad_plots; ++i)
    {
        int row_index;
        int column_index;
        do
        {
            row_index = random() % ROWS;
            column_index = random() % COLUMNS;
        } while (field[row_index * COLUMNS + column_index] == -1);

        field[row_index * COLUMNS + column_index] = -1;
    }

    for (int i = 0; i < ROWS; ++i)
    {
        for (int j = 0; j < COLUMNS; ++j)
        {
            if (field[i * COLUMNS + j] < 0)
            {
                printf("x ");
            }
            else
            {
                printf("%d ", field[i * COLUMNS + j]);
            }
        }
        printf("\n");
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

    chpid1 = fork();
    if (chpid1 == 0)
    {
        runFirstGardener(COLUMNS, ROWS);
    }
    else if (chpid1 < 0)
    {
        perror("Can't run first gardener");
        exit(-1);
    }

    chpid2 = fork();
    if (chpid2 == 0)
    {
        runSecondGardener(COLUMNS, ROWS);
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

    unlink_all_semaphores_with_close(field_columns, field_rows);

    for (int i = 0; i < ROWS; ++i)
    {
        for (int j = 0; j < COLUMNS; ++j)
        {
            if (field[i * COLUMNS + j] < 0)
            {
                printf("x ");
            }
            else
            {
                printf("%d ", field[i * COLUMNS + j]);
            }
        }
        printf("\n");
    }

    shm_unlink(shar_object);
}