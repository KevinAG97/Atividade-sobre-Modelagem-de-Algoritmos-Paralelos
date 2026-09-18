/*
 * ex1_estatico.c — Exercício 1: PARTICIONAMENTO ESTÁTICO.
 *
 * O programa recebe o número de threads T e responde quantos números da
 * lista v(0), v(1), ..., v(K-1) são primos, dividindo a lista em T blocos
 * CONTÍGUOS de tamanho igual: a thread 0 testa os primeiros K/T números,
 * a thread 1 os K/T seguintes, e assim por diante.
 *
 * Já estão prontos: a geração da lista (valor), o teste de primalidade
 * (eh_primo) e a medição do tempo por thread. Complete os TODOs (a)-(c).
 *
 * Uso:      ./ex1_estatico <threads>
 * Conferir: o total deve ser EXATAMENTE 3025 primos, com qualquer T.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define K 32768LL   /* quantidade de números na lista */

/* O k-ésimo número da lista: cresce do 1.000.000.001 (10 dígitos)
 * até 644.206.773.400.001 (15 dígitos). Todos são ímpares. */
long long valor(long long k)
{
    return 1000000001LL + 600000LL * k * k;
}

/* Divisão tentativa, como na atividade anterior (agora com long long):
 * testa os divisores ímpares de 3 até a raiz quadrada de v.
 * Custo: quase nada se v tem um fator pequeno; milhões de divisões
 * se v é um primo grande. */
int eh_primo(long long v)
{
    if (v < 2)      return 0;
    if (v % 2 == 0) return v == 2;
    for (long long i = 3; i * i <= v; i += 2)
        if (v % i == 0)
            return 0;
    return 1;
}

/* Relógio monotônico, em segundos. */
double agora(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* Argumentos e resultados de cada thread. */
typedef struct {
    int       id;      /* 0, 1, 2, ...                       */
    long long k_ini;   /* primeiro índice do bloco           */
    long long k_fim;   /* um além do último índice do bloco  */
    long long primos;  /* resultado: primos encontrados      */
    double    tempo;   /* tempo gasto por esta thread        */
} args_t;

void *conta_bloco(void *p)
{
    args_t *a = (args_t *)p;
    double t0 = agora();

    long long meus_primos = 0;

    /* (b) Testa cada número do bloco desta thread. A contagem vai para
     * uma variável local; só copio para a struct no fim. */
    for (long long k = a->k_ini; k < a->k_fim; k++)
        if (eh_primo(valor(k)))
            meus_primos++;

    a->primos = meus_primos;
    a->tempo  = agora() - t0;
    return NULL;
}

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

    /* (a) Divide a lista em T blocos contíguos. Se K não for múltiplo de
     * T, as primeiras (K % T) threads ficam com um índice a mais. */
    long long base  = K / T;        /* tamanho mínimo de cada bloco    */
    long long resto = K % T;        /* quantas threads levam +1 índice */
    long long k = 0;

    for (int i = 0; i < T; i++) {
        long long tamanho = base + (i < resto ? 1 : 0);

        args[i].id     = i;
        args[i].k_ini  = k;
        args[i].k_fim  = k + tamanho;   /* um além do último índice */
        args[i].primos = 0;
        args[i].tempo  = 0.0;
        k = args[i].k_fim;

        pthread_create(&th[i], NULL, conta_bloco, &args[i]);
    }

    /* (c) Espera as threads e soma os parciais. É o join que garante que
     * args[i].primos já foi escrito — por isso não precisa de mutex. */
    long long total = 0;
    for (int i = 0; i < T; i++) {
        pthread_join(th[i], NULL);
        total += args[i].primos;   /* Q4: é aqui que os parciais se
                                    * encontram pela primeira vez */
    }

    double t_total = agora() - t_ini;

    for (int i = 0; i < T; i++)
        printf("Thread %d: %6lld primos em %6.2f s  (indices %lld a %lld)\n",
               args[i].id, args[i].primos, args[i].tempo,
               args[i].k_ini, args[i].k_fim - 1);

    printf("Total: %lld primos em %.2f s  (esperado: 3025)\n",
           total, t_total);
    return 0;
}
