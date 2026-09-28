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
4. `comm.reduce(..., op=MPI.SUM, root=0)` soma os resultados parciais no processo 0,
   que imprime o resultado.

Se n não for divisível por p, o último processo pega os trapézios que sobrarem.

## 5. Código desenvolvido

Arquivo: [`trapezio_mpi.py`](trapezio_mpi.py) (Python + mpi4py)

```python
import sys
from mpi4py import MPI


def f(x):
    return x * x


def trapezio(a_local, b_local, n_local, h):
    """Soma a área dos n_local trapézios entre a_local e b_local."""
    soma = (f(a_local) + f(b_local)) / 2.0
    for i in range(1, n_local):
        soma += f(a_local + i * h)
    return soma * h


comm = MPI.COMM_WORLD
rank = comm.Get_rank()   # "número" deste processo (0, 1, 2, ...)
size = comm.Get_size()   # quantos processos existem no total

a, b = 0.0, 3.0
n = int(sys.argv[1]) if len(sys.argv) > 1 else 1000   # total de trapézios

h = (b - a) / n            # largura de cada trapézio (igual para todos)
n_local = n // size        # quantos trapézios cada processo calcula
a_local = a + rank * n_local * h
b_local = a_local + n_local * h

# O último processo fica com as "sobras" caso n não seja divisível por size
if rank == size - 1:
    n_local = n - rank * (n // size)
    b_local = b

t0 = MPI.Wtime()
integral_local = trapezio(a_local, b_local, n_local, h)

# Junta (soma) os resultados parciais de todos os processos no processo 0
integral = comm.reduce(integral_local, op=MPI.SUM, root=0)
t1 = MPI.Wtime()

if rank == 0:
    exato = (b**3 - a**3) / 3.0
    erro = abs(integral - exato)
    nome_no = MPI.Get_processor_name()
    print(f"nó={nome_no} processos={size} n={n} "
          f"estimado={integral:.12f} exato={exato:.12f} "
          f"erro={erro:.3e} tempo={t1 - t0:.4f}s")
```

### Como executar

```bash
pip install mpi4py                       # precisa de uma implementação MPI (ex.: OpenMPI)
mpirun -np 4 python3 trapezio_mpi.py 1000
```

## 6. Resultados: analítico × estimado

Ambiente: **1 nó computacional** (host `vm`, 4 núcleos) com **4 processos por nó**
(4 processos MPI no total).

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

Com n = 1000, rodar com 1, 2 ou 4 processos dá **o mesmo resultado** (9,0000045): dividir o
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
- Em n = 1 000 000 o erro esperado seria 4,5×10⁻¹², mas deu 4,56×10⁻¹². A diferença vem
  do arredondamento dos números de ponto flutuante do computador, não do método.
