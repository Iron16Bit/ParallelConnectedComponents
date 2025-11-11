#define _GNU_SOURCE
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int a;
    int b;
} Edge;

static int cmp_edge(const void* x, const void* y) {
    const Edge *e1 = (const Edge*)x, *e2 = (const Edge*)y;
    if (e1->a != e2->a) return e1->a < e2->a ? -1 : 1;
    if (e1->b != e2->b) return e1->b < e2->b ? -1 : 1;
    return 0;
}

static unsigned long long splitmix64(unsigned long long* s) {
    unsigned long long z = (*s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static int rand_range(unsigned long long* state, int lo, int hi) {
    unsigned long long r = splitmix64(state);
    return lo + (int)(r % (unsigned long long)(hi - lo + 1));
}

static void shuffle(int* arr, int n, unsigned long long* state) {
    for (int i = n - 1; i > 0; --i) {
        int j = rand_range(state, 0, i);
        int t = arr[i];
        arr[i] = arr[j];
        arr[j] = t;
    }
}

static char* default_output(const char* argv0, int next_index) {
    char exe[PATH_MAX];
    if (!realpath(argv0, exe)) strcpy(exe, argv0);
    char* dirc = strdup(exe);
    char* d = dirname(dirc);
    char base[PATH_MAX];
    snprintf(base, sizeof(base), "%s/..", d);
    char* out = malloc(PATH_MAX);
    snprintf(out, PATH_MAX, "%s/data/matrix_%d.mtx", base, next_index);
    free(dirc);
    return out;
}

static int count_files_in_data(const char* argv0) {
    char exe[PATH_MAX];
    if (!realpath(argv0, exe)) strcpy(exe, argv0);
    char* dirc = strdup(exe);
    char* d = dirname(dirc);
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/../data", d);
    DIR* dir = opendir(path);
    int c = 0;
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir))) {
            if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) c++;
        }
        closedir(dir);
    }
    free(dirc);
    return c;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s num_nodes num_components [--output path]\n", argv[0]);
        return 1;
    }
    long num_nodes = strtol(argv[1], NULL, 10);
    long num_components = strtol(argv[2], NULL, 10);
    if (num_components > num_nodes || num_nodes <= 0 || num_components <= 0) {
        fprintf(stderr, "invalid arguments\n");
        return 1;
    }
    char* output = NULL;
    if (argc >= 5 && strcmp(argv[3], "--output") == 0) {
        output = strdup(argv[4]);
    } else {
        int len = count_files_in_data(argv[0]);
        output = default_output(argv[0], len + 1);
    }

    long base_size = num_nodes / num_components;
    long remainder = num_nodes % num_components;
    long* comp_sizes = (long*)malloc(sizeof(long) * num_components);
    for (long i = 0; i < num_components; ++i) comp_sizes[i] = base_size + (i < remainder);

    long* start_indices = (long*)malloc(sizeof(long) * num_components);
    long s = 0;
    for (long i = 0; i < num_components; ++i) {
        start_indices[i] = s;
        s += comp_sizes[i];
    }

    Edge** comp_edges = (Edge**)calloc(num_components, sizeof(Edge*));
    size_t* comp_counts = (size_t*)calloc(num_components, sizeof(size_t));

#pragma omp parallel for schedule(dynamic)
    for (long cid = 0; cid < num_components; ++cid) {
        long size = comp_sizes[cid];
        long start = start_indices[cid];
        int* nodes = (int*)malloc(sizeof(int) * size);
        for (long i = 0; i < size; ++i) nodes[i] = (int)(start + i);
        unsigned long long state = ((unsigned long long)cid + 1ULL) ^ ((unsigned long long)omp_get_thread_num() << 32);
        shuffle(nodes, (int)size, &state);

        size_t cap = (size_t)(2 * (size - 1) + 2 * size + 16);
        Edge* edges = (Edge*)malloc(sizeof(Edge) * cap);
        size_t cnt = 0;

        for (long i = 0; i < size - 1; ++i) {
            int a = nodes[i], b = nodes[i + 1];
            edges[cnt++] = (Edge){a, b};
            edges[cnt++] = (Edge){b, a};
        }

        int extra = rand_range(&state, 0, (int)size);
        for (int e = 0; e < extra; ++e) {
            int ia = rand_range(&state, 0, (int)size - 1);
            int ib = rand_range(&state, 0, (int)size - 1);
            if (ia == ib) {
                e--;
                continue;
            }
            int a = nodes[ia], b = nodes[ib];
            if (cnt + 2 > cap) {
                cap *= 2;
                edges = (Edge*)realloc(edges, sizeof(Edge) * cap);
            }
            edges[cnt++] = (Edge){a, b};
            edges[cnt++] = (Edge){b, a};
        }

        qsort(edges, cnt, sizeof(Edge), cmp_edge);
        size_t uniq = 0;
        for (size_t i = 0; i < cnt; ++i) {
            if (uniq == 0 || cmp_edge(&edges[i], &edges[uniq - 1]) != 0) edges[uniq++] = edges[i];
        }

        comp_edges[cid] = edges;
        comp_counts[cid] = uniq;
        free(nodes);
    }

    Edge* global = NULL;
    size_t total = 0, gcap = 0;
    for (long i = 0; i < num_components; ++i) {
        if (total + comp_counts[i] > gcap) {
            gcap = (total + comp_counts[i]) * 2 + 1024;
            global = (Edge*)realloc(global, sizeof(Edge) * gcap);
        }
        memcpy(global + total, comp_edges[i], sizeof(Edge) * comp_counts[i]);
        total += comp_counts[i];
        free(comp_edges[i]);
    }
    free(comp_edges);
    free(comp_counts);

    if (total + (size_t)num_nodes > gcap) {
        gcap = total + (size_t)num_nodes;
        global = (Edge*)realloc(global, sizeof(Edge) * gcap);
    }
    for (int i = 0; i < num_nodes; ++i) global[total++] = (Edge){i, i};

    qsort(global, total, sizeof(Edge), cmp_edge);
    size_t uniq = 0;
    for (size_t i = 0; i < total; ++i) {
        if (uniq == 0 || cmp_edge(&global[i], &global[uniq - 1]) != 0) global[uniq++] = global[i];
    }

    FILE* f = fopen(output, "w");
    if (!f) {
        fprintf(stderr, "cannot open output\n");
        free(global);
        free(output);
        free(comp_sizes);
        free(start_indices);
        return 1;
    }
    fprintf(f, "%% Graph with %ld CCs\n", num_components);
    fprintf(f, "%ld %ld %zu\n", num_nodes, num_nodes, uniq);
    for (size_t i = 0; i < uniq; ++i) fprintf(f, "%d %d\n", global[i].a, global[i].b);
    fclose(f);

    free(global);
    free(output);
    free(comp_sizes);
    free(start_indices);
    return 0;
}
