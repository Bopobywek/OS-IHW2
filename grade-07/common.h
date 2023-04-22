#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

struct GardenerTask
{
    int plot_i;
    int plot_j;
    int gardener_id;
    int working_time;
};

const char *main_waiter_sem = "/sem-main-waiter";
const char *pid_waiter_sem = "/sem-pid-waiter";
const char *finish_sem = "/finish-sem";
const int MAX_OF_SEMAPHORES = 1024;
const char *shared_object = "/posix-shared-object";
const char *exit_shared_object = "/exit-shared-object";
char *sempahore_template_name = "/garden-semaphore-id-";
const int EMPTY_PLOT_COEFFICIENT = 2;

sem_t *createOrOpenSemaphore(const char *sem_name)
{
    sem_t *sem;

    if ((sem = sem_open(sem_name, O_CREAT | O_EXCL, 0666, 0)) == 0)
    {
        if ((sem = sem_open(sem_name, 0)) == 0)
        {
            printf("sem_open: Can not open or create semaphore %s\n", sem_name);
            exit(-1);
        }
    }

    int val;
    sem_getvalue(sem, &val);
    printf("Value of semaphore %s is %d\n", sem_name, val);
    return sem;
}

int *createOrOpenExitSharedMemory()
{
    int exit_shmid;
    int *exit_data;
    if ((exit_shmid = shm_open(exit_shared_object, O_CREAT | O_RDWR, 0666)) < 0)
    {
        perror("Can't connect to shared memory");
        exit(-1);
    }
    else
    {
        if (ftruncate(exit_shmid, 3 * sizeof(int)) < 0)
        {
            perror("Can't rezie shm");
            exit(-1);
        }
        if ((exit_data = mmap(0, 3 * sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, exit_shmid, 0)) < 0)
        {
            printf("Can\'t connect to shared memory\n");
            exit(-1);
        };
        exit_data[0] = 0;
        exit_data[1] = 0;
        exit_data[2] = 0;
    }
    return exit_data;
}

void getSemaphores(sem_t **semaphores, int columns, int rows)
{
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
}

void handleGardenPlot(sem_t **semaphores, int *field, int big_columns, struct GardenerTask task)
{
    int columns = big_columns * 2;
    sem_wait(semaphores[task.plot_i / 2 * big_columns + task.plot_j / 2]);
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
    sem_post(semaphores[task.plot_i / 2 * big_columns + task.plot_j / 2]);
}