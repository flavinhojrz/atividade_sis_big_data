#!/bin/bash
#SBATCH --job-name=trap_tempos
#SBATCH --output=tabela_tempos.txt
#SBATCH --partition=amd-512
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=32
#SBATCH --time=0-0:30

TIMEFORMAT="%R"
rm -f resultados_programa.txt
srun -N 1 -n 1 ./trapezio_send 1000 > /dev/null 2>&1

echo "Nos alocados: $SLURM_JOB_NODELIST"
echo ""
printf "%-14s %-5s %-5s %-12s %-12s\n" "n" "p" "nos" "send (s)" "reduce (s)"

for n in 1000000 10000000 100000000 1000000000 10000000000
do
    for p in 1 2 4 8 16 32 64
    do
        if [ $p -le 32 ]; then nos=1; else nos=2; fi
        t_send=$( { time srun -N $nos -n $p ./trapezio_send $n >> resultados_programa.txt 2>&1; } 2>&1 )
        t_reduce=$( { time srun -N $nos -n $p ./trapezio_reduce $n >> resultados_programa.txt 2>&1; } 2>&1 )
        printf "%-14s %-5s %-5s %-12s %-12s\n" $n $p $nos $t_send $t_reduce
    done
    echo ""
done
