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
 * Wrapper around calculate_charge_detailed without breakdown output.
 */
double calculate_charge(long entry_time, long exit_time, int is_vip)
{
    return calculate_charge_detailed(entry_time, exit_time, is_vip, NULL);
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

/*
 * calculate_charge_detailed — Compute parking fee with full breakdown.
 *
 * Same logic as calculate_charge, but populates a BillingBreakdown
 * struct with intermediate values for display in the exit receipt.
 */
double calculate_charge_detailed(long entry_time, long exit_time,
                                 int is_vip, BillingBreakdown *bd)
{
    long real_seconds;
    double garage_seconds;
    double garage_minutes;
    double charge;

    /* Initialize breakdown */
    if (bd) {
        bd->garage_minutes = 0.0;
        bd->raw_charge = 0.0;
        bd->after_discount = 0.0;
        bd->final_charge = 0.0;
        bd->grace_applied = 0;
        bd->cap_applied = 0;
    }

    /* Guard against negative durations */
    real_seconds = exit_time - entry_time;
    if (real_seconds <= 0) {
        if (bd) bd->grace_applied = 1;
        return 0.0;
    }

    /* Scale to garage time */
    garage_seconds = (double)real_seconds * TIME_SCALE;
    garage_minutes = garage_seconds / 60.0;

    if (bd) bd->garage_minutes = garage_minutes;

    /* Grace period: first 15 garage-minutes are free */
    if (garage_minutes <= 15.0) {
        if (bd) bd->grace_applied = 1;
        return 0.0;
    }

    /* 10 AED per hour, prorated by the minute */
    charge = (garage_minutes / 60.0) * 10.0;
    if (bd) bd->raw_charge = charge;

    /* Apply VIP discount: 20% off */
    if (is_vip) {
        charge *= 0.80;
    }
    if (bd) bd->after_discount = charge;

    /* Apply daily maximum cap */
    if (charge > 200.0) {
        if (bd) bd->cap_applied = 1;
        charge = 200.0;
    }

    if (bd) bd->final_charge = charge;

    return charge;
}
