/*
 * billing.c — Time-Based Parking Billing Calculator (AED)
 *
 * See billing.h for the rate structure documentation.
 *
 * TIME SCALING:
 *   10 real seconds = 1 garage hour.
 *   Scale factor TIME_SCALE = 360 (defined in billing.h).
 *
 * RATE:
 *   10 AED per garage hour, prorated by the minute.
 *   First 15 garage-minutes are free (grace period).
 *   VIP discount: 20% off.
 *   Daily cap: 200 AED.
 */

#include "../include/billing.h"
#include <stdio.h>   /* snprintf */

/*
 * calculate_charge — Compute the parking fee in AED.
 *
 * Steps:
 *   1. Compute real elapsed seconds.
 *   2. Scale to garage time (multiply by TIME_SCALE).
 *   3. Convert to garage minutes.
 *   4. Apply grace period (first 15 minutes free).
 *   5. Charge 10 AED/hour for remaining time.
 *   6. Apply VIP discount and daily cap.
 */
double calculate_charge(long entry_time, long exit_time, int is_vip)
{
    long real_seconds;
    double garage_seconds;
    double garage_minutes;
    double charge;

    /* Guard against negative durations (clock issues) */
    real_seconds = exit_time - entry_time;
    if (real_seconds <= 0) return 0.0;

    /* Scale to garage time */
    garage_seconds = (double)real_seconds * TIME_SCALE;
    garage_minutes = garage_seconds / 60.0;

    /* Grace period: first 15 garage-minutes are free */
    if (garage_minutes <= 15.0) return 0.0;

    /* 10 AED per hour, prorated by the minute */
    charge = (garage_minutes / 60.0) * 10.0;

    /* Apply VIP discount: 20% off */
    if (is_vip) {
        charge *= 0.80;
    }

    /* Apply daily maximum cap */
    if (charge > 200.0) {
        charge = 200.0;
    }

    return charge;
}

/*
 * format_duration — Convert real seconds to garage-time "Xh Ym" string.
 *
 * Scales by TIME_SCALE so the displayed duration reflects garage time,
 * not wall-clock time.
 */
void format_duration(long seconds, char *buffer)
{
    long garage_seconds;
    long hours, minutes;

    if (!buffer) return;

    if (seconds < 0) seconds = 0;

    /* Scale to garage time */
    garage_seconds = seconds * TIME_SCALE;

    hours = garage_seconds / 3600;
    minutes = (garage_seconds % 3600) / 60;

    if (hours > 0) {
        snprintf(buffer, 32, "%ldh %ldm", hours, minutes);
    } else {
        snprintf(buffer, 32, "%ldm", minutes);
    }
}
