#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#define MAX_STRING 400

/* Função que vamos integrar */
double f(double x){
    return x * x;
}

/* Regra do trapézio no pedaço [a_local, b_local] com n_local trapézios de largura h */
double trapezio(double a_local, double b_local, long n_local, double h){
    double soma = (f(a_local) + f(b_local)) / 2.0;
    for(long i=1; i<n_local; i++){
        soma += f(a_local + i*h);
    }
    return soma * h;
}

int main(int argc, char* argv[]){
    char saudacao[MAX_STRING];
    char nome_no[MPI_MAX_PROCESSOR_NAME];
    int tam_nome;
    int comm_sz;                 /* Numero de processos */
    int my_rank;                 /* ID do meu processo  */

    double a = 0.0, b = 3.0;     /* Limites da integral */
    long n = 1000;               /* Numero total de trapezios */
    if(argc > 1) n = atol(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Get_processor_name(nome_no, &tam_nome);

    /* ---- Parte 1: saudacao (codigo do professor), agora com o nome do no ---- */
    if(my_rank != 0){
        snprintf(saudacao, MAX_STRING, "Saudacao do processo %d/%d (no: %s)!", my_rank, comm_sz, nome_no);
        MPI_Send(saudacao, strlen(saudacao)+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    } else {
        printf("Saudacao do processo %d/%d (no: %s)!\n", my_rank, comm_sz, nome_no);
        for(int i=1; i<comm_sz; i++){
            MPI_Recv(saudacao, MAX_STRING, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%s\n", saudacao);
        }
    }

    /* ---- Parte 2: regra do trapezio ---- */
    double h = (b - a) / n;              /* Largura de cada trapezio (igual para todos) */
    long n_local = n / comm_sz;          /* Quantos trapezios cada processo calcula */
    double a_local = a + my_rank * n_local * h;
    double b_local = a_local + n_local * h;

    /* O ultimo processo fica com as sobras se n nao for divisivel por comm_sz */
    if(my_rank == comm_sz - 1){
        n_local = n - my_rank * (n / comm_sz);
        b_local = b;
    }

    double integral_local = trapezio(a_local, b_local, n_local, h);

    if(my_rank != 0){
        /* Processos 1, 2, 3...: enviam o resultado parcial para o processo 0 */
        MPI_Send(&integral_local, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    } else {
        /* Processo 0: soma o proprio pedaco com os pedacos dos outros */
        double total = integral_local;
        double parcial;
        for(int i=1; i<comm_sz; i++){
            MPI_Recv(&parcial, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            total += parcial;
        }

        double exato = (b*b*b - a*a*a) / 3.0;   /* Primitiva de x^2 e x^3/3 */
        printf("n = %ld | estimado = %.12f | exato = %.12f | erro = %.3e\n",
               n, total, exato, fabs(total - exato));
    }

    MPI_Finalize();
    return 0;
}
