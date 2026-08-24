/*
 * ex0_fila.c — Parte 0 (exemplo completo): fila limitada com produtor e
 * consumidores, usando mutex e VARIÁVEIS DE CONDIÇÃO (pthread_cond_t).
 *
 * Este arquivo está pronto: compile, execute e leia com atenção.
 * Ele é o modelo para a fila de tarefas que você vai completar no ex2_bag.c.
 *
 * A ideia:
 *   - Um PRODUTOR (a própria main) insere 20 tarefas em uma fila que só
 *     comporta 4 itens por vez (fila LIMITADA).
 *   - Três CONSUMIDORES (threads) retiram tarefas e as "processam".
 *   - Quando as tarefas acabam, o produtor insere uma TAREFA DE
 *     ENCERRAMENTO (FIM) para cada consumidor: ao retirá-la, o consumidor
 *     sabe que deve encerrar. (Na literatura esse padrão é conhecido
 *     como "poison pill".)
 *
 * Por que precisamos de variáveis de condição?
 *   - Se um consumidor encontra a fila VAZIA, ele precisa ESPERAR até que
 *     o produtor insira algo. Esperar "girando" num laço (busy-wait)
 *     desperdiçaria CPU; pthread_cond_wait põe a thread para DORMIR.
 *   - Se o produtor encontra a fila CHEIA, ele também espera (nao_cheia).
 *
 * Compile: gcc -Wall -O2 -o ex0_fila ex0_fila.c -pthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define CAPACIDADE     4    /* a fila comporta no máximo 4 itens         */
#define N_TAREFAS     20    /* total de tarefas "de verdade"             */
#define N_CONSUMIDORES 3
#define FIM           -1    /* tarefa especial: "não há mais trabalho"   */

/* ---------------- A fila (circular) e sua proteção ---------------- */

int itens[CAPACIDADE];
int inicio = 0;   /* posição do item mais antigo                        */
int tras   = 0;   /* posição onde o próximo item será inserido          */
int tam    = 0;   /* quantos itens a fila contém agora                  */

pthread_mutex_t mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  nao_vazia = PTHREAD_COND_INITIALIZER; /* "tem item!"    */
pthread_cond_t  nao_cheia = PTHREAD_COND_INITIALIZER; /* "tem espaço!"  */

/* Insere um item na fila; se estiver cheia, espera abrir espaço. */
void insere(int v)
{
    pthread_mutex_lock(&mutex);

    /* Enquanto a fila estiver cheia, dorme esperando o aviso nao_cheia.
     * ATENÇÃO: é um while, não um if. Ao acordar, a thread reconfere a
     * condição, porque (1) pode haver "acordadas espúrias" e (2) outra
     * thread pode ter agido entre o aviso e a nossa volta ao mutex.
     * (Este é o assunto da Q1.)
     *
     * pthread_cond_wait LIBERA o mutex enquanto a thread dorme e o
     * READQUIRE automaticamente antes de retornar. (Assunto da Q2.) */
    while (tam == CAPACIDADE)
        pthread_cond_wait(&nao_cheia, &mutex);

    itens[tras] = v;
    tras = (tras + 1) % CAPACIDADE;
    tam++;

    /* Avisa UMA thread que espera por item (se houver alguma). */
    pthread_cond_signal(&nao_vazia);

    pthread_mutex_unlock(&mutex);
}

/* Retira um item da fila; se estiver vazia, espera chegar algo. */
int retira(void)
{
    pthread_mutex_lock(&mutex);

    while (tam == 0)                            /* while, nunca if! (Q1) */
        pthread_cond_wait(&nao_vazia, &mutex);

    int v = itens[inicio];
    inicio = (inicio + 1) % CAPACIDADE;
    tam--;

    /* Avisa o produtor que abriu espaço. */
    pthread_cond_signal(&nao_cheia);

    pthread_mutex_unlock(&mutex);
    return v;
}

/* ---------------------- Os consumidores --------------------------- */

void *consumidor(void *arg)
{
    int id = *(int *)arg;
    int processadas = 0;

    while (1) {
        int tarefa = retira();

        if (tarefa == FIM) {
            /* Não há mais trabalho: encerra a thread. (Q3) */
            printf("[consumidor %d] FIM recebido, encerrando "
                   "(processei %d tarefas)\n", id, processadas);
            return NULL;
        }

        /* "Processa" a tarefa (aqui, só imprime e perde um tempinho). */
        printf("[consumidor %d] processando tarefa %2d\n", id, tarefa);
        usleep(100 * 1000); /* 100 ms */
        processadas++;
    }
}

/* ------------------------- O produtor ----------------------------- */

int main(void)
{
    pthread_t th[N_CONSUMIDORES];
    int ids[N_CONSUMIDORES];

    for (int i = 0; i < N_CONSUMIDORES; i++) {
        ids[i] = i;
        pthread_create(&th[i], NULL, consumidor, &ids[i]);
    }

    /* Produz as tarefas. Como a fila só comporta 4 itens, o produtor
     * naturalmente espera os consumidores abrirem espaço. */
    for (int t = 1; t <= N_TAREFAS; t++) {
        insere(t);
        printf("[produtor    ] inseri a tarefa %2d\n", t);
    }

    /* Fim do trabalho: uma tarefa FIM para CADA consumidor. (Q3: o que
     * acontece se estas inserções forem comentadas?) */
    for (int i = 0; i < N_CONSUMIDORES; i++)
        insere(FIM);

    for (int i = 0; i < N_CONSUMIDORES; i++)
        pthread_join(th[i], NULL);

    printf("[produtor    ] todos os consumidores encerraram. Fim.\n");
    return 0;
}
