# Parallel Connected Components

## Checker

Simple pyhton script that uses the networkx library to compute the connected components of a graph. Used for checking correctness. Needs `networkx` to be installed.

## Dataset

Graphs come from the (SuiteSparse Matrix Collection)[https://sparse.tamu.edu/], in particular we have selected:

- https://sparse.tamu.edu/Barabasi/NotreDame_yeast
- https://sparse.tamu.edu/Pajek/geom
- https://sparse.tamu.edu/Pajek/dictionary28
- https://sparse.tamu.edu/LAW/dblp-2010
- https://sparse.tamu.edu/SNAP/as-Skitter

To look for matrices: filter them by putting a high number of highly connected components (SuiteSparrse only has **highly** connected components) and set their pattern to symmetric to only obtain undirected graphs.

Run `setup.sh` to prepare the dataset for use.