#ifndef SHT30_HISTORY_H
#define SHT30_HISTORY_H

#include "sensors/sht30.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <zephyr/kernel.h>

/** Number of samples kept for sliding-window PID / other consumers. */
#define SHT30_HISTORY_LEN 10

/**
 * Ring buffer of recent SHT30 samples (temp + RH paired per slot),
 * plus a mutex-protected snapshot for cross-thread handoff.
 *
 * Index convention for getters:
 *   i = 0 → newest
 *   i = count-1 → oldest still stored
 *
 * Producer (sensor path):
 *   sht30_read → sht30_history_push  (publishes snapshot, signals waiters)
 *
 * Consumer (control / UI):
 *   sht30_history_get_latest()           — non-blocking copy
 *   sht30_history_wait_latest(timeout)   — block until a new publish
 */

/** Coherent snapshot published after each successful push. */
struct sht30_reading {
	int16_t avg_temp_c_x100;
	uint16_t avg_rh_x100;
	struct sht30_sample newest;
	uint32_t seq;   /* increments on every publish */
	bool valid;     /* false until the first push */
};

/** Init mutex / sem. Call once before any other history API. */
void sht30_history_init(void);

/** Clear the history and invalidate the published snapshot. */
void sht30_history_reset(void);

/** Append one sample, publish averages, signal waiters. */
void sht30_history_push(const struct sht30_sample *sample);

/** How many valid samples are stored (0 … SHT30_HISTORY_LEN). */
size_t sht30_history_count(void);

/** Average temperature of all stored samples (locked). */
int16_t sht30_history_avg_temp(void);

/** Average humidity of all stored samples (locked). */
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

/**
 * Copy the latest published snapshot into *out (non-blocking).
 *
 * @return 0 on success, -ENOENT if nothing published yet, -EINVAL if out is NULL
 */
int sht30_history_get_latest(struct sht30_reading *out);

/**
 * Wait until a new sample is published, then copy the snapshot into *out.
 *
 * @return 0 on success, -EAGAIN on timeout, -EINVAL if out is NULL
 */
int sht30_history_wait_latest(struct sht30_reading *out, k_timeout_t timeout);

#endif /* SHT30_HISTORY_H */
