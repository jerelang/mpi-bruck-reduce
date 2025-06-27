#include "algorithms.h"

void mergeInts(const int* a, int aSize, const int* b, int bSize, int* out) {
    int i = 0, j = 0, k = 0;
    while (i < aSize && j < bSize) {
        out[k++] = (a[i] < b[j]) ? a[i++] : b[j++];
    }
    while (i < aSize) out[k++] = a[i++];
    while (j < bSize) out[k++] = b[j++];
}