#include <string.h>
#include <assert.h>
#include "patterns.h"
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include "debug.h"
#include <math.h>

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

        //free
        free(values);
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

void buildRange(TYPE *treeSum, int lo, int hi, void (*worker)(void *dst, const void *v1, const void *v2))
{
#pragma omp parallel for
    for (size_t i = lo; i < hi; i += 2)
    {
        // printf("Lo: %ld\nHI: %d\n", i, hi);
        worker(&treeSum[i / 2], &treeSum[i], &treeSum[i + 1]);
    }
    // }
}

void sumRange(TYPE *treeSum, TYPE *treeLeft, int lo, int hi, void (*worker)(void *dst, const void *v1, const void *v2))
{
#pragma omp parallel for
    for (size_t i = lo; i <= hi; i++)
    {
        int leftChild = i * 2 + 1;
        // printf("Lo: %ld\nHI: %d\n", i, hi);
        // printf("LChild: %d\n", leftChild);
        memcpy(&treeLeft[leftChild], &treeLeft[i], sizeof(TYPE));
        worker(&treeLeft[leftChild + 1], &treeLeft[i], &treeSum[leftChild]);
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
    // char *s = src;
    TYPE *sd = src;
    TYPE *treeSum;
    TYPE *treeFromLeft;
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
                memcpy(&treeSum[idx + i], &sd[i], sizeJob);
            }

            // memcpy(&treeSum[idx], &sd[0], lastRow * sizeJob);
            int remaining = (nJob - lastRow);
            // printf("remaining:%d\n", remaining);
            idx = idx - remaining;
#pragma omp for
            for (size_t i = 0; i < remaining; i++)
            {
                memcpy(&treeSum[idx + i], &sd[lastRow + i], sizeJob);
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
            buildRange(treeSum, lowRange, highRange, worker);
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
            sumRange(treeSum, treeFromLeft, lowRange, highRange, worker);
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
                worker(&d[i * sizeJob], &treeFromLeft[idx + i], &treeSum[idx + i]);
            }

            int remaining = (nJob - lastRow);
            idx = idx - remaining;
#pragma omp for
            for (size_t i = 0; i < remaining; i++)
            {
                worker(&d[(i + lastRow) * sizeJob], &treeFromLeft[idx + i], &treeSum[idx + i]);
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
