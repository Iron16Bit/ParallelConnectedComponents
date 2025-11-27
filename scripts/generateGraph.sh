#!/bin/bash
#PBS -l select=1:ncpus=4:mem=2gb
#PBS -l walltime=0:10:00
#PBS -q short_cpuQ

./ParallelConnectedComponents/utils/generator.out 1000000 209523
