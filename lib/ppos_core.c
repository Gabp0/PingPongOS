// GRR20203895 Gabriel de Oliveira Pontarolo

#define _XOPEN_SOURCE 600 /* Single UNIX Specification, Version 3 */
// vscode reclamava das funcoes da lib signal.h sem essa definicao

#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>

#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_RESET "\x1b[0m"

// #define DEBUG

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

task_t main_task;       // tarefa main
task_t dispatcher_task; // tarefa do dispatcher

unsigned long long last_task_id = 0; // id da ultima tarefa criada

task_t *curr = NULL;    // task atualmente rodando
int user_tasks_num = 0; // numero de tarefas de usuario no sistema

task_t *ready_tasks = NULL;     // fila de tarefas do usuario
task_t *suspended_tasks = NULL; // fila de tarefas suspensas
task_t *sleeping_tasks = NULL;  // fila de tarefas dormindo

struct sigaction action; // estrutura que define um tratador de sinal (deve ser global ou static)
struct itimerval timer;  // estrutura de inicialização do timer
int clock_ticks = 0;     // numero atual de ticks do relogio

unsigned int sysclock = 0; // relogio do sistema (em ms)

// declaracoes de prototipos de funcoes privadas
void dispatcher_init(void);
void main_init(void);
void dispatcher(void);
task_t *scheduler(void);
void task_print(void *elem);
void timer_init();
void interruption_handler(int signum);

void ppos_init()
{
    DEBUG_MSG("ppos_init: inicializando o SO\n");

    // desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);

    dispatcher_init();
    DEBUG_MSG("ppos_init: dispatcher inicializado\n");

    main_init();
    DEBUG_MSG("ppos_init: task main iniciada, id = %d\n", main_task.id);

    timer_init();
    DEBUG_MSG("ppos_init: timer e tratador de interrupcoes inicializados\n");
}

void interruption_handler(int signum)
{
    sysclock++;
    clock_ticks++;
    if (clock_ticks == QUANTUM)
    {
        DEBUG_MSG("interruption_handler: fim de quantum, fazendo preempcao por tempo\n")
        clock_ticks = 0;
        if (curr->id == DISPATCHER_PID)
        {
            DEBUG_MSG("interruption_handler: dispatcher nao pode ser preemptado\n")
            return;
        }
        task_yield();
    }
}

void timer_init()
{
    // define o tratador para SIGALRM
    DEBUG_MSG("timer_init: definindo o tratador de interrupcoes\n");
    action.sa_handler = interruption_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0)
    {
        perror("Erro em sigaction: ");
        exit(INTERRUPTION_INIT_ERROR);
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1;              // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec = 0;               // primeiro disparo, em segundos
    timer.it_interval.tv_usec = TIMER_TICKS; // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec = 0;            // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL
    DEBUG_MSG("timer_init: armando o timer\n");
    if (setitimer(ITIMER_REAL, &timer, 0) < 0)
    {
        perror("Erro em setitimer: ");
        exit(INTERRUPTION_INIT_ERROR);
    }
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
    main_task.activations = 1;
    main_task.execution_time = systime();
    main_task.processor_time = 0;
    main_task.exit_code = 0;
    main_task.waiting_task = -1;

    DEBUG_MSG("main_init: adicionando na fila...\n");

    queue_append((queue_t **)&ready_tasks, (queue_t *)&main_task);
    user_tasks_num++;

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
    dispatcher_task.activations = 0;
    dispatcher_task.execution_time = systime();
    dispatcher_task.processor_time = 0;
    dispatcher_task.exit_code = 0;
    dispatcher_task.waiting_task = -1;
}

void task_print(void *elem)
{
    task_t *qelem = elem;
    fprintf(stderr, "id: %d status: %d prio: %d;", qelem->id, qelem->status, qelem->dynamic_priority);
}

task_t *scheduler(void)
{

    DEBUG_MSG("scheduler: procurando nova task\n");

#ifdef DEBUG
    fprintf(stderr, ANSI_MAGENTA);
    queue_print("scheduler: ready_tasks", (queue_t *)ready_tasks, (void *)task_print);
    fprintf(stderr, ANSI_RESET);
#endif

    task_t *aux = ready_tasks;
    task_t *next = ready_tasks;
    while (aux && (aux != ready_tasks->prev))
    {
        if (aux->dynamic_priority < next->dynamic_priority)
        {
            next = aux;
        }
        aux = aux->next;
    }
    if (aux && (aux->dynamic_priority < next->dynamic_priority))
    {
        next = aux;
    }

#ifdef DEBUG
    if (next)
    {
        DEBUG_MSG("scheduler: task escolhida %d com prio %d\n", next->id, next->dynamic_priority);
    }
    else
    {
        DEBUG_MSG("scheduler: fila de prontos vazia, sem proxima task\n");
    }
#endif

    DEBUG_MSG("scheduler: envelecendo as tasks\n");
    aux = ready_tasks;
    while (aux && (aux != ready_tasks->prev))
    {
        aux->dynamic_priority += ALPHA;
        aux = aux->next;
    }
    if (aux)
    {
        aux->dynamic_priority += ALPHA;
    }
#ifdef DEBUG
    fprintf(stderr, ANSI_MAGENTA);
    queue_print("scheduler: ready_tasks", (queue_t *)ready_tasks, (void *)task_print);
    fprintf(stderr, ANSI_RESET);
#endif

    // task vai para o final da fila
    // user_tasks = user_tasks->next;
    // reseta a prioridade
    if (next)
    {
        next->dynamic_priority = next->static_priority;
    }

    return next;
}

