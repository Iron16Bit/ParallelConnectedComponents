#!/bin/bash
#PBS -l select=1:ncpus=1:mem=2gb
#PBS -l walltime=0:15:00
#PBS -q short_cpuQ

./ParallelConnectedComponents/out/sequentialSV.out ./ParallelConnectedComponents/data/matrix_1.mtx
