import os
import random
import concurrent.futures
from typing import Tuple, Set

BASE_PATH = f"{os.path.dirname(os.path.realpath(__file__))}/.."

def generate_component(component_size: int, start_index: int) -> Set[Tuple[int, int]]:
    edges = set()
    nodes = list(range(start_index, start_index + component_size))
    random.shuffle(nodes)

    # Ensure connectivity within the component
    for i in range(component_size - 1):
        a, b = nodes[i], nodes[i + 1]
        edges.add((a, b))
        edges.add((b, a))

    # Add random extra edges
    extra_edges = random.randint(0, component_size)
    for _ in range(extra_edges):
        a, b = random.sample(nodes, 2)
        edges.add((a, b))
        edges.add((b, a))

    return edges

def generate_graph(num_nodes: int, num_components: int) -> Set[Tuple[int, int]]:
    if num_components > num_nodes:
        raise ValueError("Number of components cannot exceed number of nodes")

    base_size = num_nodes // num_components
    remainder = num_nodes % num_components
    component_sizes = [base_size + (1 if i < remainder else 0) for i in range(num_components)]

    edges = set()
    start_index = 0

    with concurrent.futures.ThreadPoolExecutor() as executor:
        futures = []
        for size in component_sizes:
            futures.append(executor.submit(generate_component, size, start_index))
            start_index += size

        for future in concurrent.futures.as_completed(futures):
            edges.update(future.result())

    # Add self-loops for all nodes
    for i in range(num_nodes):
        edges.add((i, i))

    return edges

def write_adjacency_file(edges: Set[Tuple[int, int]], num_nodes: int, filename: str, num_cc: int):
    with open(filename, 'w') as f:
        f.write(f"% Graph with {num_cc} CCs\n")
        f.write(f"{num_nodes} {num_nodes} {len(edges)}\n")
        for a, b in sorted(edges):
            f.write(f"{a} {b}\n")

if __name__ == "__main__":
    import argparse
    
    _, _, files = next(os.walk(BASE_PATH+"/data"))
    lenght = len(files)

    parser = argparse.ArgumentParser(description="Generate undirected graph with connected components and self-loops")
    parser.add_argument("num_nodes", type=int, help="Total number of nodes")
    parser.add_argument("num_components", type=int, help="Number of connected components")
    parser.add_argument("--output", type=str, default=f"{BASE_PATH}/data/matrix_{lenght+1}.mtx", help="Output file name")

    args = parser.parse_args()

    edges = generate_graph(args.num_nodes, args.num_components)
    write_adjacency_file(edges, args.num_nodes, args.output, args.num_components)
    print(f"Graph with {args.num_nodes} nodes and {args.num_components} components written to {args.output}")
