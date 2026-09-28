# Integral numérica com MPI: regra do trapézio

## 1. Função e limites escolhidos

- **Função:** f(x) = x²
- **Limites:** a = 0, b = 3

## 2. Integral analítica (o valor "exato")

A primitiva de x² é x³/3. Então:

```
∫₀³ x² dx = [x³/3]₀³ = 3³/3 − 0³/3 = 27/3 − 0 = 9
```

**Valor exato = 9**

## 3. Regra do trapézio: a ideia

Dividimos o intervalo [a, b] em **n** fatias de mesma largura:

```
h = (b − a) / n
```

Cada fatia vira um trapézio. A área de um trapézio é `(base maior + base menor) / 2 × altura`.
Aqui as "bases" são f(xᵢ) e f(xᵢ₊₁), e a "altura" é h:

```
Área de um trapézio = h × [f(xᵢ) + f(xᵢ₊₁)] / 2
```

Somando todos os trapézios, os pontos do meio aparecem duas vezes (são o fim de um e o
início do próximo), então a fórmula fica:

```
Integral ≈ h × [ f(x₀)/2 + f(x₁) + f(x₂) + ... + f(xₙ₋₁) + f(xₙ)/2 ]
```

### Exemplo à mão com n = 4

h = (3 − 0)/4 = 0,75 → pontos: 0; 0,75; 1,5; 2,25; 3

| x    | f(x) = x² |
|------|-----------|
| 0    | 0         |
| 0,75 | 0,5625    |
| 1,5  | 2,25      |
| 2,25 | 5,0625    |
| 3    | 9         |

```
Integral ≈ 0,75 × [ 0/2 + 0,5625 + 2,25 + 5,0625 + 9/2 ]
         = 0,75 × [ 0 + 7,875 + 4,5 ]
         = 0,75 × 12,375
         = 9,28125
```

Erro = 9,28125 − 9 = 0,28125. É exatamente o valor que o programa imprime para n = 4.

## 4. Como o MPI divide o trabalho

Com **p** processos, cada um fica com `n/p` trapézios consecutivos:

```
Intervalo [0, 3], n = 8, p = 4  →  h = 0,375, 2 trapézios por processo

 processo 0     processo 1     processo 2     processo 3
[0 ... 0,75]  [0,75 ... 1,5]  [1,5 ... 2,25]  [2,25 ... 3]
```

1. Todos os processos rodam o **mesmo programa**; o que muda é o `rank` (0, 1, 2, 3).
2. Cada processo usa seu `rank` para descobrir o seu pedaço: `a_local = a + rank × n_local × h`.
3. Cada um calcula a regra do trapézio **só no seu pedaço** (em paralelo).
4. Os processos 1, 2, 3... enviam seu resultado com `MPI_Send`. O processo 0 recebe cada um
   com `MPI_Recv`, soma tudo e imprime.

Se n não for divisível por p, o último processo pega os trapézios que sobrarem.

## 5. Código desenvolvido

Arquivo: [`trapezio_mpi.c`](trapezio_mpi.c) (C + MPI)

Os processos 1, 2, 3... mandam o resultado parcial com `MPI_Send`. O processo 0 recebe cada
um com `MPI_Recv` e soma. O programa roda vários valores de n de uma vez, então um único job
gera a tabela inteira.

```c
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
```

### Execução no supercomputador NPAD (UFRN)

O NPAD usa o gerenciador de filas **SLURM**. Você não roda o programa direto: escreve um
script dizendo quantos nós e processos quer, e o SLURM executa quando houver máquinas livres.

**1. Conectar (no seu computador):**

```bash
ssh -p 4422 SEU_USUARIO@sc2.npad.ufrn.br
```

**2. Enviar os arquivos para a pasta `atv_MPI_SEND_` (em outro terminal, no seu computador):**

```bash
scp -P 4422 trapezio_mpi.c job_trapezio.sh SEU_USUARIO@sc2.npad.ufrn.br:~/atv_MPI_SEND_/
```

**3. Compilar (já no NPAD):**

```bash
cd ~/atv_MPI_SEND_
mpicc trapezio_mpi.c -o trapezio_mpi -lm
```

**4. Script de submissão** (`job_trapezio.sh`): 2 nós × 4 processos por nó = 8 processos.

```bash
#!/bin/bash
#SBATCH --job-name=trapezio
#SBATCH --output=saida_%j.out
#SBATCH --error=erro_%j.err
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --time=0-0:5

mpirun ./trapezio_mpi
```

**5. Submeter e acompanhar:**

```bash
sbatch job_trapezio.sh      # responde: Submitted batch job 12345
squeue -u $USER             # PD = na fila, R = rodando; sumiu = terminou
cat saida_12345.out         # resultado
```

## 6. Resultados: analítico × estimado

Teste local: **1 nó computacional** (host `vm`) com **4 processos por nó** (4 processos
MPI no total). No NPAD, o script pede **2 nós com 4 processos por nó** (8 processos). Os
valores são os mesmos, porque dividir o trabalho não muda a conta. Depois de rodar no NPAD,
troque esta linha pelos nomes dos nós que aparecem no início de `saida_JOBID.out`.

| n (trapézios) | Integral estimada | Integral exata | Erro absoluto |
|--------------:|------------------:|---------------:|--------------:|
| 4             | 9,281250000000    | 9              | 2,81 × 10⁻¹   |
| 8             | 9,070312500000    | 9              | 7,03 × 10⁻²   |
| 16            | 9,017578125000    | 9              | 1,76 × 10⁻²   |
| 100           | 9,000450000000    | 9              | 4,50 × 10⁻⁴   |
| 1 000         | 9,000004500000    | 9              | 4,50 × 10⁻⁶   |
| 10 000        | 9,000000045000    | 9              | 4,50 × 10⁻⁸   |
| 100 000       | 9,000000000450    | 9              | 4,50 × 10⁻¹⁰  |
| 1 000 000     | 9,000000000005    | 9              | 4,56 × 10⁻¹²  |
| 10 000 000    | 9,000000000000    | 9              | 3,6 × 10⁻¹⁵   |

Com n = 1000, rodar com 1, 3 ou 4 processos dá **o mesmo resultado** (9,0000045): dividir o
trabalho não muda a conta, só quem a faz.

## 7. Análise

- **Quanto maior n, menor o erro.** Quando n é multiplicado por 10, o erro cai 100 vezes
  (de 4,5×10⁻⁴ para 4,5×10⁻⁶, e assim por diante). O erro é proporcional a 1/n².
- **Por que sempre sobra?** A parábola x² é "curvada para cima". A reta de cada trapézio
  fica acima da curva, então a estimativa é sempre um pouco maior que 9.
- **Fórmula do erro:** para a regra do trapézio, `erro = (b − a) × h² × f''/12`. Com
  f''(x) = 2, b − a = 3 e h = 3/n:

  ```
  erro = 3 × (3/n)² × 2 / 12 = 4,5 / n²
  ```

  Para n = 100: 4,5/10 000 = 0,00045, que é o valor da tabela.
- A partir de n = 1 000 000 o erro deixa de seguir 4,5/n² (em n = 10⁷ o esperado seria
  4,5×10⁻¹⁴, mas deu 3,6×10⁻¹⁵). Aqui o erro do método já é tão pequeno quanto o
  arredondamento dos números `double` do computador, que passa a dominar.
