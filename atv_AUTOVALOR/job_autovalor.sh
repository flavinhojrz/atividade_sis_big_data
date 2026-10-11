#!/bin/bash
#SBATCH --job-name=autovalor
#SBATCH --output=resultados_autovalor.txt
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=32
#SBATCH --time=0-0:30

srun -N 1 -n 1 ./autovalor_mpi 100 > /dev/null 2>&1

echo "Nos alocados: $SLURM_JOB_NODELIST"
echo ""
printf "%-8s %-4s %-6s %-20s %-10s %-12s %-12s %-10s\n" "n" "p" "iter" "autovalor" "exato" "erro" "erro vetor" "tempo (s)"

for n in 1000 10000 20000 40000
do
    for p in 1 2 4 8 16 32 64
    do
        if [ $p -le 32 ]; then nos=1; else nos=2; fi
        srun -N $nos -n $p ./autovalor_mpi $n
    done
    echo ""
done
