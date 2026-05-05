#include "diag_common.h"

const char *diag_health_status(double warning_threshold, double critical_threshold, double value)
{
    if (value >= critical_threshold) {
        return "CRITICAL";
    }

    if (value >= warning_threshold) {
        return "WARNING";
    }

    return "OK";
}
