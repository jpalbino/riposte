
#define R_NO_REMAP
#include <Rinternals.h>
#include <R_ext/GraphicsEngine.h>

void R_GE_checkVersionOrDie(int version) {
    throw "NYI: R_GE_checkVersionOrDie";
}

pGEDevDesc Rf_desc2GEDesc(pDevDesc dd) {
    throw "NYI: Rf_desc2GEDesc";
}

pGEDevDesc GEgetDevice(int) {
    throw "NYI: GEgetDevice";
}

void GEaddDevice(pGEDevDesc) {
    throw "NYI: GEaddDevice";
}

void GEaddDevice2(pGEDevDesc, const char *) {
    throw "NYI: GEaddDevice2";
}

void GEaddDevice2f(pGEDevDesc, const char *, const char *) {
    throw "NYI: GEaddDevice2f";
}

void GEkillDevice(pGEDevDesc) {
    throw "NYI: GEkillDevice";
}

pGEDevDesc GEcreateDevDesc(pDevDesc dev) {
    throw "NYI: GEcreateDevDesc";
}

void GEregisterSystem(GEcallback callback, int *systemRegisterIndex) {
    // TODO: figure out what this function does and implement it.
    systemRegisterIndex = 0;
}

void GEunregisterSystem(int registerIndex) {
    throw "NYI: GEunregisterSystem";
}


/* Convert an element of a R colour specification (which might be a
   number or a string) into an internal colour specification. */
rcolor Rf_RGBpar(SEXP, int) {
    throw "NYI: Rf_RGBpar";
}

rcolor Rf_RGBpar3(SEXP, int, rcolor) {
    throw "NYI: Rf_RGBpar3";
}

/* Convert an internal colour specification to/from a colour name */
const char *Rf_col2name(rcolor col) { /* used in par.c, grid */
    throw "NYI: Rf_col2name";
}

/* Convert either a name or a #RRGGBB[AA] string to internal.
   Because people were using it, it also converts "1", "2" ...
   to a colour in the palette, and "0" to transparent white.
*/
rcolor R_GE_str2col(const char *s) {
    throw "NYI: R_GE_str2col";
}

R_GE_lineend GE_LENDpar(SEXP value, int ind) {
    throw "NYI: GE_LENDpar";
}

SEXP GE_LENDget(R_GE_lineend lend) {
    throw "NYI: GE_LENDget";
}

R_GE_linejoin GE_LJOINpar(SEXP value, int ind) {
    throw "NYI: GE_LJOINpar";
}

SEXP GE_LJOINget(R_GE_linejoin ljoin) {
    throw "NYI: GE_LJOINget";
}

void GESetClip(double x1, double y1, double x2, double y2, pGEDevDesc dd) {
    throw "NYI: GE_SetClip";
}

void GENewPage(const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GENewPage";
}

void GELine(double x1, double y1, double x2, double y2,
        const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GELine";
}

void GEPolyline(int n, double *x, double *y,
        const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GEPolyline";
}

void GEPolygon(int n, double *x, double *y,
           const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GEPolygon";
}

SEXP GEXspline(int n, double *x, double *y, double *s, Rboolean open,
           Rboolean repEnds, Rboolean draw,
           const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GEXspline";
}

void GECircle(double x, double y, double radius,
          const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GECircle";
}

void GERect(double x0, double y0, double x1, double y1,
        const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GERect";
}

void GEPath(double *x, double *y,
            int npoly, int *nper,
            Rboolean winding,
            const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GEPath";
}

void GERaster(unsigned int *raster, int w, int h,
              double x, double y, double width, double height,
              double angle, Rboolean interpolate,
              const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GERaster";
}

SEXP GECap(pGEDevDesc dd) {
    throw "NYI: GECap";
}

void GEText(double x, double y, const char * const str, cetype_t enc,
        double xc, double yc, double rot,
        const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GEText";
}

void GEMode(int mode, pGEDevDesc dd) {
    throw "NYI: GEMode";
}

void GESymbol(double x, double y, int pch, double size,
          const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GESymbol";
}

void GEPretty(double *lo, double *up, int *ndiv) {
    throw "NYI: GEPretty";
}

void GEMetricInfo(int c, const pGEcontext gc,
          double *ascent, double *descent, double *width,
          pGEDevDesc dd) {
    throw "NYI: GEMetricInfo";
}

double GEStrWidth(const char *str, cetype_t enc,
          const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GEStrWidth";
}

double GEStrHeight(const char *str, cetype_t enc,
          const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GEStrHeight";
}

void GEStrMetric(const char *str, cetype_t enc, const pGEcontext gc,
                 double *ascent, double *descent, double *width,
                 pGEDevDesc dd) {
    throw "NYI: GEStrMetric";
}

int GEstring_to_pch(SEXP pch) {
    throw "NYI: GEstring_to_pch";
}

/*-------------------------------------------------------------------
 *
 *  LINE TEXTURE CODE is concerned with the internals of R
 *  line texture representation.
 */
