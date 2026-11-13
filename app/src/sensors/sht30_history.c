/*
 * Fixed-size ring of recent SHT30 samples in normal RAM (linker-allocated).
 * Publishes a mutex-protected snapshot for sensor → control handoff.
 */

#include "sensors/sht30_history.h"

#include <errno.h>
#include <string.h>

static struct sht30_sample buf[SHT30_HISTORY_LEN];
static size_t head;  /* index of newest sample when count > 0 */
static size_t count;

static struct sht30_reading latest;
static struct k_mutex lock;
static struct k_sem data_ready;
static bool inited;

static int16_t avg_temp_unlocked(void)
{
	if (count == 0) {
		return 0;
	}

	int32_t sum = 0;

	for (size_t i = 0; i < count; i++) {
		sum += buf[i].temp_c_x100;
	}
	return (int16_t)(sum / (int32_t)count);
}

static uint16_t avg_hum_unlocked(void)
{
	if (count == 0) {
		return 0;
	}

	uint32_t sum = 0;

	for (size_t i = 0; i < count; i++) {
		sum += buf[i].rh_x100;
	}
	return (uint16_t)(sum / (uint32_t)count);
}

static void publish_unlocked(void)
{
	if (count == 0) {
		latest.valid = false;
		return;
	}

	latest.avg_temp_c_x100 = avg_temp_unlocked();
	latest.avg_rh_x100 = avg_hum_unlocked();
	latest.newest = buf[head];
	latest.seq++;
	latest.valid = true;
}

void sht30_history_init(void)
{
	if (inited) {
		return;
	}

	k_mutex_init(&lock);
	k_sem_init(&data_ready, 0, 1);
	memset(&latest, 0, sizeof(latest));
	head = 0;
	count = 0;
	memset(buf, 0, sizeof(buf));
	inited = true;
}

void sht30_history_reset(void)
{
	if (!inited) {
		sht30_history_init();
	}

	k_mutex_lock(&lock, K_FOREVER);
	head = 0;
	count = 0;
	memset(buf, 0, sizeof(buf));
	memset(&latest, 0, sizeof(latest));
	k_mutex_unlock(&lock);

	/* Drop any stale "data ready" so waiters block for a fresh push. */
	while (k_sem_take(&data_ready, K_NO_WAIT) == 0) {
	}
}

void sht30_history_push(const struct sht30_sample *sample)
{
	if (sample == NULL) {
		return;
	}

	if (!inited) {
		sht30_history_init();
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (count == 0) {
		head = 0;
	} else {
		head = (head + 1) % SHT30_HISTORY_LEN;
	}

	buf[head] = *sample;

	if (count < SHT30_HISTORY_LEN) {
		count++;
	}

	publish_unlocked();
	k_mutex_unlock(&lock);

	k_sem_give(&data_ready);
}

size_t sht30_history_count(void)
{
	size_t n;

	if (!inited) {
		return 0;
	}

	k_mutex_lock(&lock, K_FOREVER);
	n = count;
	k_mutex_unlock(&lock);
	return n;
}

int16_t sht30_history_avg_temp(void)
{
	int16_t avg;

	if (!inited) {
		return 0;
	}

	k_mutex_lock(&lock, K_FOREVER);
	avg = avg_temp_unlocked();
	k_mutex_unlock(&lock);
	return avg;
}

uint16_t sht30_history_avg_hum(void)
{
	uint16_t avg;

	if (!inited) {
		return 0;
	}

	k_mutex_lock(&lock, K_FOREVER);
	avg = avg_hum_unlocked();
	k_mutex_unlock(&lock);
	return avg;
}

int sht30_history_get(size_t i, struct sht30_sample *out)
{
	int err = 0;

	if (out == NULL) {
		return -EINVAL;
	}
	if (!inited) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);
	if (i >= count) {
		err = -EINVAL;
	} else {
		size_t idx = (head + SHT30_HISTORY_LEN - i) % SHT30_HISTORY_LEN;

		*out = buf[idx];
	}
	k_mutex_unlock(&lock);
	return err;
}

int sht30_history_newest(struct sht30_sample *out)
{
	int err = 0;

	if (out == NULL) {
		return -EINVAL;
	}
	if (!inited) {
		return -ENOENT;
	}

	k_mutex_lock(&lock, K_FOREVER);
	if (count == 0) {
		err = -ENOENT;
	} else {
		*out = buf[head];
	}
	k_mutex_unlock(&lock);
	return err;
}

int sht30_history_get_latest(struct sht30_reading *out)
{
	int err = 0;

	if (out == NULL) {
		return -EINVAL;
	}
	if (!inited) {
		return -ENOENT;
	}

	k_mutex_lock(&lock, K_FOREVER);
	if (!latest.valid) {
		err = -ENOENT;
	} else {
		*out = latest;
	}
	k_mutex_unlock(&lock);
	return err;
}

int sht30_history_wait_latest(struct sht30_reading *out, k_timeout_t timeout)
{
	int err;

	if (out == NULL) {
		return -EINVAL;
	}
	if (!inited) {
		sht30_history_init();
	}

	err = k_sem_take(&data_ready, timeout);
	if (err != 0) {
		return -EAGAIN;
	}

	return sht30_history_get_latest(out);
}
