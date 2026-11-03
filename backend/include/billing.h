/*
 * billing.h — Time-Based Parking Billing Calculator
 *
 * PURPOSE:
 *   Computes the parking charge based on how long a vehicle was parked.
 *   Uses accelerated garage time (10 real seconds = 1 garage hour).
 *
 * RATE STRUCTURE:
 *   - First 15 garage-minutes:  Free (grace period)
 *   - After grace period:       10 AED/hour (prorated by minute)
 *   - VIP discount:             20% off the total
 *   - Daily maximum cap:        200 AED
 *
 * TIME SCALING:
 *   10 real seconds = 1 garage hour (factor of 360x).
 *   This lets demos run in real-time without waiting hours.
 */

#ifndef BILLING_H
#define BILLING_H

/* 10 real seconds = 1 garage hour (3600 seconds). Scale factor = 360. */
#define TIME_SCALE 360

/*
 * calculate_charge — Compute the parking fee in AED.
 * @entry_time: Unix timestamp when the vehicle entered.
 * @exit_time:  Unix timestamp when the vehicle is exiting.
 * @is_vip:     1 if the vehicle is VIP (gets 20% discount), 0 otherwise.
 *
 * Internally scales real elapsed time to garage time before billing.
 * Returns the amount to charge (in AED, as a double).
 */
double calculate_charge(long entry_time, long exit_time, int is_vip);

/*
 * format_duration — Format real elapsed seconds as garage time string.
 * @seconds: Duration in real seconds.
 * @buffer:  Output buffer (at least 32 bytes).
 *
 * Scales by TIME_SCALE and writes e.g. "2h 15m" or "45m" into buffer.
 */
void format_duration(long seconds, char *buffer);

/*
 * BillingBreakdown — Detailed billing calculation results.
 * Used to show the step-by-step calculation in the exit receipt.
 */
typedef struct {
    double garage_minutes;    /* Total garage-time minutes              */
    double raw_charge;        /* Charge at 10 AED/hr before adjustments */
    double after_discount;    /* Charge after VIP discount (if any)     */
    double final_charge;      /* Final amount after cap                 */
    int    grace_applied;     /* 1 if within 15-min grace period        */
    int    cap_applied;       /* 1 if 200 AED daily cap was applied     */
} BillingBreakdown;

/*
 * calculate_charge_detailed — Compute parking fee with full breakdown.
 * Returns the final charge (same as calculate_charge) and populates
 * the breakdown struct with intermediate calculation steps.
 */
double calculate_charge_detailed(long entry_time, long exit_time,
                                 int is_vip, BillingBreakdown *bd);

#endif /* BILLING_H */
