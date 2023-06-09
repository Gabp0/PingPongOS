// Exemplo de definicao de tratador de sinal UNIX
// Consulte os sinais do UNIX com "man 7 signal"
//
// Carlos Maziero, 2015

#define _XOPEN_SOURCE 700 /* Single UNIX Specification, Version 4 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// operating system check
#if defined(_WIN32) || (!defined(__unix__) && !defined(__unix) && (!defined(__APPLE__) || !defined(__MACH__)))
#warning Este codigo foi planejado para ambientes UNIX (LInux, *BSD, MacOS). A compilacao e execucao em outros ambientes e responsabilidade do usuario.
#endif

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction interruption_action;

/* função que tratará os sinais recebidos */
void tratador(int signum)
{
   printf("Recebi o sinal %d\n", signum);
}

int main(void)
{
   // registra a ação para o sinal SIGINT (Ctrl-C no teclado)
   interruption_action.sa_handler = tratador;
   sigemptyset(&interruption_action.sa_mask);
   interruption_action.sa_flags = 0;

   if (sigaction(SIGINT, &interruption_action, 0) < 0)
   {
      perror("Erro em sigaction: ");
      exit(1);
   }

   /* laço vazio, não faz nada enquanto aguarda sinais */
   while (1)
      ;
}
