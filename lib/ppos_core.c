// GRR20203895 Gabriel de Oliveira Pontarolo
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

// #define DEBUG

#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_RESET "\x1b[0m"

// debug message macro
#ifdef DEBUG
#define DEBUG_MSG(...)            \
    fprintf(stderr, ANSI_YELLOW); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, ANSI_RESET);  \
    fflush(stderr);

// evita warning "state with no effect"
#else
#define DEBUG_MSG(...) \
    while (0)          \
        ;
#endif

task_t main_task;
task_t dispatcher_task;              // tarefa do dispatcher
unsigned long long last_task_id = 0; // id da ultima tarefa criada
task_t *curr = NULL;                 // task atualmente rodando
task_t *user_tasks = NULL;           // fila de tarefas do usuario

// declaracoes de prototipos de funcoes privadas
void dispatcher_init(void);
void main_init(void);
void dispatcher(void);
task_t *scheduler(void);
void task_print(void *elem);

void ppos_init()
{
    DEBUG_MSG("ppos_init: inicializando o SO\n");

    // desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);

    dispatcher_init();
    DEBUG_MSG("ppos_init: dispatcher inicializado\n");

    main_init();
    DEBUG_MSG("ppos_init: task main iniciada, id = %d\n", main_task.id);
}

void main_init(void)
{
    DEBUG_MSG("main_init: inicializando a main\n");
    getcontext(&(main_task.context));

    // atualiza informacoes gerenciais
    main_task.id = MAIN_PID;
    main_task.status = READY;
    main_task.prev = main_task.next = NULL;
    main_task.static_priority = DEFAULT_PRIORITY;
    main_task.dynamic_priority = DEFAULT_PRIORITY;

    DEBUG_MSG("main_init: adicionando na fila...\n");

    queue_append((queue_t **)&user_tasks, (queue_t *)&main_task);
    curr = &main_task;
}

void dispatcher_init(void)
{
    DEBUG_MSG("dispatcher_init: inicializando o dispatcher\n");
    getcontext(&(dispatcher_task.context));

    // inicializa a pilha da tarefa
    char *stack = malloc(STACKSIZE);
    if (stack)
    {
        dispatcher_task.context.uc_stack.ss_sp = stack;
        dispatcher_task.context.uc_stack.ss_size = STACKSIZE;
        dispatcher_task.context.uc_stack.ss_flags = 0;
        dispatcher_task.context.uc_link = 0;
    }
    else
    {
        perror("Erro na criação da pilha para o dispatcher: ");
        exit(STACK_CREATION_ERROR);
    }

    makecontext(&(dispatcher_task.context), (void *)(dispatcher), 1, NULL);

    // atualiza informacoes gerenciais
    dispatcher_task.id = DISPATCHER_PID;
    dispatcher_task.status = RUNNING;
    dispatcher_task.static_priority = MAX_PRIORITY;
    dispatcher_task.prev = dispatcher_task.next = NULL;
}

task_t *scheduler(void)
{

    DEBUG_MSG("scheduler: procurando nova task\n");
#ifdef DEBUG
    printf(ANSI_MAGENTA);
    queue_print("scheduler: user_tasks", (queue_t *)user_tasks, (void *)task_print);
    printf(ANSI_RESET);
#endif
    
    task_t *aux = user_tasks;
    task_t *next = user_tasks;

    DEBUG_MSG("scheduler: envelecendo as tasks e esolhendo a proxima\n");
    do
    {
        if (aux->dynamic_priority < next->dynamic_priority)
        {
            next = aux;
        }
        aux->dynamic_priority += ALPHA;
        aux = aux->next;
    
    }
    while (aux->next != user_tasks)
   
    DEBUG_MSG("scheduler: task escolhida %d com prio %d\n", next->id, next->dynamic_priority);

    // task vai para o final da fila
    user_tasks = (task_t *)user_tasks->next;

    return next;
}

void dispatcher(void)
{
    while (queue_size((queue_t *)user_tasks) > 0)
    {

        DEBUG_MSG("dispatcher: recebendo o controle\n");

        task_t *next = scheduler();

        DEBUG_MSG("dispatcher: proxima task %d\n", next->id);

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
                /* code */
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

    DEBUG_MSG("task_init: iniciando nova task...\n");

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
        exit(STACK_CREATION_ERROR);
    }

    makecontext(&(task->context), (void *)(*start_routine), 1, arg);

    // atualiza informacoes gerenciais
    task->id = ++last_task_id;
    task->status = READY;
    task->prev = task->next = NULL;
    task->static_priority = DEFAULT_PRIORITY;
    task->dynamic_priority = DEFAULT_PRIORITY;

    DEBUG_MSG("task_init: adicionando na fila...\n");

    queue_append((queue_t **)&user_tasks, (queue_t *)task);

    DEBUG_MSG("task_init: task inicializada, id = %d, status = %d\n", task->id, task->status);

    return task->id;
}

void task_yield()
{
    DEBUG_MSG("task_yield: recebendo o controle\n")
    DEBUG_MSG("task_yield: taks id = %d passou o controle para o dispatcher\n", curr->id);

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
        exit(NULL_PTR_ERROR);
    }

    DEBUG_MSG("task_switch: trocando de task id = %d para id = %d\n", curr->id, task->id);

    curr->status = SUSPENDED;
    curr->dynamic_priority = curr->static_priority;
    task->status = RUNNING;

    task_t *prev = curr;
    curr = task;
    return swapcontext(&(prev->context), &(task->context));
}

void task_exit(int exit_code)
{
    curr->status = TERMINATED;
    task_t *prev = curr;

    // retorna para main caso seja o dispatcher
    if (prev->id == DISPATCHER_PID)
    {

        DEBUG_MSG("task_exit: task dispatcher finalizada, encerrando o SO\n");

        exit(EXIT_SUCCESS);
    }
    else
    {

        DEBUG_MSG("task_exit: encerrando a task id = %d e retornando para o dispatcher\n", curr->id);

        queue_remove((queue_t **)&user_tasks, (queue_t *)prev);
        curr = &dispatcher_task;
    }

    swapcontext(&(prev->context), &(curr->context));
}

void task_setprio(task_t *task, int prio)
{

    DEBUG_MSG("task_setprio: definindo prioridade\n");

    if (!task)
    {
        task = curr;
        DEBUG_MSG("task_setprio: definindo prioridade da task atual id = %d\n", task->id);
    }

    DEBUG_MSG("task_setprio: definido prioridade da task id = %d para %d\n", task->id, prio);

    task->static_priority = prio;
    task->dynamic_priority = prio;
}

int task_getprio(task_t *task)
{

    DEBUG_MSG("task_getprio: obtendo prioridade\n");

    if (!task)
    {
        task = curr;
        DEBUG_MSG("task_getprio: obtendo prioridade da task atual id = %d\n", task->id);
    }

    DEBUG_MSG("task_getprio: retornando prioridade da task id = %d\n", task->id);

    return task->static_priority;
}

void task_print(void *elem)
{
    task_t *qelem = elem;
    printf("id: %d status: %d prio: %d;", qelem->id, qelem->status, qelem->dynamic_priority);
}
