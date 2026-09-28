# Integral com MPI usando a regra do trapézio

## O que a atividade pede

1. Escolher uma função que dá pra integrar.
2. Escolher os limites **a** e **b**.
3. Fazer um código em C com MPI que calcula essa integral pela **regra do trapézio**,
   dividindo o trabalho entre vários processos.
4. Comparar o valor exato (feito na mão) com o valor que o programa calcula, usando
   vários valores de **n** (quantidade de trapézios), e dizer quantos nós e quantos
   processos por nó foram usados.

---

## 1. A função e os limites

- **Função:** f(x) = x²
- **De onde até onde:** a = 0 e b = 3

Escolhi x² porque é fácil de integrar na mão, então dá pra saber a resposta certa e
conferir se o programa está acertando.

---

## 2. Resolvendo a integral na mão

### O que a integral significa

A integral de 0 até 3 de x² é a **área embaixo da curva** y = x², entre x = 0 e x = 3.
É isso que a gente quer descobrir.

```
 y
 9 |                 *
   |               * |
   |             *   |
 4 |          *      |
   |       *  ÁREA   |
 1 |    *            |
   |*________________|___ x
   0    1    2    3
```

### Passo 1: achar a primitiva

A regra para integrar uma potência é:

> **soma 1 no expoente e divide pelo expoente novo.**

Para x²: o expoente é 2, somando 1 vira 3, e divide por 3:

```
x²  →  x³ / 3
```

(Dá pra conferir derivando: a derivada de x³/3 é 3x²/3 = x². Voltou pra x², então está certo.)

### Passo 2: colocar os limites

Agora é só calcular a primitiva no limite de cima (b = 3) e subtrair ela no limite de
baixo (a = 0):

```
∫₀³ x² dx = (3³ / 3) − (0³ / 3)
          = (27 / 3) − (0 / 3)
          =    9     −    0
          =    9
```

**A resposta certa é 9.** Esse é o número que o programa tem que chegar perto.

---

## 3. Como funciona a regra do trapézio

O computador não sabe fazer "primitiva". Então a gente faz um truque: corta a área em
várias fatias finas e troca cada fatia por um **trapézio**, que é uma figura que a gente
sabe calcular a área.

### A área de um trapézio

```
     f(x₁)
       |\
       | \
f(x₀)  |  |      área = (lado esquerdo + lado direito) / 2 × largura
  |\   |  |           = (f(x₀) + f(x₁)) / 2 × h
  | \__|__|
  x₀   x₁
  <-h->
```

- Os "lados" do trapézio são a altura da curva em cada ponta: f(x₀) e f(x₁).
- A largura de cada fatia é **h**.

### A largura h

Se eu quero cortar o intervalo de 0 até 3 em **n** fatias iguais:

```
h = (b − a) / n
```

### Somando todos os trapézios

Somando a área de todos, os pontos do meio aparecem **duas vezes** (um ponto é o fim de
um trapézio e o começo do próximo). Por isso os do meio entram inteiros e as duas pontas
entram pela metade:

```
Integral ≈ h × [ f(x₀)/2 + f(x₁) + f(x₂) + ... + f(xₙ₋₁) + f(xₙ)/2 ]
```

### Fazendo na mão com n = 4

**Largura:** h = (3 − 0) / 4 = **0,75**

**Os pontos:** começa no 0 e vai somando 0,75:

| ponto | x    | f(x) = x² |
|-------|------|-----------|
| x₀    | 0    | 0         |
| x₁    | 0,75 | 0,5625    |
| x₂    | 1,5  | 2,25      |
| x₃    | 2,25 | 5,0625    |
| x₄    | 3    | 9         |

**As pontas pela metade:** (0 + 9) / 2 = **4,5**

**Os do meio inteiros:** 0,5625 + 2,25 + 5,0625 = **7,875**

**Junta tudo e multiplica por h:**

```
Integral ≈ 0,75 × (4,5 + 7,875)
         = 0,75 × 12,375
         = 9,28125
```

Deu **9,28125**, e o certo é **9**. Errou por 0,28125. Com só 4 trapézios, tudo bem.
Se usar mais trapézios, o erro vai caindo (veja a tabela lá embaixo).

---

## 4. Onde entra o MPI

A ideia do MPI é: **em vez de um processo fazer tudo, vários processos dividem o trabalho.**

Cada processo pega um pedaço do intervalo, calcula a regra do trapézio só no pedaço dele,
e depois todo mundo manda o resultado pro processo 0, que soma tudo.

Exemplo com 4 processos e n = 8 (h = 0,375, então 2 trapézios pra cada):

```
 processo 0      processo 1      processo 2      processo 3
[0 ... 0,75]   [0,75 ... 1,5]  [1,5 ... 2,25]  [2,25 ... 3]
     |               |               |               |
     |     MPI_Send  |     MPI_Send  |     MPI_Send  |
     |<--------------+---------------+---------------+
     |
  processo 0 recebe com MPI_Recv, soma tudo e imprime
```

