// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

#include "ppos.h"

// operacoes de disco
typedef enum operations
{
  READ,
  WRITE
} operations;

// estrutura que representa uma requisicao de operacao em disco
typedef struct request_t
{
  struct request_t *prev, *next; // ponteiros para fila

  int tid;       // id da tarefa que fez a requisicao
  int block;     // bloco a ser lido/escrito
  void *buffer;  // buffer de dados
  operations op; // operacao de leitura/escrita
} request_t;

// estrutura que representa um disco no sistema operacional
typedef struct
{
  int num_blocks; // numero de blocos do disco
  int block_size; // tamanho de cada bloco, em bytes

  semaphore_t sem_disk;     // semafaro de acesso ao disco
  task_t *disk_queue;       // fila de tarefas esperando operacao em disco
  request_t *request_queue; // fila de requisicoes de operacao em disco

  int signal; // se o disco gerou uma interrupcao
  // completar com os campos necessarios
} disk_t;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init(int *numBlocks, int *blockSize);

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer);

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer);

#endif
