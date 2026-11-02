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

#endif /* BILLING_H */
