// GRR20203895 Gabriel de Oliveira Pontarolo

#define _XOPEN_SOURCE 600

#include "ppos_disk.h"
#include "disk.h"
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

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
    fprintf(stderr, ANSI_RED);    \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, ANSI_RESET);  \
    fflush(stderr);

// evita warning "state with no effect"
#else
#define DEBUG_MSG(...) \
    while (0)          \
        ;
#endif

disk_t disk;
struct sigaction disk_action;

void disk_signal_handler(int signum);
void disk_driver(void *args);
void request_print(void *elem);
int disk_block_op(int block, void *buffer, operations op);

void disk_signal_handler(int signum)
{
    disk.signal = 1;
    task_resume(&disk_driver_task, &disk.disk_queue);
}

void disk_driver(void *args)
{
    DEBUG_MSG("disk_driver: inicializando\n");
    while (1)
    {
        sem_down(&disk.sem_disk);

        // se foi acordado devido a um sinal do disco
        if (disk.signal)
        {
            DEBUG_MSG("disk_driver: sinal recebido, acordando primeira tarefa\n");
            // acorda a tarefa cujo pedido foi atendido
            task_resume(disk.disk_queue, &disk.disk_queue);
            disk.signal = 0;
        }

#ifdef DEBUG
        fprintf(stderr, ANSI_BLUE);
        queue_print("disk_driver: request queue", (queue_t *)disk.request_queue, (void *)request_print);
        fprintf(stderr, ANSI_RESET);
#endif

        // se o disco estiver livre e houver pedidos de E/S na fila
        if ((disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE) && (queue_size((queue_t *)disk.request_queue) > 0))
        {
            DEBUG_MSG("disk_driver: disco livre, atendendo pedido\n");

            // escolhe na fila o pedido a ser atendido, usando FCFS
            request_t *request = (request_t *)disk.request_queue;
            queue_remove((queue_t **)&disk.request_queue, (queue_t *)disk.request_queue);

            // solicita ao disco a operação de E/S, usando disk_cmd()
            switch (request->op)
            {
            case READ:
                DEBUG_MSG("disk_driver: fazendo pedido de leitura de bloco %d\n", request->block);
                if (disk_cmd(DISK_CMD_READ, request->block, request->buffer) < 0)
                {
                    DEBUG_MSG("disk_driver: erro ao ler bloco");
                }
                break;

            case WRITE:
                DEBUG_MSG("disk_driver: fazendo pedido de escrita de bloco %d\n", request->block);
                if (disk_cmd(DISK_CMD_WRITE, request->block, request->buffer) < 0)
                {
                    DEBUG_MSG("disk_driver: erro ao escrever bloco");
                }
                break;

            default:
                break;
            }

            free(request);
        }

        sem_up(&disk.sem_disk);

        DEBUG_MSG("disk_driver: indo dormir\n");
        task_suspend(&disk.disk_queue);
        DEBUG_MSG("disk_driver: acordado\n")
    }

    task_exit(0);
}

int disk_mgr_init(int *numBlocks, int *blockSize)
{
    DEBUG_MSG("disk_mgr_init: inicializando gerente de disco\n");
    int init_status = disk_cmd(DISK_CMD_INIT, 0, 0);
    if (init_status < 0)
    {
        return -1;
    }

    int block_num = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    int block_size = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if (block_num < 0 || block_size < 0)
    {
        return -1;
    }

    *numBlocks = block_num;
    *blockSize = block_size;

    disk.num_blocks = block_num;
    disk.block_size = block_size;

    DEBUG_MSG("disk_mgr_init: inicializando tarefa e semafaro de disco\n")
    if (sem_init(&disk.sem_disk, 1) < 0)
    {
        return -1;
    }
    if (task_init(&disk_driver_task, (void *)disk_driver, NULL) < 0)
    {
        return -1;
    }

    DEBUG_MSG("disk_mgr_init: disco inicializado com %d blocos de %d bytes\n", block_num, block_size);

    // define o tratador para SIGALRM
    DEBUG_MSG("disk_mgr_init: definindo o tratador de interrupcoes\n");
    disk_action.sa_handler = disk_signal_handler;
    sigemptyset(&disk_action.sa_mask);
    disk_action.sa_flags = 0;
    if (sigaction(SIGUSR1, &disk_action, 0) < 0)
    {
        perror("Erro em sigaction: ");
        exit(INTERRUPTION_INIT_ERROR);
    }

    return 0;
}

int disk_block_op(int block, void *buffer, operations op)
{
    DEBUG_MSG("disk_block_op: solicitada operacao do bloco %d\n", block);
    sem_down(&disk.sem_disk);

    // cria um pedido de leitura
    DEBUG_MSG("disk_block_op: enfileirando pedido do bloco %d\n", block);
    request_t *request = (request_t *)malloc(sizeof(request_t));
    if (!request)
    {
        DEBUG_MSG("disk_block_op: erro ao alocar memoria para pedido\n");
        return -1;
    }
    request->prev = request->next = NULL;
    request->tid = task_id();
    request->block = block;
    request->buffer = buffer;
    request->op = op;
    queue_append((queue_t **)&disk.request_queue, (queue_t *)request);

#ifdef DEBUG
    fprintf(stderr, ANSI_BLUE);
    queue_print("disk_driver: request queue", (queue_t *)disk.request_queue, (void *)request_print);
    fprintf(stderr, ANSI_RESET);
#endif

    DEBUG_MSG("disk_block_op: checando tarefa do disco\n");
    if (disk_driver_task.status == SUSPENDED)
    {
        task_resume(&disk_driver_task, &disk.disk_queue);
    }

    sem_up(&disk.sem_disk);

    task_suspend(&disk.disk_queue);

    return 0;
}

int disk_block_read(int block, void *buffer)
{
    DEBUG_MSG("disk_block_read: solicitada leitura do bloco %d\n", block);
    return disk_block_op(block, buffer, READ);
}

int disk_block_write(int block, void *buffer)
{
    DEBUG_MSG("disk_block_write: solicitada escrita do bloco %d\n", block);
    return disk_block_op(block, buffer, WRITE);
}

void request_print(void *elem)
{
    request_t *relem = elem;
    fprintf(stderr, "by:%d type:%d block:%d;", relem->tid, relem->op, relem->block);
}
