/*
 * Fixed-size ring of recent SHT30 samples in normal RAM (linker-allocated).
 * Not bare-metal absolute SRAM addresses — Zephyr owns the memory map.
 */

#include "sensors/sht30_history.h"

#include <errno.h>
#include <string.h>

static struct sht30_sample buf[SHT30_HISTORY_LEN];
static size_t head;  /* index of newest sample when count > 0 */
static size_t count;

void sht30_history_reset(void)
{
	head = 0;
	count = 0;
	memset(buf, 0, sizeof(buf));
}

int16_t sht30_history_avg_temp(void)
{
	if (count == 0) {
		return 0;
	}

	int32_t avgt = 0;
	for (size_t i = 0; i < count; i++) {
		avgt += buf[i].temp_c_x100;
	}
	return (int16_t)(avgt / (int32_t)count);
}

uint16_t sht30_history_avg_hum(void)
{
	if (count == 0) {
		return 0;
	}

	uint32_t avg_h = 0;
	for (size_t i = 0; i < count; i++) {
		avg_h += buf[i].rh_x100;
	}
	return (uint16_t)(avg_h / (uint32_t)count);
}

void sht30_history_push(const struct sht30_sample *sample)
{
	if (sample == NULL) {
		return;
	}

	if (count == 0) {
		head = 0;
	} else {
		head = (head + 1) % SHT30_HISTORY_LEN;
	}

	buf[head] = *sample;

	if (count < SHT30_HISTORY_LEN) {
		count++;
	}
}

size_t sht30_history_count(void)
{
	return count;
}

int sht30_history_get(size_t i, struct sht30_sample *out)
{
	if (out == NULL || i >= count) {
		return -EINVAL;
	}

	/* i=0 newest → walk backward from head */
	size_t idx = (head + SHT30_HISTORY_LEN - i) % SHT30_HISTORY_LEN;
	*out = buf[idx];
	return 0;
}

int sht30_history_newest(struct sht30_sample *out)
{
	if (out == NULL) {
		return -EINVAL;
	}
	if (count == 0) {
		return -ENOENT;
	}

	*out = buf[head];
	return 0;
}
