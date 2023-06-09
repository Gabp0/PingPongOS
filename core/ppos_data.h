// GRR20203895 Gabriel de Oliveira Pontarolo

// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023
// Modificado por Gabriel Pontarolo

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h> // biblioteca POSIX de trocas de contexto

#define STACKSIZE 64 * 1024 // tamanho de pilha das threads

#define MAIN_PID 0
#define DISPATCHER_PID -1

#define MIN_PRIORITY 20
#define DEFAULT_PRIORITY 0
#define MAX_PRIORITY -20
#define ALPHA -1

#define TIMER_TICKS 1000 // em microssegundos (1 milissegundo)
#define QUANTUM_LIMIT 10

// status das tasks atuais
enum task_state
{
    READY,
    RUNNING,
    SUSPENDED,
    TERMINATED,
};

// error exit codes
enum error_codes
{
    STACK_CREATION_ERROR = 2,
    NULL_PTR_ERROR,
    INTERRUPTION_INIT_ERROR,
};

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
    struct task_t *prev, *next; // ponteiros para usar em filas

    int id;             // identificador da tarefa
    ucontext_t context; // contexto armazenado da tarefa
    short status;       // pronta, rodando, suspensa, ...

    int quantum; // tempo de execucao da tarefa

    int exit_code; // codigo de saida da tarefa

    short dynamic_priority; // prioridade dinâmica da tarefa
    short static_priority;  // prioridade estática da tarefa

    unsigned int execution_time; // tempo desde a criacao da tarefa
    unsigned int processor_time; // tempo passado executando
    unsigned int activations;    // numero de vezes em que foi feito task_switch para ela

    int waiting_task; // id da tarefa que a tarefa atual está esperando

    unsigned int wake_up_time; // tempo em que a tarefa deve acordar

    // ... (outros campos serão adicionados mais tarde)
} task_t;

// estrutura que define um semáforo
typedef struct
{
    int value;     // capacidade do semáforo
    task_t *queue; // fila de tarefas bloqueadas no semáforo

    int lock; // para evitar raceconditions

    int active; // indica se o semaforo nao foi destruido
} semaphore_t;

// estrutura que define um mutex
typedef struct
{
    // preencher quando necessário
} mutex_t;

// estrutura que define uma barreira
typedef struct
{
    // preencher quando necessário
} barrier_t;

// estrutura que define uma fila de mensagens
typedef struct msg_t
{
    struct msg_t *prev, *next; // ponteiros para usar em filas
    void *msg;                 // conteúdo da mensagem
} msg_t;

typedef struct
{
    msg_t *buffer; // buffer da fila

    int max_msgs; // capacidade da fila
    int msg_size; // tamanho da mensagem em bytes

    semaphore_t s_buffer; // controle do buffer
    semaphore_t s_item;   // controle de consumo dos itens
    semaphore_t s_vaga;   // controle de vaga no buffer

    int active; // indica se a fila nao foi destruida
} mqueue_t;

extern task_t disk_driver_task;

#endif
