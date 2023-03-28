// GRR20203895 Gabriel de Oliveira Pontarolo
#include "queue.h"
#include <stdio.h>

// codigos de erro
#define SUCESS 0
#define EMPTY_QUEUE -1
#define NO_ELEM -2
#define BELONGS_TO_ANOTHER_QUEUE -3
#define NO_QUEUE -4
#define NOT_ON_QUEUE -5

int queue_size(queue_t *queue)
// retorna o tamanho da fila
{
    // se fila nao tem elementos, tamanho 0
    if (queue == NULL)
    {
        return 0;
    }

    queue_t *aux = queue;
    size_t size = 1;
    // percorre a fila
    while (aux->next != queue)
    {
        aux = aux->next;
        size++;
    }

    return size;
}

void queue_print(char *name, queue_t *queue, void print_elem(void *))
// imprime a fila de acordo com print_elem
{
    printf("%s: [", name);

    if (queue != NULL) // se nao vazia
    {
        queue_t *aux = queue;
        // percorre a fila e chama print_elem para cada nodo
        while (aux->next != queue)
        {
            print_elem(aux);
            printf(" ");
            aux = aux->next;
        }
        print_elem(aux);
    }

    printf("]\n");
}

int queue_append(queue_t **queue, queue_t *elem)
// insere elemento ao final da fila
{
    // se a fila nao existe, retorna erro
    if (queue == NULL)
    {
        return NO_QUEUE;
    }

    // se elemento nao existe, retorna erro
    if (elem == NULL)
    {
        return NO_ELEM;
    }

    // se o elemento pertence a outra fila, retorna erro
    if ((elem->next != NULL) || (elem->prev != NULL))
    {
        return BELONGS_TO_ANOTHER_QUEUE;
    }

    // se tamanho da fila == 0, insere elemento no inicio
    if (*queue == NULL)
    {
        *queue = elem;
        elem->prev = *queue;
        elem->next = *queue;
        return SUCESS;
    }

    // percorre a fila
    queue_t *aux = *queue;
    while (aux->next != *queue)
    {
        aux = aux->next;
    }

    // insere elemento no final da fila
    aux->next = elem;
    elem->next = *queue;
    (*queue)->prev = elem;
    elem->prev = aux;

    return SUCESS;
}

int queue_remove(queue_t **queue, queue_t *elem)
// remove elemento da fila
{
    // se a fila nao existe, retorna erro
    if (queue == NULL)
    {
        return NO_QUEUE;
    }

    // se elemento nao existe, retorna erro
    if (elem == NULL)
    {
        return NO_ELEM;
    }

    // se fila esta vazia, retorna erro
    if (*queue == NULL)
    {
        return EMPTY_QUEUE;
    }

    // se elemento nao pertence a fila, retorna erro
    if (elem->next == NULL || elem->prev == NULL)
    {
        return NOT_ON_QUEUE;
    }

    // se elemento pertence a outra fila, retorna erro
    queue_t *aux = elem->next;
    while ((aux != elem) && (aux != *queue))
    {
        aux = aux->next;
    }
    if ((aux == elem) && (aux != *queue))
    {
        return BELONGS_TO_ANOTHER_QUEUE;
    }

    // se elemento e o unico da fila
    if (elem->next == elem)
    {
        *queue = NULL;
        elem->next = NULL;
        elem->prev = NULL;
        return SUCESS;
    }

    // se elemento e o primeiro da fila, cabeca aponta para o proximo
    if (elem == *queue)
    {
        *queue = elem->next;
    }

    // remove elemento da fila
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
    elem->next = NULL;
    elem->prev = NULL;

    return SUCESS;
}