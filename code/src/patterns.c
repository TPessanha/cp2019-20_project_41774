#include <string.h>
#include <assert.h>
#include "patterns.h"
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include "debug.h"

void map(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2))
{
    assert(dest != NULL);
    assert(src != NULL);
    assert(worker != NULL);
    char *d = dest;
    char *s = src;

#pragma omp parallel for
    for (int i = 0; i < nJob; i++)
    {
        worker(&d[i * sizeJob], &s[i * sizeJob]);
    }
}

//CODIGO DO PROFESSOR NAO ALTERADO
void map_seq(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2))
{
    /* To be implemented */
    assert(dest != NULL);
    assert(src != NULL);
    assert(worker != NULL);
    char *d = dest;
    char *s = src;
    for (int i = 0; i < nJob; i++)
    {
        worker(&d[i * sizeJob], &s[i * sizeJob]);
    }
}

void reduce(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3))
{
    /* To be implemented */
    assert(dest != NULL);
    assert(src != NULL);
    assert(worker != NULL);
    char *d = dest;
    char *s = src;
    char *values;
    int size;

    if (nJob > 0)
    {
        int maxThreads = omp_get_max_threads();
        if (omp_get_max_threads() > nJob)
            omp_set_num_threads(nJob);
#pragma omp parallel shared(values, size)
        {
#pragma omp single
            //Make an array for final thread value
            {
                size = sizeJob * omp_get_num_threads();
                values = malloc(size);
            }
            TYPE tmp;
            //grab initial value from source
            int tid = omp_get_thread_num();
            memcpy(&tmp, &s[tid * sizeJob], sizeJob);

#pragma omp for
            //Do Thread work and add its value to the final array
            for (int i = omp_get_num_threads(); i < nJob; i++)
            {
                worker(&tmp, &tmp, &s[i * sizeJob]);
            }
            memcpy(&values[tid * sizeJob], &tmp, sizeJob);
        }
        //do final reduction after all theads finished
        for (int i = 1; i < size / sizeof(TYPE); i++)
        {
            worker(&values[0], &values[0], &values[i * sizeJob]);
        }
        //Set final value
        memcpy(&d[0], &values[0], sizeJob);

        //free and reset
        free(values);
        omp_set_num_threads(maxThreads);
    }
}

void reduce_seq(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3))
{
    /* To be implemented */
    assert(dest != NULL);
    assert(src != NULL);
    assert(worker != NULL);
    char *d = dest;
    char *s = src;
    if (nJob > 0)
    {
        memcpy(&d[0], &s[0], sizeJob);
        for (int i = 1; i < nJob; i++)
            worker(&d[0], &d[0], &s[i * sizeJob]);
    }
}

void scan(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3))
{
    /* To be implemented */
    assert(dest != NULL);
    assert(src != NULL);
    assert(worker != NULL);
    char *d = dest;
    char *s = src;
    if (nJob > 1)
    {
        memcpy(&d[0], &s[0], sizeJob);
        for (int i = 1; i < nJob; i++)
            worker(&d[i * sizeJob], &d[(i - 1) * sizeJob], &s[i * sizeJob]);
    }
}

int pack(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter)
{
    /* To be implemented */
    assert(dest != NULL);
    assert(src != NULL);
    assert(filter != NULL);
    assert(nJob >= 0);
    assert(sizeJob > 0);
    char *d = dest;
    char *s = src;
    int pos = 0;
    for (int i = 0; i < nJob; i++)
    {
        if (filter[i])
        {
            memcpy(&d[pos * sizeJob], &s[i * sizeJob], sizeJob);
            pos++;
        }
    }
    return pos;
}

void gather(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter, int nFilter)
{
    /* To be implemented */
    assert(dest != NULL);
    assert(src != NULL);
    assert(filter != NULL);
    assert(nJob >= 0);
    assert(sizeJob > 0);
    assert(nFilter >= 0);
    char *d = dest;
    char *s = src;
    for (int i = 0; i < nFilter; i++)
    {
        assert(filter[i] < nJob);
        memcpy(&d[i * sizeJob], &s[filter[i] * sizeJob], sizeJob);
    }
}

void scatter(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter)
{
    /* To be implemented */
    assert(dest != NULL);
    assert(src != NULL);
    assert(filter != NULL);
    assert(nJob >= 0);
    assert(sizeJob > 0);
    char *d = dest;
    char *s = src;
    for (int i = 0; i < nJob; i++)
    {
        assert(filter[i] < nJob);
        memcpy(&d[filter[i] * sizeJob], &s[i * sizeJob], sizeJob);
    }
}

void pipeline(void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers)
{
    /* To be implemented */
    assert(dest != NULL);
    assert(src != NULL);
    assert(workerList != NULL);
    assert(nJob >= 0);
    assert(sizeJob > 0);
    char *d = dest;
    char *s = src;
    for (int i = 0; i < nJob; i++)
    {
        memcpy(&d[i * sizeJob], &s[i * sizeJob], sizeJob);
        for (int j = 0; j < nWorkers; j++)
        {
            assert(workerList[j] != NULL);
            workerList[j](&d[i * sizeJob], &d[i * sizeJob]);
        }
    }
}

void farm(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2), size_t nWorkers)
{
    /* To be implemented */
    map(dest, src, nJob, sizeJob, worker); // it provides the right result, but is a very very vey bad implementation…
}