Funciona porque a área total é a soma das áreas dos pedaços.

---

## 5. O código

O código ([`trapezio_mpi.c`](trapezio_mpi.c)) foi escrito **em cima do código de saudação
que o professor mandou**. A parte da saudação continua lá, e a regra do trapézio foi
adicionada usando a mesma ideia de `MPI_Send` e `MPI_Recv`.

```c
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
```

### Explicando por partes

**Os `#include`**: trazem as ferramentas que o código usa.
`stdio.h` é pro `printf`, `stdlib.h` pro `atol` (transforma texto em número),
`string.h` pro `strlen`, `math.h` pro `fabs` (valor absoluto) e `mpi.h` pro MPI.

**A função `f(x)`**: é a nossa função x². Recebe um x e devolve x vezes x.
Se um dia quiser integrar outra função, é só mudar essa linha.

**A função `trapezio(...)`**: é a regra do trapézio feita no computador, igualzinho à
conta na mão:

1. `soma = (f(a_local) + f(b_local)) / 2.0;`: começa com as duas pontas pela metade.
2. O `for` passa pelos pontos do meio (i = 1, 2, ..., n_local − 1) e soma f de cada um.
   O ponto número i fica em `a_local + i*h`, que é andar i passos de tamanho h a partir do começo.
3. `return soma * h;`: no final multiplica tudo pela largura h.

Pra ver que é a mesma coisa da conta na mão, com a = 0, b = 3, n = 4 e h = 0,75:

| passo            | o que acontece             | soma    |
|------------------|----------------------------|---------|
| começo           | (f(0) + f(3)) / 2 = 4,5    | 4,5     |
| i = 1            | soma f(0,75) = 0,5625      | 5,0625  |
| i = 2            | soma f(1,5) = 2,25         | 7,3125  |
| i = 3            | soma f(2,25) = 5,0625      | 12,375  |
| final            | 12,375 × 0,75              | **9,28125** |

**O começo do `main`**: MPI_Init liga o MPI. Depois:
- `MPI_Comm_size` descobre **quantos processos** estão rodando e guarda em `comm_sz`.
- `MPI_Comm_rank` descobre **qual é o número deste processo** (0, 1, 2...) e guarda em `my_rank`.
- `MPI_Get_processor_name` descobre **em qual nó (computador)** o processo está.

Todos os processos rodam **o mesmo código**. O que faz cada um se comportar diferente é o
`my_rank`.

**Parte 1: a saudação.** É o código do professor. Quem não é o processo 0 manda uma
mensagem de texto (`MPI_CHAR`) pro processo 0, e o 0 recebe uma por uma e imprime. Só
acrescentei o nome do nó na mensagem, pra saber onde cada processo rodou.

**Parte 2: a divisão do trabalho.** Cada processo calcula o seu pedaço:

- `h`: largura de cada trapézio. É igual pra todo mundo.
- `n_local = n / comm_sz`: quantos trapézios cada processo faz. Se são 1000 trapézios e
  8 processos, cada um faz 125.
- `a_local = a + my_rank * n_local * h`: onde começa o pedaço deste processo. O processo 0
  começa no 0, o processo 1 começa depois dos 125 trapézios do processo 0, e assim por diante.
- `b_local = a_local + n_local * h`: onde o pedaço termina (começo + tamanho do pedaço).

**E se a divisão não for exata?** Com n = 100 e 8 processos, 100 / 8 dá 12 (em C, divisão
de inteiros joga fora o que sobra). 8 × 12 = 96, então sobram 4 trapézios. Por isso o
`if(my_rank == comm_sz - 1)`: o último processo pega o que sobrou (100 − 7 × 12 = 16
trapézios) e vai até o b, pra não deixar nenhum pedaço de fora.

**Juntando os resultados.** Mesma ideia da saudação, mas agora a mensagem é um número:
- Processos 1, 2, 3...: `MPI_Send(&integral_local, 1, MPI_DOUBLE, 0, ...)` quer dizer
  "manda **1** número do tipo **double** pro processo **0**".
- Processo 0: começa com o próprio pedaço em `total` e, no `for`, recebe com `MPI_Recv` o
  pedaço de cada processo e vai somando.

**O final.** O processo 0 calcula o valor exato com a primitiva x³/3 (a mesma conta feita
na mão), compara com o total e imprime o erro. `MPI_Finalize` desliga o MPI.

### Resumo das variáveis

