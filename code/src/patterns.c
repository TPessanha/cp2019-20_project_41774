#include <string.h>
#include <assert.h>
#include "patterns.h"
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include "debug.h"
#include <math.h>

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
            char *tmp = malloc(sizeJob);
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
        for (int i = 1; i < size / sizeJob; i++)
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

void buildRange(char *treeSum, int lo, int hi, void (*worker)(void *dst, const void *v1, const void *v2), size_t sizeJob)
{
#pragma omp parallel for
    for (size_t i = lo; i < hi; i += 2)
    {
        // printf("Lo: %ld\nHI: %d\n", i, hi);
        worker(&treeSum[(i / 2) * sizeJob], &treeSum[i * sizeJob], &treeSum[(i + 1) * sizeJob]);
    }
    // }
}

void sumRange(char *treeSum, char *treeLeft, int lo, int hi, void (*worker)(void *dst, const void *v1, const void *v2), size_t sizeJob)
{
#pragma omp parallel for
    for (size_t i = lo; i <= hi; i++)
    {
        int leftChild = i * 2 + 1;
        // printf("Lo: %ld\nHI: %d\n", i, hi);
        // printf("LChild: %d\n", leftChild);
        memcpy(&treeLeft[leftChild * sizeJob], &treeLeft[i * sizeJob], sizeJob);
        worker(&treeLeft[(leftChild + 1) * sizeJob], &treeLeft[i * sizeJob], &treeSum[leftChild * sizeJob]);
    }
    // }
}

int getFirstRowIdx(int level)
{
    return ((int)pow(2, level)) - 1;
}

int getHeight(int nNodes)
{
    return ((int)log2(nNodes));
}

void scan(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3))
{
    assert(dest != NULL);
    assert(src != NULL);
    assert(worker != NULL);
    char *d = dest;
    char *s = src;
    char *treeSum;
    char *treeFromLeft;
    if (nJob > 1)
    {
        int treeSize = nJob * 2 - 1;
        treeSum = malloc(treeSize * sizeJob);
        int levels = getHeight(treeSize);
        // printf("Njob:%ld\ntreeSize:%d\nlevels:%d\n", nJob, treeSize, levels);

        //Builds leafs*****************************************************
        int idx = getFirstRowIdx(levels);
        int lastRow = treeSize - idx;
#pragma omp parallel firstprivate(idx)
        {
#pragma omp for
            for (size_t i = 0; i < lastRow; i++)
            {
                memcpy(&treeSum[(idx + i) * sizeJob], &s[i * sizeJob], sizeJob);
            }

            // memcpy(&treeSum[idx], &sd[0], lastRow * sizeJob);
            int remaining = (nJob - lastRow);
            // printf("remaining:%d\n", remaining);
            idx = idx - remaining;
#pragma omp for
            for (size_t i = 0; i < remaining; i++)
            {
                memcpy(&treeSum[(idx + i) * sizeJob], &s[(lastRow + i) * sizeJob], sizeJob);
            }
        }
        // memcpy(&treeSum[idx - remaining], &sd[lastRow], remaining * sizeJob);

        // printDouble(treeSum, treeSize, "Leafs");

        // Build tree UP pass************************************************
        int lvl = levels;
        int lowRange = getFirstRowIdx(lvl);
        int highRange = treeSize - 1;
        while (lvl > 0)
        {
            // printf("lvl: %d\nFrom %d to %d\n", lvl, lowRange, highRange);
            buildRange(treeSum, lowRange, highRange, worker, sizeJob);
            highRange = getFirstRowIdx(lvl) - 1;
            lowRange = getFirstRowIdx(lvl - 1);
            lvl--;
        }

        // printDouble(treeSum, treeSize, "TREESUM up pass");

        // Down Pass ************************************************************
        treeFromLeft = malloc(treeSize * sizeJob);
        treeFromLeft[0] = 0;
        lvl = 0;
        lowRange = 0;
        highRange = 0;
        while (lvl < levels)
        {
            // printf("lvl: %d\nFrom %d to %d\n", lvl, lowRange, highRange);
            sumRange(treeSum, treeFromLeft, lowRange, highRange, worker, sizeJob);
            lvl++;
            lowRange = getFirstRowIdx(lvl);
            if (lvl < levels - 1)
                highRange = getFirstRowIdx(lvl + 1) - 1;
            else
                highRange = lowRange + lastRow / 2 - 1;
        }

        // printDouble(treeFromLeft, treeSize, "TREE-From-Left DOWNPASS");

        //Final output***************************************************************

        // printf("Levels: %d\n", levels);
        idx = getFirstRowIdx(levels);
#pragma omp parallel firstprivate(idx)
        {
#pragma omp for
            for (size_t i = 0; i < lastRow; i++)
            {
                worker(&d[i * sizeJob], &treeFromLeft[(idx + i) * sizeJob], &treeSum[(idx + i) * sizeJob]);
            }

            int remaining = (nJob - lastRow);
            idx = idx - remaining;
#pragma omp for
            for (size_t i = 0; i < remaining; i++)
            {
                worker(&d[(i + lastRow) * sizeJob], &treeFromLeft[(idx + i) * sizeJob], &treeSum[(idx + i) * sizeJob]);
            }
        }
        //free
        free(treeSum);
        free(treeFromLeft);
    }
}

