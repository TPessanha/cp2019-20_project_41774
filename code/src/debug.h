#ifndef __DEBUG_H
#define __DEBUG_H

extern int debug;

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

void printDouble(const double *src, size_t n, const char *msg);

void printInt(const int *src, size_t n, const char *msg);

void printFloat(const float *src, size_t n, const char *msg);

#endif