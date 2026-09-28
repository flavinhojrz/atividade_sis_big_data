/*
 * Regra do trapézio paralela com MPI (MPI_Send / MPI_Recv)
 * Integral de f(x) = x^2 no intervalo [a, b] = [0, 3]  ->  valor exato = 9
 *
 * Compilar: mpicc trapezio_mpi.c -o trapezio_mpi
 * Executar: mpirun -np 4 ./trapezio_mpi          (roda vários valores de n)
 *           mpirun -np 4 ./trapezio_mpi 1000     (roda só n = 1000)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

double f(double x) {
    return x * x;
}

/* Soma a área dos n_local trapézios entre a_local e b_local */
double trapezio(double a_local, double b_local, long n_local, double h) {
    double soma = (f(a_local) + f(b_local)) / 2.0;
    for (long i = 1; i < n_local; i++)
        soma += f(a_local + i * h);
    return soma * h;
}

/* Cada processo calcula o seu pedaço; o processo 0 junta tudo */
double integral_paralela(double a, double b, long n, int rank, int size) {
    double h = (b - a) / n;          /* largura de cada trapézio */
    long n_local = n / size;         /* trapézios por processo */
    double a_local = a + rank * n_local * h;
    double b_local = a_local + n_local * h;

    /* O último processo fica com as "sobras" se n não for divisível por size */
    if (rank == size - 1) {
        n_local = n - rank * (n / size);
        b_local = b;
    }

    double integral_local = trapezio(a_local, b_local, n_local, h);

    if (rank != 0) {
        /* Processos 1, 2, 3...: enviam seu resultado para o processo 0 */
        MPI_Send(&integral_local, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        return 0.0;
    }

    /* Processo 0: soma o próprio pedaço com o que recebe dos outros */
    double total = integral_local;
    for (int origem = 1; origem < size; origem++) {
        double parcial;
        MPI_Recv(&parcial, 1, MPI_DOUBLE, origem, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        total += parcial;
    }
    return total;
}

int main(int argc, char *argv[]) {
    int rank, size, len;
    char nome_no[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* "número" deste processo */
    MPI_Comm_size(MPI_COMM_WORLD, &size);   /* total de processos */
    MPI_Get_processor_name(nome_no, &len);

    /* Mostra em qual nó cada processo está rodando */
    printf("Processo %d de %d rodando no nó %s\n", rank, size, nome_no);
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);

    double a = 0.0, b = 3.0;
    double exato = (b * b * b - a * a * a) / 3.0;   /* primitiva x^3/3 */

    long lista_n[] = {4, 8, 16, 100, 1000, 10000, 100000, 1000000, 10000000};
    int qtd = sizeof(lista_n) / sizeof(lista_n[0]);
    if (argc > 1) {                  /* se passar n na linha de comando, usa só ele */
        lista_n[0] = atol(argv[1]);
        qtd = 1;
    }

    if (rank == 0)
        printf("\n%10s  %18s  %8s  %12s  %10s\n",
               "n", "estimado", "exato", "erro", "tempo(s)");

    for (int k = 0; k < qtd; k++) {
        long n = lista_n[k];
        double t0 = MPI_Wtime();
        double integral = integral_paralela(a, b, n, rank, size);
        double t1 = MPI_Wtime();

        if (rank == 0)
            printf("%10ld  %18.12f  %8.4f  %12.3e  %10.6f\n",
                   n, integral, exato, fabs(integral - exato), t1 - t0);
    }

    MPI_Finalize();
    return 0;
}
