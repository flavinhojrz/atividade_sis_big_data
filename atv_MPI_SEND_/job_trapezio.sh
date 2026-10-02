#!/bin/bash
#SBATCH --job-name=trapezio
#SBATCH --output=saida_%j.out
#SBATCH --error=erro_%j.err
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --time=0-0:5

for n in 4 8 16 100 1000 10000 100000 1000000 10000000
do
    echo "===== n = $n ====="
    mpirun ./trapezio_mpi $n
done
