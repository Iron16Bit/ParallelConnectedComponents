#!/bin/bash

#PBS -l select=1:ncpus=1:mem=12gb

#PBS -l walltime=0:15:00

#PBS -q short_cpuQ

module load OpenMPI
mpirun.actual -n 64 ./ParallelConnectedComponents/out/parallelSV.out