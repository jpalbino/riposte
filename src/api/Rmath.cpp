
#include "api.h"
#include <stddef.h>

#include <R_ext/Arith.h>
#include <R_ext/Rdynload.h>
#include <Rmath.h>

DL_FUNC  User_norm_fun = NULL;

double R_pow(double x, double y) {
    _NYI("R_pow");
}

double R_pow_di(double x, int n) {
    _NYI("R_pow_di");
}

