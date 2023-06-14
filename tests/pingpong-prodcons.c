// Teste de produtor/consumidor com semaforos
#include <stdio.h>
#include <stdlib.h>
#include "../lib/ppos.h"
#include "../lib/queue.h"

typedef struct item_t
{
    struct item_t *prev, *next;
    int item;
} item_t;

#define PRODS 3
#define CONS 2
#define BUFSIZE 5

task_t prod, cons;
semaphore_t s_vaga, s_item, s_buffer;
item_t *buffer;

void *productor(void *arg)
{
    long int id = (long int)arg;

    while (1)
    {
        task_sleep(1000);
        int item = rand() % 100;

        sem_down(&s_vaga);

        sem_down(&s_buffer);
        printf("p%ld produziu %d\n", id, item);
        item_t *item_elem = malloc(sizeof(item_t));
        item_elem->item = item;
        queue_append((queue_t **)&buffer, (queue_t *)item_elem);
        sem_up(&s_buffer);

        sem_up(&s_item);
    }

    return NULL;
}

void *consumer(void *arg)
{
    long int id = (long int)arg;

    while (1)
    {
        sem_down(&s_item);

        sem_down(&s_buffer);
        item_t *item_elem = buffer;
        queue_remove((queue_t **)&buffer, (queue_t *)buffer);
        sem_up(&s_buffer);

        sem_up(&s_vaga);

        printf("    c%ld consumiu %d\n", id, item_elem->item);
        free(item_elem);
        task_sleep(1000);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    printf("main: inicio\n");
    ppos_init();

    sem_init(&s_vaga, BUFSIZE);
    sem_init(&s_item, 0);
    sem_init(&s_buffer, 1);

    for (long int i = 0; i < PRODS; i++)
    {
        task_init(&prod, (void *)productor, (void *)i);
    }

    for (long int i = 0; i < CONS; i++)
    {
        task_init(&cons, (void *)consumer, (void *)i);
    }

    printf("main: fim\n");
    task_exit(0);
}
