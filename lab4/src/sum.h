#ifndef SUM_H
#define SUM_H

#include <stdint.h>   // для uint64_t

struct SumArgs {
  int *array;
  int begin;
  int end;
};

uint64_t Sum(const struct SumArgs *args);

#endif