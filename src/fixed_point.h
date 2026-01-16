#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h>

typedef int64_t fixed64_t;

#define FIX_FRAC_BITS 52
#define FIX_ONE ((int64_t)1 << FIX_FRAC_BITS)


#define fix_add(a,b) ((a)+(b))
#define fix_sub(a,b) ((a)-(b))

fixed64_t fix_from_int(int x);
fixed64_t fix_from_float(double x);
int fix_to_int_round(fixed64_t x);
fixed64_t fix_mul(fixed64_t a, fixed64_t b);

#endif