#include <stdint.h>
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

static int32_t qdec_position(void);
void dial_thread_entry(void *p1, void *p2, void *p3);

static int32_t dial_value_diff;
static volatile int32_t wraps;
static K_SEM_DEFINE(edge_sem, 0, 1);
K_THREAD_DEFINE(dial_thread, 1024, dial_thread_entry, NULL, NULL, NULL, 7, 0, SYS_FOREVER_MS);


void dial_thread_entry(void *p1, void *p2, void *p3) {
    int32_t last_dial_value = 0;
    int32_t dial_value;
    while (1) {
        k_sem_take(&edge_sem, K_FOREVER);
        dial_value = qdec_position();
        dial_value_diff = dial_value - last_dial_value;
        LOG_INF("pos = %d", dial_value);
        last_dial_value = dial_value;
    }
}

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

    /* Edge on channel A (TI1) - fires on encoder movement */
    if (LL_TIM_IsActiveFlag_CC1(tim)) {
        LL_TIM_ClearFlag_CC1(tim);
        k_sem_give(&edge_sem);
    }
}

static int32_t qdec_position(void)
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

const int *init_dial() {
    const struct device *qdec = DEVICE_DT_GET(QDEC_NODE);

    if (!device_is_ready(qdec)) {
        LOG_ERR("QDEC not ready");
        return 0;
    }

    /* The driver has already put TIM3 in encoder mode and set up the pins.
     * Set ARR explicitly so the wrap arithmetic is well defined. */
    LL_TIM_SetAutoReload(tim, 0xFFFF);
    LL_TIM_SetCounter(tim, 0);

    LL_TIM_ClearFlag_UPDATE(tim);
    LL_TIM_ClearFlag_CC1(tim);
    LL_TIM_EnableIT_UPDATE(tim);
    LL_TIM_EnableIT_CC1(tim);      /* drop this if you only want wrap IRQs */

    IRQ_CONNECT(DT_IRQ_BY_NAME(TIMER_NODE, global, irq),
                DT_IRQ_BY_NAME(TIMER_NODE, global, priority),
                qdec_isr, NULL, 0);
    irq_enable(DT_IRQ_BY_NAME(TIMER_NODE, global, irq));

    k_thread_start(dial_thread);
    return &dial_value_diff;
}