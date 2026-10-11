#!/bin/bash
#SBATCH --job-name=autovalor_tempo
#SBATCH --output=resultados.csv
#SBATCH --error=erros.txt
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=32
#SBATCH --time=0-1:00

echo "$SLURM_JOB_NODELIST" > nos_alocados.txt

mpirun -np 1 ./autovalor_tempo 512 10 > /dev/null

echo "n,p,steps,tempo,autovalor"

for n in 2048 4096 8192 16384
do
    for steps in 10 50 100
    do
        for p in 1 2 4 8 16 32 64
        do
            mpirun -np $p ./autovalor_tempo $n $steps
        done
    done
done
