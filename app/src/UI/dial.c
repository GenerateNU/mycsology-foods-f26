#include <stdint.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <stm32f4xx_ll_tim.h>

#include "dial.h"

LOG_MODULE_REGISTER(dial);

// Aliases
#define TIMER_NODE DT_NODELABEL(timers3)
#define QDEC_NODE  DT_NODELABEL(qdec)

static TIM_TypeDef *const tim = (TIM_TypeDef *)DT_REG_ADDR(TIMER_NODE);

static volatile int32_t wraps;

static void qdec_isr(const void *arg)
{
    ARG_UNUSED(arg);

    /* Counter wrapped past ARR or 0 */
    if (LL_TIM_IsActiveFlag_UPDATE(tim)) {
        LL_TIM_ClearFlag_UPDATE(tim);
        if (LL_TIM_GetDirection(tim) == LL_TIM_COUNTERDIRECTION_DOWN) {
            wraps--;
        } else {
            wraps++;
        }
    }
}

int32_t dial_get_position(void)
{
    int32_t w;
    uint32_t cnt;

    /* Re-read if a wrap happened while reading */
    do {
        w = wraps;
        cnt = LL_TIM_GetCounter(tim);
    } while (w != wraps);

    return w * (int32_t)(LL_TIM_GetAutoReload(tim) + 1) + (int32_t)cnt;
}

int init_dial(void) {
    const struct device *qdec = DEVICE_DT_GET(QDEC_NODE);

    if (!device_is_ready(qdec)) {
        LOG_ERR("QDEC not ready");
        return -ENODEV;
    }

    /* The driver has already put TIM3 in encoder mode and set up the pins.
     * Set ARR explicitly so the wrap arithmetic is well defined. */
    LL_TIM_SetAutoReload(tim, 0xFFFF);
    LL_TIM_SetCounter(tim, 0);

    LL_TIM_ClearFlag_UPDATE(tim);
    LL_TIM_EnableIT_UPDATE(tim);

    IRQ_CONNECT(DT_IRQ_BY_NAME(TIMER_NODE, global, irq),
                DT_IRQ_BY_NAME(TIMER_NODE, global, priority),
                qdec_isr, NULL, 0);
    irq_enable(DT_IRQ_BY_NAME(TIMER_NODE, global, irq));

    return 0;
}
