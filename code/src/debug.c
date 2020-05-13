#include <stdio.h>
#include <unistd.h>
#include "debug.h"

#define MAX_DEBUG_VALUES 30

int debug = 0;

void printDouble(const double *src, size_t n, const char *msg)
{
    if (debug)
    {
        if (n > MAX_DEBUG_VALUES)
            n = MAX_DEBUG_VALUES;
        printf("%s %s: ", msg, "double");
        for (int i = 0; i < n; i++)
            printf("[%d]=%lf ", i, src[i]);
        printf("\n-------------------------------\n");
    }
}

void printInt(const int *src, size_t n, const char *msg)
{
    if (debug)
    {
        if (n > MAX_DEBUG_VALUES)
            n = MAX_DEBUG_VALUES;
        printf("%s int: ", msg);
        for (int i = 0; i < n; i++)
            printf("[%d]=%d ", i, src[i]);
        printf("\n-------------------------------\n");
    }
}

void printFloat(const float *src, size_t n, const char *msg)
{
    if (debug)
    {
        if (n > MAX_DEBUG_VALUES)
            n = MAX_DEBUG_VALUES;
        printf("%s %s: ", msg, "double");
        for (int i = 0; i < n; i++)
            printf("[%d]=%f ", i, src[i]);
        printf("\n-------------------------------\n");
    }
}