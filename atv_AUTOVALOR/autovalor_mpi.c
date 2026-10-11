#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

int main(int argc, char* argv[]){
    int comm_sz;
    int my_rank;

    int n = 8;
    if(argc > 1) n = atoi(argv[1]);
    int max_iter = 1000;
    double tolerancia = 1e-12;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if(n % comm_sz != 0){
        if(my_rank == 0) printf("Erro: n = %d precisa ser divisivel por p = %d\n", n, comm_sz);
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
    double lambda_antigo = 0.0;
    int iter;

    for(iter=1; iter<=max_iter; iter++){
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

        if(fabs(lambda - lambda_antigo) < tolerancia * fabs(lambda)) break;
        lambda_antigo = lambda;
    }

    if(my_rank == 0){
        double exato = n + 1;
        double v_exato = 1.0 / sqrt(n);
        double erro_vetor = 0.0;
        for(int i=0; i<n; i++){
            double e = fabs(x[i] - v_exato);
            if(e > erro_vetor) erro_vetor = e;
        }

        printf("Tamanho da matriz: %d x %d\n", n, n);
        printf("Processos: %d (%d linhas para cada)\n", comm_sz, linhas);
        printf("Iteracoes: %d\n", iter);
        printf("Autovalor calculado: %.10f\n", lambda);
        printf("Autovalor exato:     %.10f\n", exato);
        printf("Erro do autovalor:   %.3e\n", fabs(lambda - exato));
        printf("Erro do autovetor:   %.3e\n", erro_vetor);
        if(n <= 10){
            printf("Autovetor: (");
            for(int i=0; i<n; i++){
                printf("%.6f", x[i]);
                if(i < n-1) printf(", ");
            }
            printf(")\n");
        } else {
            printf("Autovetor (primeiros 5): (%.6f, %.6f, %.6f, %.6f, %.6f, ...)\n",
                   x[0], x[1], x[2], x[3], x[4]);
        }
        printf("Valor certo de cada posicao do autovetor: 1/raiz(%d) = %.6f\n", n, v_exato);
        printf("\n");
    }

    free(A_local);
    free(x);
    free(y_local);
    free(x_local);
    MPI_Finalize();
    return 0;
}
