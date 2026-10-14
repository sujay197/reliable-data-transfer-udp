#ifndef RTO_H
#define RTO_H

typedef struct {
    double estimated_rtt_ms;
    double dev_rtt_ms;
    double rto_ms;
} RTOEstimator;

void rto_init(RTOEstimator *rto, double initial_rto_ms);

void rto_update(RTOEstimator *rto, double sample_rtt_ms);

double rto_get(const RTOEstimator *rto);

#endif