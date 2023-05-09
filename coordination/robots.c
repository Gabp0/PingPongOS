#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_NUM 3
#define SLEEP_TIME 1

sem_t bart_s, lisa_s, maggie_s;

void *bart(void *id)
{
    while (1)
    {
        sem_wait(&bart_s);
        printf("Bart\n");
        fflush(stdout);
        sem_post(&lisa_s);
        sleep(SLEEP_TIME);
    }
}

void *lisa(void *id)
{
    while (1)
    {
        sem_wait(&lisa_s);
        printf("    Lisa\n");
        fflush(stdout);
        sem_post(&maggie_s);
        sem_wait(&lisa_s);
        printf("    Lisa\n");
        fflush(stdout);
        sem_post(&bart_s);

        sleep(SLEEP_TIME);
    }
}

void *maggie(void *id)
{
    while (1)
    {
        sem_wait(&maggie_s);
        printf("        Maggie\n");
        fflush(stdout);
        sem_post(&lisa_s);
        // sem_wait(&maggie_s);
        sleep(SLEEP_TIME);
    }
}

int main(int argc, char const *argv[])
{
    pthread_t threads[THREAD_NUM];

    sem_init(&bart_s, 0, 1);
    sem_init(&lisa_s, 0, 0);
    sem_init(&maggie_s, 0, 0);

    pthread_create(&threads[0], NULL, bart, NULL);
    pthread_create(&threads[1], NULL, lisa, NULL);
    pthread_create(&threads[2], NULL, maggie, NULL);

    for (int i = 0; i < THREAD_NUM; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
