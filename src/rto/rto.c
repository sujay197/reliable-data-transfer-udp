#include "rto.h"
#include <stddef.h>

void rto_init(RTOEstimator *rto, double initial_rto_ms)
{
    if (rto == NULL) {
        return;
    }

    rto->estimated_rtt_ms = initial_rto_ms;
    rto->dev_rtt_ms = 0.0;
    rto->rto_ms = initial_rto_ms;
}

void rto_update(RTOEstimator *rto, double sample_rtt_ms)
{
    if (rto == NULL || sample_rtt_ms <= 0.0) {
        return;
    }

    /*
     * Jacobson/Karels-style RTT estimation:
     *
     * EstimatedRTT = 0.875 * EstimatedRTT
     *             + 0.125 * SampleRTT
     *
     * DevRTT = 0.75 * DevRTT
     *        + 0.25 * |SampleRTT - EstimatedRTT|
     *
     * RTO = EstimatedRTT + 4 * DevRTT
     */

    double error = sample_rtt_ms - rto->estimated_rtt_ms;

    rto->estimated_rtt_ms =
        0.875 * rto->estimated_rtt_ms +
        0.125 * sample_rtt_ms;

    rto->dev_rtt_ms =
        0.75 * rto->dev_rtt_ms +
        0.25 * (error >= 0.0 ? error : -error);

    rto->rto_ms =
        rto->estimated_rtt_ms +
        4.0 * rto->dev_rtt_ms;

    /* Keep RTO within reasonable limits. */
    if (rto->rto_ms < 100.0) {
        rto->rto_ms = 100.0;
    }

    if (rto->rto_ms > 10000.0) {
        rto->rto_ms = 10000.0;
    }
}

double rto_get(const RTOEstimator *rto)
{
    if (rto == NULL) {
        return 0.0;
    }

    return rto->rto_ms;
}