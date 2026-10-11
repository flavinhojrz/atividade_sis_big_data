#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

int main(int argc, char* argv[]){
    int comm_sz;
    int my_rank;

    int n = 1024;
    int steps = 100;
    if(argc > 1) n = atoi(argv[1]);
    if(argc > 2) steps = atoi(argv[2]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if(n % comm_sz != 0){
        if(my_rank == 0) printf("Erro: n = %d precisa ser multiplo de p = %d\n", n, comm_sz);
        MPI_Finalize();
        return 1;
    }

    int linhas = n / comm_sz;

    double *A = NULL;
    if(my_rank == 0){
        A = malloc((long)n * n * sizeof(double));
        for(int i=0; i<n; i++){
            for(int j=0; j<n; j++){
                if(i == j) A[(long)i*n + j] = 2.0;
                else A[(long)i*n + j] = 1.0;
            }
        }
    }

    double *A_local = malloc((long)linhas * n * sizeof(double));
    MPI_Scatter(A, linhas * n, MPI_DOUBLE, A_local, linhas * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if(my_rank == 0){
        free(A);
        A = NULL;
    }

    double *x = malloc(n * sizeof(double));
    double *y_local = malloc(linhas * sizeof(double));
    double *x_local = malloc(linhas * sizeof(double));

    double norma = 0.0;
    for(int i=0; i<n; i++){
        x[i] = i + 1;
        norma += x[i] * x[i];
    }
    norma = sqrt(norma);
    for(int i=0; i<n; i++){
        x[i] = x[i] / norma;
    }

    int inicio = my_rank * linhas;
    double lambda = 0.0;

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    for(int k=0; k<steps; k++){
        for(int i=0; i<linhas; i++){
            double soma = 0.0;
            for(int j=0; j<n; j++){
                soma += A_local[(long)i*n + j] * x[j];
            }
            y_local[i] = soma;
        }

        double xAx_local = 0.0;
        for(int i=0; i<linhas; i++){
            xAx_local += x[inicio + i] * y_local[i];
        }
        double xAx;
        MPI_Allreduce(&xAx_local, &xAx, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        double xx = 0.0;
        for(int i=0; i<n; i++){
            xx += x[i] * x[i];
        }

        lambda = xAx / xx;

        double norma_local = 0.0;
        for(int i=0; i<linhas; i++){
            norma_local += y_local[i] * y_local[i];
        }
        MPI_Allreduce(&norma_local, &norma, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        norma = sqrt(norma);

        for(int i=0; i<linhas; i++){
            x_local[i] = y_local[i] / norma;
        }
        MPI_Allgather(x_local, linhas, MPI_DOUBLE, x, linhas, MPI_DOUBLE, MPI_COMM_WORLD);
    }

    double t1 = MPI_Wtime();
    double tempo_local = t1 - t0;
    double tempo;
    MPI_Reduce(&tempo_local, &tempo, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if(my_rank == 0){
        printf("%d,%d,%d,%.6f,%.6f\n", n, comm_sz, steps, tempo, lambda);
    }

    free(A_local);
    free(x);
    free(y_local);
    free(x_local);
    MPI_Finalize();
    return 0;
}
