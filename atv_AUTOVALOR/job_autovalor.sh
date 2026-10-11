#!/bin/bash
#SBATCH --job-name=autovalor
#SBATCH --output=resultados_autovalor.txt
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --time=0-0:5

echo "Nos alocados: $SLURM_JOB_NODELIST"
echo ""

mpirun ./autovalor_mpi 8
mpirun ./autovalor_mpi 1000