void wake_tasks(void)
{
    task_t *aux = sleeping_tasks, *aux_next;
    unsigned int now = systime();
    DEBUG_MSG("wake_tasks: verificando as tasks adormecidas em %d ms\n", now);

#ifdef DEBUG
    fprintf(stderr, ANSI_GREEN);
    queue_print("wake_tasks: sleeping_tasks", (queue_t *)sleeping_tasks, (void *)task_print);
    fprintf(stderr, ANSI_RESET);
#endif

    while (aux && (aux != sleeping_tasks->prev))
    {
        aux_next = aux->next;
        DEBUG_MSG("wake_tasks: checando task id = %d com wt = %d e now = %d\n", aux->id, aux->wake_up_time, now)
        if (aux->wake_up_time <= now)
        {
            DEBUG_MSG("wake_tasks: acordando task id = %d\n", aux->id)
            task_resume(aux, &sleeping_tasks);
        }
        aux = aux_next;
    }
    if (aux && (aux->wake_up_time <= now))
    {
        DEBUG_MSG("wake_tasks: acordando task id = %d\n", aux->id)
        task_resume(aux, &sleeping_tasks);
    }

#ifdef DEBUG
    fprintf(stderr, ANSI_MAGENTA);
    queue_print("wake_tasks: ready_tasks", (queue_t *)ready_tasks, (void *)task_print);
    fprintf(stderr, ANSI_RESET);
#endif
}

void dispatcher(void)
{
    while (user_tasks_num > 0)
    {

        DEBUG_MSG("dispatcher: recebendo o controle\n");
        dispatcher_task.activations++;

        wake_tasks();

        task_t *next = scheduler();

        if (next)
        {
            DEBUG_MSG("dispatcher: proxima task %d\n", next->id);
            unsigned int start = systime();
            next->activations++;
            task_switch(next);
            next->processor_time += systime() - start;

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
        else
        {
            DEBUG_MSG("dispatcher: sem proxima task\n");
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
    task->activations = 0;
    task->execution_time = systime();
    task->processor_time = 0;
    task->exit_code = 0;
    task->waiting_task = -1;

    DEBUG_MSG("task_init: adicionando na fila...\n");

    queue_append((queue_t **)&ready_tasks, (queue_t *)task);
    user_tasks_num++;

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
    task->status = RUNNING;

    task_t *prev = curr;
    curr = task;
    return swapcontext(&(prev->context), &(task->context));
}

void task_exit(int exit_code)
{
    curr->status = TERMINATED;
    task_t *prev = curr;
    curr->execution_time = systime() - curr->execution_time;
    curr->exit_code = exit_code;

    fprintf(stdout, "Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", curr->id, curr->execution_time, curr->processor_time, curr->activations);
    fflush(stdout);

    DEBUG_MSG("task_exit: acordando as tasks herdadas\n");
    task_t *aux = suspended_tasks, *next;
    while (aux && (aux != suspended_tasks->prev))
    {
        next = aux->next;
        if (aux->waiting_task == curr->id)
        {
            task_resume(aux, &suspended_tasks);
            aux->waiting_task = -1;
        }
        aux = next;
    }
    if (aux && (aux->waiting_task == curr->id)) // ultimo da fila
    {
        task_resume(aux, &suspended_tasks);
        aux->waiting_task = -1;
    }

    // retorna para main caso seja o dispatcher
    if (prev->id == DISPATCHER_PID)
    {

        DEBUG_MSG("task_exit: task dispatcher finalizada, encerrando o SO\n");

        exit(EXIT_SUCCESS);
    }
    else
    {

        DEBUG_MSG("task_exit: encerrando a task id = %d e retornando para o dispatcher\n", curr->id);

        queue_remove((queue_t **)&ready_tasks, (queue_t *)prev);
        user_tasks_num--;
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

unsigned int systime()
{
    return sysclock;
}

void task_suspend(task_t **queue)
{
    DEBUG_MSG("task_suspend: suspendendo a task id = %d\n", curr->id);

    if (!queue)
    {
        perror("Ponteiro inválido");
        exit(NULL_PTR_ERROR);
    }

    queue_remove((queue_t **)&ready_tasks, (queue_t *)curr);
    curr->status = SUSPENDED;
    queue_append((queue_t **)queue, (queue_t *)curr);

#ifdef DEBUG
    fprintf(stderr, ANSI_CYAN);
    queue_print("task_suspend: queue", (queue_t *)*queue, (void *)task_print);
    fprintf(stderr, ANSI_MAGENTA);
    queue_print("task_suspend: ready_tasks", (queue_t *)ready_tasks, (void *)task_print);
    fprintf(stderr, ANSI_RESET);
#endif

    task_switch(&dispatcher_task);
}

void task_resume(task_t *task, task_t **queue)
{
    DEBUG_MSG("task_resume: resumindo a task id = %d\n", task->id);

    if (!task || !queue)
    {
        perror("Ponteiro inválido");
        exit(NULL_PTR_ERROR);
    }

    queue_remove((queue_t **)queue, (queue_t *)task);
    task->status = READY;
    queue_append((queue_t **)ready_tasks, (queue_t *)task);
}

int task_wait(task_t *task)
{
    DEBUG_MSG("task_wait: task id = %d ira aguardar pela task id = %d\n", curr->id, task->id);

    if (!task || task->status == TERMINATED)
    {
        return -1;
    }

    curr->waiting_task = task->id;
    task_suspend(&suspended_tasks);

    DEBUG_MSG("task_wait: task id = %d retornou da task id = %d com exit_code = %d\n", curr->id, task->id, task->exit_code);

    return task->exit_code;
}

void task_sleep(int t)
{
    curr->wake_up_time = systime() + t;
    DEBUG_MSG("task_sleep: task id = %d ira dormir por %d millisegundos ate %d\n", curr->id, t, curr->wake_up_time);
    task_suspend(&sleeping_tasks);
}