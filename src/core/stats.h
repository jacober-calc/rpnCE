#ifndef RPNCE_CORE_STATS_H
#define RPNCE_CORE_STATS_H

/*
 * Statistics and curve fitting, HP-42S / RPN83P style. Data points (x, y)
 * are accumulated into running sums; everything else is computed from them.
 * All functions may raise OS errors (OS_E_STAT for too little data or an
 * unusable model), so call them under the error handler.
 */

#include <stdbool.h>
#include <stdint.h>
#include <ti/real.h>

/* The 13 sums, in the order of the STAT > Sigma menu. */
enum {
    SUM_N, SUM_X, SUM_X2, SUM_Y, SUM_Y2, SUM_XY,
    SUM_LX, SUM_LX2, SUM_LY, SUM_LY2, SUM_LXLY, SUM_XLY, SUM_YLX,
    SUM_COUNT
};

/* Curve-fit models. */
enum { FIT_LIN, FIT_LOG, FIT_EXP, FIT_PWR, FIT_COUNT };

typedef struct {
    real_t sum[SUM_COUNT];
    bool linear_only;       /* LIN-Sigma mode: only the first six sums */
    bool lnx_ok, lny_ok;    /* log sums are valid (all x > 0, all y > 0) */
    uint8_t model;          /* FIT_* used by the CFIT functions */
    real_t first[2];        /* witnesses for genuinely constant X/Y data */
    uint8_t constant;       /* bit 0: all X equal first[0]; bit 1: all Y equal */
} stats_t;

void stats_clear(stats_t *s);                    /* CL-Sigma, keeps modes */

/* Sigma+ (sign = +1) or Sigma- (sign = -1) for the point (x, y). */
void stats_add(stats_t *s, const real_t *x, const real_t *y, int sign);

/* x and y results: SUM, MEAN, WMN, and SDEV / PDEV (sample = true for SDEV). */
void stats_sums(const stats_t *s, real_t *x, real_t *y);
void stats_mean(const stats_t *s, real_t *x, real_t *y);
void stats_wmean(const stats_t *s, real_t *x, real_t *y);
void stats_sdev(const stats_t *s, bool sample, real_t *x, real_t *y);
real_t stats_cov(const stats_t *s, bool sample);

/* Curve fit with the current model: y = intercept + slope * x (LIN),
 * intercept + slope * ln x (LOG), intercept * e^(slope x) (EXP), or
 * intercept * x^slope (PWR). */
real_t stats_slope(const stats_t *s);
real_t stats_intercept(const stats_t *s);
real_t stats_corr(const stats_t *s);
real_t stats_forecast_y(const stats_t *s, const real_t *x);
real_t stats_forecast_x(const stats_t *s, const real_t *y);

/* Picks the valid model with the largest |correlation| and selects it. */
uint8_t stats_best(stats_t *s);

/* Whether a model can be used with the data so far. */
bool stats_model_ok(const stats_t *s, uint8_t model);

#endif
