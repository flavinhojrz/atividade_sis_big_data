#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#define MAX_STRING 400

double f(double x){
    return x * x;
}

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
    int comm_sz;
    int my_rank;

    double a = 0.0, b = 3.0;
    long n = 1000;
    if(argc > 1) n = atol(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Get_processor_name(nome_no, &tam_nome);

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

    double h = (b - a) / n;
    long n_local = n / comm_sz;
    double a_local = a + my_rank * n_local * h;
    double b_local = a_local + n_local * h;

    if(my_rank == comm_sz - 1){
        n_local = n - my_rank * (n / comm_sz);
        b_local = b;
    }

    double integral_local = trapezio(a_local, b_local, n_local, h);

    if(my_rank != 0){
        MPI_Send(&integral_local, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    } else {
        double total = integral_local;
        double parcial;
        for(int i=1; i<comm_sz; i++){
            MPI_Recv(&parcial, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            total += parcial;
        }

        double exato = (b*b*b - a*a*a) / 3.0;
        printf("n = %ld | estimado = %.12f | exato = %.12f | erro = %.3e\n",
               n, total, exato, fabs(total - exato));
    }

    MPI_Finalize();
    return 0;
}
