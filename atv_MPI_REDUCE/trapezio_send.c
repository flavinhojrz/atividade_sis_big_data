#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

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
    int comm_sz;
    int my_rank;

    double a = 0.0, b = 3.0;
    long n = 1000;
    if(argc > 1) n = atol(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

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
        printf("versao = send   | p = %d | n = %ld | estimado = %.12f | erro = %.3e\n",
               comm_sz, n, total, fabs(total - exato));
    }

    MPI_Finalize();
    return 0;
}
