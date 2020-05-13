#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

#include "unit.h"
#include "debug.h"

#include "omp.h"

extern char *optarg;
extern int optind, opterr, optopt;

#if defined(INT)
#define TYPE int
#define FMT "%d"
#elif defined(FLOAT)
#define TYPE float
#define FMT "%f"
#else
#define TYPE double
#define FMT "%lf"
#endif

////////////////////////////////////////////////////////////////////////////////////////
/// Get wall clock time as a double
/// You may replace this with opm_get_wtime()
double wctime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + 1E-6 * tv.tv_usec;
}

int main(int argc, char *argv[])
{
    int i, N;
    int nThreads;
    int c;
    while ((c = getopt(argc, argv, "dt:")) != -1)
        switch (c)
        {
        case 'd':
            debug = 1;
            break;
        case 't':
            nThreads = atoi(optarg);
            if (nThreads > 0)
                omp_set_num_threads(nThreads);
            break;
        default:
            printf("Invalid option\n");
            abort();
        }
    argc -= optind;
    argv += optind;

    if (argc != 1)
    {
        printf("Usage: ./example N\n");
        return -1;
    }

    srand48(time(NULL));
    srand48(time(NULL));

    N = atol(argv[0]);

    printf("Initializing SRC array\n");
    TYPE *src = malloc(sizeof(*src) * N);
    for (i = 0; i < N; i++)
        src[i] = drand48();
    printf("Done!\n");

    printDouble(src, N, "SRC");
    if (debug)
        printf("\n\n");

    int testFunctions[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0};
    // int testFunctions[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
    for (int i = 0; i < nTestFunction; i++)
    {
        if (testFunctions[i] == 0)
            continue;

        double start = omp_get_wtime();
        testFunction[i](src, N, sizeof(*src));
        double end = omp_get_wtime();
        printf("%s:\t%6.5lf seconds\n", testNames[i], end - start);
        if (debug)
            printf("\n\n");
    }

    free(src);
    return 0;
}
