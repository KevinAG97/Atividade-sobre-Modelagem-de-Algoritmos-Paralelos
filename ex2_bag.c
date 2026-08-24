/*
 * ex2_bag.c — Exercício 2: BAG-OF-TASKS.
 *
 * Mesmo problema do Exercício 1, agora com distribuição DINÂMICA:
 *
 *   - A main faz o papel de MESTRE: percorre a lista e insere cada
 *     número na fila de tarefas (uma tarefa = um número). Ao final,
 *     insere uma tarefa de encerramento (FIM) para cada trabalhadora.
 *   - As T threads TRABALHADORAS retiram um número da fila, testam se
 *     é primo (acumulando numa variável local) e voltam à fila para
 *     pegar o próximo — quem termina antes pega mais trabalho.
 *
 * A fila é limitada, exatamente como a do ex0_fila.c: use aquele código
 * como referência para completar os TODOs (a)-(c).
 *
 * Uso:      ./ex2_bag <threads>
 * Conferir: o total deve ser EXATAMENTE 3025 primos, com qualquer T,
 *           igual ao do Exercício 1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define K        32768LL   /* quantidade de números na lista           */
#define CAP_FILA 1024      /* capacidade da fila de tarefas            */
#define FIM      -1LL      /* tarefa especial: "não há mais trabalho"  */

/* ------------- Iguais ao Exercício 1 (já prontos) ------------------ */

long long valor(long long k)
{
    return 1000000001LL + 600000LL * k * k;
}

int eh_primo(long long v)
{
    if (v < 2)      return 0;
    if (v % 2 == 0) return v == 2;
    for (long long i = 3; i * i <= v; i += 2)
        if (v % i == 0)
            return 0;
    return 1;
}

double agora(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* --------------- A fila de tarefas (a "sacola") -------------------- */

long long itens[CAP_FILA];
int inicio = 0;
int tras   = 0;
int tam    = 0;

pthread_mutex_t mutex_fila = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  nao_vazia  = PTHREAD_COND_INITIALIZER;
pthread_cond_t  nao_cheia  = PTHREAD_COND_INITIALIZER;

/* Insere uma tarefa na fila; se estiver cheia, espera abrir espaço. */
void insere(long long v)
{
    pthread_mutex_lock(&mutex_fila);

    /* TODO (a): 1. espere enquanto a fila estiver cheia (tam == CAP_FILA),
     *              com while + pthread_cond_wait em nao_cheia;
     *           2. insira v no buffer circular:
     *                itens[tras] = v;
     *                tras = (tras + 1) % CAP_FILA;
     *                tam++;
     *           3. sinalize nao_vazia com pthread_cond_signal. */

    pthread_mutex_unlock(&mutex_fila);
}

/* Retira uma tarefa da fila; se estiver vazia, espera chegar algo. */
long long retira(void)
{
    long long v = FIM;
    pthread_mutex_lock(&mutex_fila);

    /* TODO (b): 1. espere enquanto a fila estiver vazia (tam == 0),
     *              com while + pthread_cond_wait em nao_vazia;
     *           2. retire o item mais antigo do buffer circular:
     *                v = itens[inicio];
     *                inicio = (inicio + 1) % CAP_FILA;
     *                tam--;
     *           3. sinalize nao_cheia com pthread_cond_signal. */

    pthread_mutex_unlock(&mutex_fila);
    return v;
}

/* -------------------- Total global e trabalhadoras ------------------ */

long long       total_primos = 0;
pthread_mutex_t mutex_total  = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int       id;
    long long primos;   /* quantos primos ESTA thread encontrou */
    long long tarefas;  /* quantas tarefas ESTA thread executou */
    double    tempo;
} args_t;

void *trabalhadora(void *p)
{
    args_t *a = (args_t *)p;
    double t0 = agora();

    long long meus_primos  = 0;
    long long minhas_tarefas = 0;

    /* TODO (c): em um laço:
     *   1. retire uma tarefa da fila com retira();
     *   2. se for o FIM, saia do laço;
     *   3. senão, teste com eh_primo e acumule em meus_primos
     *      (e conte a tarefa em minhas_tarefas).
     * Depois do laço, some meus_primos ao total_primos global,
     * protegendo a soma com mutex_total. */

    a->primos  = meus_primos;
    a->tarefas = minhas_tarefas;
    a->tempo   = agora() - t0;
    return NULL;
}

/* ------------------------- O mestre --------------------------------- */

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "uso: %s <threads>\n", argv[0]);
        return 1;
    }
    int T = atoi(argv[1]);
    if (T < 1 || T > 64) {
        fprintf(stderr, "número de threads deve estar entre 1 e 64\n");
        return 1;
    }

    pthread_t th[64];
    args_t    args[64] = {{0}};
    double    t_ini = agora();

    /* Cria as trabalhadoras ANTES de encher a fila: como a fila é
     * limitada, elas já vão consumindo enquanto o mestre produz. */
    for (int i = 0; i < T; i++) {
        args[i].id = i;
        pthread_create(&th[i], NULL, trabalhadora, &args[i]);
    }

    /* O mestre percorre a lista e coloca cada número na sacola... */
    for (long long k = 0; k < K; k++)
        insere(valor(k));

    /* ...e encerra com uma tarefa FIM para cada trabalhadora. */
    for (int i = 0; i < T; i++)
        insere(FIM);

    for (int i = 0; i < T; i++)
        pthread_join(th[i], NULL);

    double t_total = agora() - t_ini;

    for (int i = 0; i < T; i++)
        printf("Thread %d: %6lld primos em %6.2f s  (%lld tarefas)\n",
               args[i].id, args[i].primos, args[i].tempo, args[i].tarefas);

    printf("Total: %lld primos em %.2f s  (esperado: 3025)\n",
           total_primos, t_total);
    return 0;
}
