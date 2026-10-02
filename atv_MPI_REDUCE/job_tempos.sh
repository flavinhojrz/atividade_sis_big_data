#!/bin/bash
#SBATCH --job-name=trap_tempos
#SBATCH --output=saida_%j.out
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=32
#SBATCH --time=0-0:30

TIMEFORMAT="tempo = %R s"
echo "Nos alocados: $SLURM_JOB_NODELIST"

for n in 1000000 10000000 100000000 1000000000 10000000000
do
    for p in 1 2 4 8 16 32 64
    do
        if [ $p -le 32 ]; then nos=1; else nos=2; fi
        echo "----- n = $n | p = $p | nos = $nos -----"
        time srun -N $nos -n $p ./trapezio_send $n
        time srun -N $nos -n $p ./trapezio_reduce $n
    done
done
