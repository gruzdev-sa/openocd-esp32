/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

#define LOG_LOCAL_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include "esp_log.h"
const static char *TAG = "ut_app";

volatile int run_test = 0;

void blink_task(void *pvParameter)
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while(1) {
        ESP_LOGI(TAG, "Toggle LED");
        /* Blink off (output low) */
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

/* This test calls functions recursively many times, exhausing the
 * register space and triggering window overflow exceptions.
 * Upon returning, it triggers window underflow exceptions.
 * If the test passes, then OpenOCD and GDB can both handle
 * window exceptions correctly.
 */
int sum;  // not static so the whole loop is not optimized away

static void recursive(int levels)
{
    if (levels - 1 == 0) {
        return;
    }
    sum += levels;
    recursive(levels - 1);
}

void window_exception_test(void* arg)
{
    recursive(20);
    printf("sum=%d\n",sum);
}

void app_main()
{
    ESP_LOGI(TAG, "Run test %d\n", run_test);
    switch(run_test){
        case 100:
            xTaskCreate(&blink_task, "blink_task", 2048, NULL, 5, NULL);
            break;
        case 200:
            xTaskCreate(&window_exception_test, "win_exc_task", 8192, NULL, 5, NULL);
            break;
        default:
            ESP_LOGE(TAG, "Invalid test id (%d)!", run_test);
            while(1){
              vTaskDelay(1);
            }
    }
}