| variável         | o que guarda                                    | por que existe |
|------------------|-------------------------------------------------|----------------|
| `a`, `b`         | começo e fim da integral (0 e 3)                | são os limites escolhidos |
| `n`              | total de trapézios                              | quanto maior, mais preciso; vem da linha de comando |
| `h`              | largura de cada trapézio                        | usada pra achar os pontos e pra multiplicar no final |
| `comm_sz`        | quantos processos existem                       | pra saber em quantos pedaços dividir |
| `my_rank`        | o número deste processo                         | pra cada um saber qual pedaço é o seu |
| `n_local`        | quantos trapézios este processo faz             | é a parte do trabalho de cada um |
| `a_local`, `b_local` | começo e fim do pedaço deste processo       | cada um só calcula no seu pedaço |
| `integral_local` | a área do pedaço deste processo                 | é o que cada um manda pro processo 0 |
| `total`          | a soma de todos os pedaços (só no processo 0)   | é a resposta final |
| `saudacao`, `nome_no` | a mensagem e o nome do nó                  | pra mostrar onde cada processo rodou |

---

## 6. Como rodar no NPAD

O NPAD é o supercomputador da UFRN. Lá quem manda rodar os programas é o **SLURM**: você
escreve um script dizendo quantos nós e processos quer, e ele roda quando tiver máquina livre.

O script [`job_trapezio.sh`](job_trapezio.sh):

```bash
#!/bin/bash
#SBATCH --job-name=trapezio
#SBATCH --output=saida_%j.out
#SBATCH --error=erro_%j.err
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --time=0-0:5

# Roda o programa uma vez para cada valor de n (numero de trapezios)
for n in 4 8 16 100 1000 10000 100000 1000000 10000000
do
    echo "===== n = $n ====="
    mpirun ./trapezio_mpi $n
done
```

- `--nodes=2`: quero 2 nós (2 computadores).
- `--ntasks-per-node=4`: 4 processos em cada nó. Dá 8 processos no total.
- `--time=0-0:5`: pode rodar por no máximo 5 minutos.
- O `for` roda o programa uma vez pra cada valor de n.

Os comandos:

```bash
# no seu computador: manda os arquivos pro NPAD
scp trapezio_mpi.c job_trapezio.sh npad:~/atv_MPI_SEND_/

# entra no NPAD
ssh npad
cd ~/atv_MPI_SEND_

# compila (-lm liga a biblioteca de matemática)
mpicc trapezio_mpi.c -o trapezio_mpi -lm

# manda pra fila e acompanha
sbatch job_trapezio.sh
squeue -u $USER
cat saida_*.out
```

---

## 7. Resultados

Rodou no NPAD (job 2132011, partição `amd-512`):

- **Nós computacionais:** 2 (`r2n14` e `r2n46`)
- **Processos por nó:** 4 (processos 0 a 3 no `r2n14`, processos 4 a 7 no `r2n46`)
- **Total:** 8 processos

| n (trapézios) | Valor calculado  | Valor exato | Erro          |
|--------------:|-----------------:|------------:|--------------:|
| 4             | 9,281250000000   | 9           | 2,81 × 10⁻¹   |
| 8             | 9,070312500000   | 9           | 7,03 × 10⁻²   |
| 16            | 9,017578125000   | 9           | 1,76 × 10⁻²   |
| 100           | 9,000450000000   | 9           | 4,50 × 10⁻⁴   |
| 1 000         | 9,000004500000   | 9           | 4,50 × 10⁻⁶   |
| 10 000        | 9,000000045000   | 9           | 4,50 × 10⁻⁸   |
| 100 000       | 9,000000000450   | 9           | 4,50 × 10⁻¹⁰  |
| 1 000 000     | 9,000000000004   | 9           | 4,48 × 10⁻¹²  |
| 10 000 000    | 9,000000000000   | 9           | 4,09 × 10⁻¹⁴  |

O valor com n = 4 (9,28125) é exatamente o mesmo da conta feita na mão lá em cima.

---

## 8. O que dá pra concluir

- **Mais trapézios, menos erro.** Cada vez que o n fica 10 vezes maior, o erro fica
  **100 vezes menor** (olha a coluna do erro: 4,5×10⁻⁴, 4,5×10⁻⁶, 4,5×10⁻⁸...).

- **Por que sempre passa um pouquinho de 9?** A curva x² é "curvada pra cima". Aí a linha
  reta de cima de cada trapézio fica um pouco acima da curva, e a área sai um pouquinho maior.

- **Dá pra prever o erro.** Existe uma fórmula pro erro da regra do trapézio:
  `erro = (b − a) × h² × f'' / 12`. Pra x², a segunda derivada f'' é 2. Com b − a = 3 e h = 3/n:

  ```
  erro = 3 × (3/n)² × 2 / 12 = 4,5 / n²
  ```

  Com n = 100: 4,5 / 10 000 = 0,00045. Bate certinho com a tabela.

- **Com n muito grande aparece uma diferencinha.** Em n = 1 000 000 a fórmula diz 4,5×10⁻¹²
  e deu 4,48×10⁻¹². Nesse ponto o erro já é tão pequeno que os arredondamentos do próprio
  computador começam a aparecer. Não é erro no código.

- **Dividir entre processos não muda a resposta.** Testando com 1, 3 ou 4 processos, o
  resultado pra n = 1000 foi o mesmo (9,0000045). O MPI só divide quem faz a conta, a conta
  continua a mesma.
