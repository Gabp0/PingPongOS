#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

// #define DEBUG

task_t main_task;
task_t dispatcher_task;              // tarefa do dispatcher
unsigned long long last_task_id = 0; // id da ultima tarefa criada
task_t *curr = NULL;                 // task atualmente rodando
task_t *user_tasks = NULL;           // fila de tarefas do usuario

// declaracoes de prototipos de funcoes privadas
void dispatcher(void);
task_t *scheduler(void);
void task_print(void *elem);

void ppos_init()
{
    // desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);

    // inicia a tarefa main
    main_task.id = MAIN_PID;
    main_task.status = RUNNING;
    curr = &main_task;

#ifdef DEBUG
    printf("ppos_init: task main iniciada, id = %d\n", main_task.id);
#endif

#ifdef DEBUG
    printf("ppos_init: inicializando o dispatcher\n");
#endif

    // inicia a tarefa dispatcher
    task_init(&dispatcher_task, (void *)dispatcher, NULL);
}

task_t *scheduler(void)
{
#ifdef DEBUG
    printf("scheduler: procurando nova task\n");
    queue_print("scheduler: user_tasks", (queue_t *)user_tasks, (void *)task_print);
#endif

    task_t *next = user_tasks;
    queue_remove((queue_t **)&user_tasks, (queue_t *)next);

    return next;
}

void dispatcher(void)
{
    while (queue_size((queue_t *)user_tasks) > 0)
    {
#ifdef DEBUG
        printf("dispatcher: recebendo o controle\n");
#endif

        task_t *next = scheduler();

#ifdef DEBUG
        printf("dispatcher: proxima task %p\n", next);
#endif

        if (next)
        {
            task_switch(next);

            switch (next->status)
            {
            case READY:
                /* code */
                break;
            case TERMINATED:
                /* code */
                break;
            case SUSPENDED:
                /* code */
                break;
            case RUNNING:

                break;

            default:
                break;
            }
        }
    }

    task_exit(0);
}

int task_init(task_t *task, void (*start_routine)(void *), void *arg)
{
#ifdef DEBUG
    printf("task_init: iniciando nova task...\n");
#endif

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
    printf("task_init: adicionando na fila...\n");
#endif

    queue_append((queue_t **)&user_tasks, (queue_t *)task);

#ifdef DEBUG
    printf("task_init: task inicializada, id = %d, status = %d\n", task->id, task->status);
#endif

    return task->id;
}

void task_yield()
{
#ifdef DEBUG
    printf("task_yield: taks id = %d passou o controle para o dispatcher\n", curr->id);
#endif

    // nao coloca o main de volta na fila
    if (curr->id != 0)
    {
        queue_append((queue_t **)&user_tasks, (queue_t *)curr);
    }

    task_switch(&dispatcher_task);
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
    // encerra o programa caso seja a task main
    if (curr->id == MAIN_PID)
    {
        curr->status = TERMINATED;
        exit(SUCESS);
    }

    curr->status = TERMINATED;
    task_t *prev = curr;

    // retorna para main caso seja o dispatcher
    if (prev->id == DISPATCHER_PID)
    {
#ifdef DEBUG
        printf("task_exit: encerrando o dispatcher e retornando para o main\n");
#endif
        curr = &main_task;
    }
    else
    {
#ifdef DEBUG
        printf("task_exit: encerrando a task id = %d e retornando para o dispatcher\n", curr->id);
#endif
        curr = &dispatcher_task;
    }

    swapcontext(&(prev->context), &(curr->context));
}

void task_print(void *elem)
{
    task_t *qelem = elem;
    printf("id: %d status: %d;", qelem->id, qelem->status);
}