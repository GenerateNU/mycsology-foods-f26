// #include <stdio.h>
#include <zephyr/kernel.h>
// zephyr/device.h is included eventually through kernel.h
#include <zephyr/drivers/gpio.h>

// This is accessing the DeviceTree node through its label (can see the entire devicetree in generated zephyr.dts file in the build directory)
#define LED2 DT_ALIAS(led0)

// 'gpios' refers to the property name within the led0 node, which is itself inside the leds parent node in the devicetree
// you can see this^^ in code if you search for the 'zephyr/boards/nordic/nrf54l15dk/nrf54l15dk_common.dtsi' file
static const struct gpio_dt_spec led_2 = GPIO_DT_SPEC_GET(LED2, gpios);

int main(void)
{
    // led_0.port is used because you need to check if the gpio controller itself is ready to be used, if you just do led_0.pin you will get a number like '9'
    if (!device_is_ready(led_2.port))
    {
        return 0;
    }

    int state = 0;

    int ret;
    // configuration of led_0 pin
    ret = gpio_pin_configure_dt(&led_2, GPIO_OUTPUT);

    if (ret != 0)
    {
        return 0;
    }

    while (true)
    {
        state = !state;
        ret = gpio_pin_set_dt(&led_2, state);
        if (ret != 0)
        {
            return 0;
        }
        printk("LED is now in state: %d", state);

        k_msleep(4000);
    }

    return 0;
}
