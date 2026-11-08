#ifndef SHT30_HISTORY_H
#define SHT30_HISTORY_H

#include "sensors/sht30.h"

#include <stddef.h>
#include <stdint.h>

/** Number of samples kept for sliding-window PID / other consumers. */
#define SHT30_HISTORY_LEN 10

/**
 * Ring buffer of recent SHT30 samples (temp + RH paired per slot).
 *
 * Index convention for getters:
 *   i = 0 → newest
 *   i = count-1 → oldest still stored
 *
 * No timestamps yet — add later if PID needs them.
 */

/** Clear the history (count → 0). */
void sht30_history_reset(void);

/** Append one sample. Overwrites oldest when full. */
void sht30_history_push(const struct sht30_sample *sample);

/** How many valid samples are stored (0 … SHT30_HISTORY_LEN). */
size_t sht30_history_count(void);

/** Average temperature of all stored samples. */
int16_t sht30_history_avg_temp(void);

/** Average humidity of all stored samples. */
uint16_t sht30_history_avg_hum(void);

/**
 * Copy sample at index i into *out.
 *
 * @return 0 on success, -EINVAL if i is out of range or out is NULL
 */
int sht30_history_get(size_t i, struct sht30_sample *out);

/**
 * Copy the newest sample into *out.
 *
 * @return 0 on success, -ENOENT if history is empty, -EINVAL if out is NULL
 */
int sht30_history_newest(struct sht30_sample *out);

#endif /* SHT30_HISTORY_H */
