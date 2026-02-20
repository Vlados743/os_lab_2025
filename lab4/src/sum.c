#include "sum.h"

uint64_t Sum(const struct SumArgs *args) {
  uint64_t sum = 0;
  for (int i = args->begin; i < args->end; i++) {
    sum += args->array[i];
  }
  return sum;
}