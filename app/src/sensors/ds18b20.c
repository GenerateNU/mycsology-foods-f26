/*
 * DS18B20 driver (application-level 1-Wire bit-bang — built step by step).
 *
 * Right now this file is only a skeleton so the project still builds.
 * Next steps fill in: GPIO ready check → bit primitives → commands →
 * CRC → temp math → full ds18b20_read().
 */

#include "sensors/ds18b20.h"

#include <errno.h>
#include <zephyr/logging/log.h>

/* Register this .c file with Zephyr's logging system.
 * LOG_MODULE_REGISTER goes in exactly ONE .c file (never in a header).
 * After this, LOG_ERR / LOG_DBG / etc. are tagged as "ds18b20" in the console.
 */
LOG_MODULE_REGISTER(ds18b20, LOG_LEVEL_INF);

int ds18b20_read(const struct gpio_dt_spec *dq,
		 enum ds18b20_resolution res,
		 struct ds18b20_sample *out)
{
	/* Fail fast on bad caller arguments — same idea as sht30_read(). */
	if (dq == NULL || out == NULL) {
		return -EINVAL;
	}

	/* Silence "unused parameter" until later steps use res. */
	(void)res;

	/* Not implemented yet — we only locked the public API in this step. */
	LOG_ERR("ds18b20_read: not implemented yet");
	return -ENOSYS;
}
