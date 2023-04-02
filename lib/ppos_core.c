#include "ppos.h"
#include "ppos_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

// #define DEBUG

#define STACKSIZE 64 * 1024 // tamanho de pilha das threads

// status das tasks atuais
#define READY 0
#define RUNNING 1
#define SUSPENDED 2
#define TERMINATED 3

// error codes
#define SUCESS 0
#define STACK_CREATION_ERROR -1
#define NULL_PTR_ERROR -2

task_t main_task;
unsigned long long last_task_id = 0; // id da ultima tarefa criada
task_t *curr = NULL;                 // task atualmente rodando

void ppos_init()
{
    // desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);

    // inicia a tarefa main
    main_task.id = 0;
    main_task.status = RUNNING;
    curr = &main_task;

#ifdef DEBUG
    printf("ppos_init: task main iniciada, id = %d\n", main_task.id);
#endif
}

int task_init(task_t *task, void (*start_routine)(void *), void *arg)
{
    getcontext(&(task->context));

    // inicializa a pilha da tarefa
    char *stack = malloc(STACKSIZE);
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }
    else
    {
        perror("Erro na criação da pilha: ");
        return STACK_CREATION_ERROR;
    }

    makecontext(&(task->context), (void *)(*start_routine), 1, arg);

    // atualiza informacoes gerenciais
    task->id = ++last_task_id;
    task->status = READY;
    task->prev = task->next = NULL;

#ifdef DEBUG
    printf("task_init: task inicializada, id = %d, status = %d\n", task->id, task->status);
#endif

    return task->id;
}

int task_id(void)
{
    return curr->id;
}

int task_switch(task_t *task)
{
    if (!task)
    {
        perror("Ponteiro inválido");
        return NULL_PTR_ERROR;
    }

#ifdef DEBUG
    printf("task_switch: trocando de task id = %d para id = %d\n", curr->id, task->id);
#endif

    curr->status = SUSPENDED;
    task->status = RUNNING;

    task_t *prev = curr;
    curr = task;
    return swapcontext(&(prev->context), &(task->context));
}

void task_exit(int exit_code)
{
    if (curr->id == 0)
    {
        curr->status = TERMINATED;
        exit(SUCESS);
    }

#ifdef DEBUG
    printf("task_exit: encerrando a task id = %d e retornando para main, id = %d\n", curr->id, main_task.id);
#endif

    curr->status = TERMINATED;
    task_t *prev = curr;
    curr = &main_task;

    swapcontext(&(prev->context), &(main_task.context));
}
