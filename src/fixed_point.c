#include <stdint.h>
#include "fixed_point.h"
// bibliothèque de nombre à virgule fixe Q12.52 pour vishash
// normalement tous les décimaux manipulés par vishash sont inférieurs à 2048, donc la partie entière sur 12 bits est largement suffisante


inline fixed64_t fix_from_int(int x) {
    return (fixed64_t)x << FIX_FRAC_BITS;
}

inline fixed64_t fix_from_float(double x) {
    return (fixed64_t)(x * (double)FIX_ONE);
}

inline int fix_to_int_round(fixed64_t x) {
    return (int)((x + (FIX_ONE >> 1)) >> FIX_FRAC_BITS);
}


inline fixed64_t fix_mul(fixed64_t a, fixed64_t b) {
    __int128 tmp = (__int128)a * b;
    return (fixed64_t)(tmp >> FIX_FRAC_BITS);
}
