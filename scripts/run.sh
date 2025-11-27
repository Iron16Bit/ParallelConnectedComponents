#!/bin/bash
#PBS -l select=1:ncpus=8:mem=2gb
#PBS -l walltime=0:15:00
#PBS -q short_cpuQ

module load mpich-3.2
mpiexec -n 8 ./ParallelConnectedComponents/out/parallelSV.out ./ParallelConnectedComponents/data/matrix_1.mtx 10
