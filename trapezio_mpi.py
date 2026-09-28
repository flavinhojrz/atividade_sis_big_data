# Regra do trapézio paralela com MPI (mpi4py)
# Integral de f(x) = x^2 no intervalo [a, b] = [0, 3]
# Execução: mpirun -np 4 python3 trapezio_mpi.py 1000

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
