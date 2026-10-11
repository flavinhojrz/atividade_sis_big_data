#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

double A(long i, long j){
    if(i == j) return 2.0;
    return 1.0;
}

int main(int argc, char* argv[]){
    int comm_sz;
    int my_rank;

    long n = 1000;
    if(argc > 1) n = atol(argv[1]);
    int max_iter = 1000;
    double tolerancia = 1e-12;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    long linhas = n / comm_sz;
    long inicio = my_rank * linhas;
    long fim = inicio + linhas;
    if(my_rank == comm_sz - 1){
        fim = n;
    }

    double *x = malloc(n * sizeof(double));
    double *y_parte = malloc(n * sizeof(double));
    double *y = malloc(n * sizeof(double));

    double norma = 0.0;
    for(long i=0; i<n; i++){
        x[i] = i + 1;
        norma += x[i] * x[i];
    }
    norma = sqrt(norma);
    for(long i=0; i<n; i++){
        x[i] = x[i] / norma;
    }

    double lambda = 0.0;
    double lambda_antigo = 0.0;
    int iter;

    double t0 = MPI_Wtime();

    for(iter=1; iter<=max_iter; iter++){
        for(long i=0; i<n; i++){
            y_parte[i] = 0.0;
        }
        for(long i=inicio; i<fim; i++){
            double soma = 0.0;
            for(long j=0; j<n; j++){
                soma += A(i, j) * x[j];
            }
            y_parte[i] = soma;
        }

        MPI_Allreduce(y_parte, y, n, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        norma = 0.0;
        for(long i=0; i<n; i++){
            norma += y[i] * y[i];
        }
        norma = sqrt(norma);

        for(long i=0; i<n; i++){
            x[i] = y[i] / norma;
        }

        lambda = norma;
        if(fabs(lambda - lambda_antigo) < tolerancia * lambda) break;
        lambda_antigo = lambda;
    }

    double t1 = MPI_Wtime();

    if(my_rank == 0){
        double exato = n + 1;
        double v_exato = 1.0 / sqrt(n);
        double erro_vetor = 0.0;
        for(long i=0; i<n; i++){
            double e = fabs(x[i] - v_exato);
            if(e > erro_vetor) erro_vetor = e;
        }
        printf("%-8ld %-4d %-6d %-20.10f %-10.0f %-12.2e %-12.2e %-10.4f\n",
               n, comm_sz, iter, lambda, exato, fabs(lambda - exato), erro_vetor, t1 - t0);
    }

    free(x);
    free(y_parte);
    free(y);
    MPI_Finalize();
    return 0;
}
