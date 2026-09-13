/*
 * billing.c — Time-Based Parking Billing Calculator
 *
 * See billing.h for the rate structure documentation.
 *
 * RATE BREAKDOWN (per-minute prorating):
 *   Minutes 0–30:   Free ($0.00)
 *   Minutes 31–120: $3.00/hour = $0.05/minute
 *   Minutes 121–360: $5.00/hour ≈ $0.0833/minute
 *   Minutes 361+:   $8.00/hour ≈ $0.1333/minute
 *   VIP discount:   20% off the total
 *   Daily cap:      $40.00 max
 */

#include "../include/billing.h"
#include <stdio.h>   /* snprintf */

/*
 * calculate_charge — Compute the tiered parking fee.
 *
 * We break the total duration into segments and apply each tier's
 * rate only to the minutes that fall within that tier. This avoids
 * overcharging — e.g., a 45-minute stay pays for only 15 billable
 * minutes (the first 30 are free).
 */
double calculate_charge(long entry_time, long exit_time, int is_vip)
{
    long duration_sec;
    double duration_min;
    double charge = 0.0;

    /* Guard against negative durations (clock issues) */
    duration_sec = exit_time - entry_time;
    if (duration_sec <= 0) return 0.0;

    duration_min = (double)duration_sec / 60.0;

    /*
     * Tier 1: First 30 minutes — FREE
     * Nothing to add for minutes 0–30.
     */

    /*
     * Tier 2: Minutes 31–120 at $3.00/hour ($0.05/min)
     *
     * We calculate how many minutes fall in this tier:
     *   - If total <= 30 min: 0 minutes in this tier
     *   - If total <= 120 min: (total - 30) minutes
     *   - If total > 120 min: full 90 minutes (120 - 30)
     */
    if (duration_min > 30.0) {
        double tier2_minutes;
        if (duration_min <= 120.0) {
            tier2_minutes = duration_min - 30.0;
        } else {
            tier2_minutes = 90.0;  /* Full tier: 30 to 120 = 90 min */
        }
        charge += tier2_minutes * (3.0 / 60.0);  /* $3/hr = $0.05/min */
    }

    /*
     * Tier 3: Minutes 121–360 at $5.00/hour ($0.0833/min)
     */
    if (duration_min > 120.0) {
        double tier3_minutes;
        if (duration_min <= 360.0) {
            tier3_minutes = duration_min - 120.0;
        } else {
            tier3_minutes = 240.0;  /* Full tier: 120 to 360 = 240 min */
        }
        charge += tier3_minutes * (5.0 / 60.0);
    }

    /*
     * Tier 4: Minutes 361+ at $8.00/hour ($0.1333/min)
     */
    if (duration_min > 360.0) {
        double tier4_minutes = duration_min - 360.0;
        charge += tier4_minutes * (8.0 / 60.0);
    }

    /* Apply VIP discount: 20% off */
    if (is_vip) {
        charge *= 0.80;
    }

    /* Apply daily maximum cap */
    if (charge > 40.0) {
        charge = 40.0;
    }

    return charge;
}

/*
 * format_duration — Convert seconds to a human-readable "Xh Ym" string.
 */
void format_duration(long seconds, char *buffer)
{
    long hours, minutes;

    if (!buffer) return;

    if (seconds < 0) seconds = 0;

    hours = seconds / 3600;
    minutes = (seconds % 3600) / 60;

    if (hours > 0) {
        snprintf(buffer, 32, "%ldh %ldm", hours, minutes);
    } else {
        snprintf(buffer, 32, "%ldm", minutes);
    }
}