unsigned int GE_LTYpar(SEXP, int) {
    throw "NYI: GE_LTYpar";
}

SEXP GE_LTYget(unsigned int) {
    throw "NYI: GE_LTYget";
}

/*
 * Raster operations
 */
void R_GE_rasterInterpolate(unsigned int *sraster, int sw, int sh,
                            unsigned int *draster, int dw, int dh) {
    throw "NYI: R_GE_rasterInterpolate";
}

/*
 * From plotmath.c
 */
double GEExpressionWidth(SEXP expr,
             const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GEExpressionWidth";
}

double GEExpressionHeight(SEXP expr,
              const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GEExpressionHeight";
}

void GEExpressionMetric(SEXP expr, const pGEcontext gc,
                        double *ascent, double *descent, double *width,
                        pGEDevDesc dd) {
    throw "NYI: GEExpressionMetric";
}

void GEMathText(double x, double y, SEXP expr,
        double xc, double yc, double rot,
        const pGEcontext gc, pGEDevDesc dd) {
    throw "NYI: GEMathText";
}




pGEDevDesc GEcurrentDevice(void) {
    throw "NYI: GEcurrentDevice";
}

void GEdirtyDevice(pGEDevDesc dd) {
    throw "NYI: GEdirtyDevice";
}

Rboolean GErecording(SEXP call, pGEDevDesc dd) {
    throw "NYI: GErecording";
}

void GErecordGraphicOperation(SEXP op, SEXP args, pGEDevDesc dd) {
    throw "NYI: GErecordGraphicOperation";
}

void GEinitDisplayList(pGEDevDesc dd) {
    throw "NYI: GEinitDisplayList";
}

void GEplayDisplayList(pGEDevDesc dd) {
    throw "NYI: GEplayDisplayList";
}

void GEcopyDisplayList(int fromDevice) {
    throw "NYI: GEcopyDisplayList";
}

SEXP GEcreateSnapshot(pGEDevDesc dd) {
    throw "NYI: GEcreateSnapshot";
}

void GEplaySnapshot(SEXP snapshot, pGEDevDesc dd) {
    throw "NYI: GEplaySnapshot";
}

/* From ../../main/plot.c, used by ../../library/grid/src/grid.c : */
// Also used by grDevices
SEXP Rf_CreateAtVector(double*, double*, int, Rboolean) {
    throw "NYI: CreateAtVector";
}

void Rf_GAxisPars(double *min, double *max, int *n, Rboolean log, int axis) {
    throw "NYI: Rf_GAxisPars";
}


// From main/colors.c, used by grDevices

extern "C" {

typedef unsigned int (*F1)(SEXP x, int i, unsigned int bg);
typedef const char * (*F2)(unsigned int col);
typedef unsigned int (*F3)(const char *s);
typedef void (*F4)(Rboolean save);

static F1 ptr_RGBpar3;
static F2 ptr_col2name;
static F3 ptr_R_GE_str2col;
static F4 ptr_savePalette;

void Rg_set_col_ptrs(F1 f1, F2 f2, F3 f3, F4 f4)
{
    ptr_RGBpar3 = f1;
    ptr_col2name = f2;
    ptr_R_GE_str2col = f3;
    ptr_savePalette = f4;
}

}

// .Internals used by grDevices

extern "C" {

SEXP do_X11(SEXP, SEXP, SEXP, SEXP) {
    throw "NYI: do_X11";
}

SEXP do_contourLines(SEXP, SEXP, SEXP, SEXP) {
    throw "NYI: do_contourLines";
}

SEXP do_getGraphicsEvent(SEXP, SEXP, SEXP, SEXP) {
    throw "NYI: do_getGraphicsEvent";
}

SEXP do_getGraphicsEventEnv(SEXP, SEXP, SEXP, SEXP) {
    throw "NYI: do_getGraphicsEventEnv";
}

SEXP do_saveplot(SEXP, SEXP, SEXP, SEXP) {
    throw "NYI: do_saveplot";
}

SEXP do_setGraphicsEventEnv(SEXP, SEXP, SEXP, SEXP) {
    throw "NYI: do_setGraphicsEventEnv";
}

SEXP do_getSnapshot(SEXP, SEXP, SEXP, SEXP) {
    throw "NYI: do_getSnapshot";
}

SEXP do_playSnapshot(SEXP, SEXP, SEXP, SEXP) {
    throw "NYI: do_playSnapshot";
}

}

// Functions used by graphics

extern "C" {

int baseRegisterIndex;

struct GPar;

GPar* Rf_gpptr(pGEDevDesc dd) {
    throw "NYI: Rf_gpptr";
}

GPar* Rf_dpptr(pGEDevDesc dd) {
    throw "NYI: Rf_dpptr";
}

/* Return a "nice" min, max and number of intervals for a given
 * range on a linear or _log_ scale, respectively: */
void Rf_GPretty(double*, double*, int*) { /* used in plot3d.c */
    throw "NYI: Rf_GPretty";
}

}