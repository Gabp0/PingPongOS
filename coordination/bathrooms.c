#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

#define NUM_THREADS 5
#define SLEEP_TIME 6
#define BATH_MAX 3

sem_t bath_s, ocpp_s, men_s, woman_s, count_s;
char ocpp = 'v';
int count = 0;

void work(long id, char g)
{
    printf("thread %ld do tipo %c esta trabalhando\n", id, g);
    //sleep(rand() % 12);
}

void use_bath_m(long id)
{
    printf("    thread %ld do tipo m quer usar o banheiro\n", id);

    sem_wait(&ocpp_s);
    if (ocpp == 'v')
    {
        ocpp = 'm';
        sem_wait(&woman_s);
    }
    sem_post(&ocpp_s);

    if (ocpp == 'w')
    {
        sem_wait(&men_s);
    }

    sem_wait(&bath_s);

    printf("        thread %ld do tipo m esta usando o banheiro\n", id);

    sem_wait(&count_s);
    count++;
    sem_post(&count_s);

    //sleep(rand() % SLEEP_TIME);

    printf("        thread %ld do tipo m terminou de usar o banheiro\n", id);

    sem_wait(&count_s);
    count--;
    sem_post(&count_s);

    sem_post(&bath_s);

    if (count == 0)
    {
        sem_wait(&ocpp_s);
        ocpp = 'v';
        sem_post(&ocpp_s);
        sem_post(&woman_s);
    }
}

void use_bath_w(long id)
{
    printf("    thread %ld do tipo w quer usar o banheiro\n", id);

    sem_wait(&ocpp_s);
    if (ocpp == 'v')
    {
        ocpp = 'w';
        sem_wait(&men_s);
    }
    sem_post(&ocpp_s);

    if (ocpp == 'h')
    {
        sem_wait(&woman_s);
    }

    sem_wait(&bath_s);

    printf("        thread %ld do tipo w esta usando o banheiro\n", id);

    sem_wait(&count_s);
    count++;
    sem_post(&count_s);

    //sleep(rand() % SLEEP_TIME);

    printf("        thread %ld do tipo w terminou de usar o banheiro\n", id);

    sem_wait(&count_s);
    count--;
    sem_post(&count_s);

    sem_post(&bath_s);

    if (count == 0)
    {
        sem_wait(&ocpp_s);
        ocpp = 'v';
        sem_post(&ocpp_s);
        sem_post(&men_s);
    }
}

void *man(void *id)
{
    long i = (long)id;
    while (1)
    {
        work(i, 'm');
        use_bath_m(i);
    }
}

void *woman(void *id)
{
    long i = (long)id;
    while (1)
    {
        work(i, 'w');
        use_bath_w(i);
    }
}

int main(int argc, char *argv[])
{
    pthread_t thread_m[NUM_THREADS];
    pthread_t thread_w[NUM_THREADS];
    pthread_attr_t attr;

    sem_init(&bath_s, 0, BATH_MAX);
    sem_init(&count_s, 0, 1);
    sem_init(&ocpp_s, 0, 1);
    sem_init(&woman_s, 0, 1);
    sem_init(&men_s, 0, 1);

    // define attribute for joinable threads
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // create threads
    for (long i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&thread_m[i], &attr, man, (void *)i);
        pthread_create(&thread_w[i], &attr, woman, (void *)i);
    }

    // wait all threads to finish
    for (long i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(thread_m[i], NULL);
        pthread_join(thread_w[i], NULL);
    }
}
