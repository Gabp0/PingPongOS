#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_THREADS 3
#define SLEEP_TIME 3

char buffer[4098];
memset(buffer, 4098, sizeof(char));
sem_t buffer_s;

typedef struct barrier
{
    sem_t size_s;
    int size;
    int counter;
    sem_t barrier_s;

} barrier;

barrier b;

void barrier_init(barrier *b, int size)
{
    sem_init(&(b->size_s), 0, 1);
    sem_init(&(b->barrier_s), 0, 0);
    b->size = size;
    b->counter = 0;
}

void barrier_wait(barrier *b)
{
    sem_wait(&(b->size_s));
    b->counter++;
    printf("c: %d\n", b->counter);
    int is_last = (b->counter == b->size);
    printf("is _last %d\n", is_last);
    if (is_last)
    {
        printf("--barrier\n");
        fflush(stdout);
        b->counter = 0;
        for (size_t i = 0; i < b->size; i++) {
            sem_post(&(b->barrier_s));
        }
        sem_post(&(b->size_s));
    } else {
        sem_post(&(b->size_s));
        sem_wait(&(b->barrier_s));
    }
}

void *threadBody(void *id)
{
    int i = (long) id;
    
    while(1)
    {
        sleep(SLEEP_TIME*i + 1);
        printf("%d antes\n", i);
        fflush(stdout);
        barrier_wait(&b);
        printf("%d depois\n", i);
        sleep(5);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    pthread_t thread[NUM_THREADS];
    pthread_attr_t attr;
    long i, status;
    
    sem_init(buffer_s, 0, 1);

    // initialize semaphore to 1
    barrier_init(&b, NUM_THREADS);

    // define attribute for joinable threads
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // create threads
    for (i = 0; i < NUM_THREADS; i++)
    {
        status = pthread_create(&thread[i], &attr, threadBody, (void *)i);
        if (status)
        {
            perror("pthread_create");
            exit(1);
        }
    }

    // wait all threads to finish
    for (i = 0; i < NUM_THREADS; i++)
    {
        status = pthread_join(thread[i], NULL);
        if (status)
        {
            perror("pthread_join");
            exit(1);
        }
    }

    pthread_attr_destroy(&attr);
    pthread_exit(NULL);
}