void scan_seq(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3))
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

static void workerAdd(void *a, const void *b, const void *c)
{
    // a = b + c
    *(int *)a = *(int *)b + *(int *)c;
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
    int *bitsum;

    // SCAN ***********************************************
    bitsum = malloc(nJob * sizeof(int));
    scan_seq(bitsum, (void *)filter, nJob, sizeof(int), workerAdd);
    int pos = bitsum[nJob - 1];

    // parallel map ******************************************
#pragma omp parallel for
    for (int i = 0; i < nJob; i++)
    {
        if (filter[i])
            memcpy(&d[(bitsum[i] - 1) * sizeJob], &s[i * sizeJob], sizeJob);
    }

    //free
    free(bitsum);

    return pos;
}

int pack_seq(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter)
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
#pragma omp parallel for
    for (int i = 0; i < nFilter; i++)
    {
        assert(filter[i] < nJob);
        memcpy(&d[i * sizeJob], &s[filter[i] * sizeJob], sizeJob);
    }
}

void gather_seq(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter, int nFilter)
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
    assert(dest != NULL);
    assert(src != NULL);
    assert(filter != NULL);
    assert(nJob >= 0);
    assert(sizeJob > 0);
    char *d = dest;
    char *s = src;

    omp_lock_t *locks;
    if (nJob > 1)
    {
        locks = malloc(nJob * sizeof(omp_lock_t));

#pragma omp parallel
        {
#pragma omp for
            for (size_t i = 0; i < nJob; i++)
            {
                omp_init_lock(&locks[i]);
            }
#pragma omp for
            for (size_t i = 0; i < nJob; i++)
            {
                assert(filter[i] < nJob);
                if (omp_test_lock(&locks[filter[i]]))
                {
                    memcpy(&d[filter[i] * sizeJob], &s[i * sizeJob], sizeJob);
                    // omp_unset_lock(&locks[i]);
                }
            }
#pragma omp for
            for (size_t i = 0; i < nJob; i++)
            {
                omp_destroy_lock(&locks[i]);
            }
        }
        //free
        free(locks);
    }
}

void scatter_seq(void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter)
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
    assert(dest != NULL);
    assert(src != NULL);
    assert(workerList != NULL);
    assert(nJob >= 0);
    assert(sizeJob > 0);
    char *d = dest;
    char *s = src;
    int *workUntil[nWorkers + 1];

    int maxThreads = omp_get_max_threads();
#pragma omp parallel
    {
#pragma omp for
        for (size_t i = 0; i < nJob; i++)
        {
            memcpy(&d[i * sizeJob], &s[i * sizeJob], sizeJob);
        }
#pragma omp for
        for (size_t i = 1; i < nWorkers + 1; i++)
        {
            workUntil[i] = malloc(sizeof(int));
            *workUntil[i] = -1;
        }
    }
    workUntil[0] = malloc(sizeof(int));
    *workUntil[0] = nJob - 1;
    // printInt(workUntil, nWorkers + 1, "WorksDone");
    omp_set_num_threads(nWorkers);

#pragma omp parallel
    {
        int tid = omp_get_thread_num() + 1;
        int *prev = workUntil[tid - 1];
        int *own = workUntil[tid];
        // if (tid == 1)
        //     printf("own: %d, prev: %d\n", *own, *prev);
        int until = (int)nJob - 1;
        while (*own < until)
        {
            // assert(workUntil[tid] <= workUntil[tid - 1]);
            if (*own == *prev)
                continue;
            // assert(workerList[tid - 1] != NULL);

            // printf("thread %d Do: %d\n", tid, workUntil[tid] + 1);
            workerList[tid - 1](&d[(*own + 1) * sizeJob], &d[(*own + 1) * sizeJob]);
            (*own)++;
        }
    }

#pragma omp parallel for
    for (size_t i = 0; i < nWorkers + 1; i++)
    {
        free(workUntil[i]);
    }

    omp_set_num_threads(maxThreads);
}

void pipeline_seq(void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers)
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

void map_seq_range(void *dest, void *src, size_t from, size_t to, size_t sizeJob, void (*worker)(void *v1, const void *v2))
{
    assert(dest != NULL);
    assert(src != NULL);
    assert(worker != NULL);
    char *d = dest;
    char *s = src;
    for (int i = from; i < to; i++)
    {
        worker(&d[i * sizeJob], &s[i * sizeJob]);
    }
}

void farm(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2), size_t nWorkers)
{
    int maxThreads = omp_get_max_threads();
    omp_set_num_threads(nWorkers);
#pragma omp parallel
    {
#pragma omp master
        {
            int splited = nJob / nWorkers;
            for (size_t i = 0; i < nWorkers; i++)
            {
                if (i == nWorkers - 1)
                {
#pragma omp task
                    map_seq_range(dest, src, splited * i, nJob, sizeJob, worker);
                }
                else
                {
#pragma omp task
                    map_seq_range(dest, src, splited * i, splited * (i + 1), sizeJob, worker);
                }
            }
        }
    }
    omp_set_num_threads(maxThreads);
}

void farm_seq(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2), size_t nWorkers)
{
    /* To be implemented */
    map_seq(dest, src, nJob, sizeJob, worker); // it provides the right result, but is a very very vey bad implementation…
}
