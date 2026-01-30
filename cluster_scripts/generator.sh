#!/bin/bash

#PBS -l select=1:ncpus=8:mem=2gb

#PBS -l walltime=0:30:00

#PBS -q shortCPUQ

./ParallelConnectedComponents/utils/generator.out 100000 74632