/*
 * billing.h — Time-Based Parking Billing Calculator
 *
 * PURPOSE:
 *   Computes the parking charge based on how long a vehicle was parked.
 *   Uses a tiered rate structure with VIP discounts and a daily cap.
 *
 * RATE STRUCTURE:
 *   - First 30 minutes:     Free
 *   - 30 min – 2 hours:     $3.00/hour (prorated by minute)
 *   - 2 hours – 6 hours:    $5.00/hour
 *   - 6+ hours:             $8.00/hour
 *   - VIP discount:         20% off the total
 *   - Daily maximum cap:    $40.00
 */

#ifndef BILLING_H
#define BILLING_H

/*
 * calculate_charge — Compute the parking fee.
 * @entry_time: Unix timestamp when the vehicle entered.
 * @exit_time:  Unix timestamp when the vehicle is exiting.
 * @is_vip:     1 if the vehicle is VIP (gets 20% discount), 0 otherwise.
 *
 * Returns the amount to charge (in dollars, as a double).
 */
double calculate_charge(long entry_time, long exit_time, int is_vip);

/*
 * format_duration — Format a duration in seconds to a human-readable string.
 * @seconds: Duration in seconds.
 * @buffer:  Output buffer (at least 32 bytes).
 *
 * Writes something like "2h 15m" or "45m" into buffer.
 */
void format_duration(long seconds, char *buffer);

#endif /* BILLING_H */
