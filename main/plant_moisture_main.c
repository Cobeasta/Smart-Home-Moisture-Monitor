#include <sys/queue.h>
#include <sys/cdefs.h>

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "hal/timer_types.h"

#define DHT_TIMER_GROUP 0
#define DHT_TIMER_IDX 0
#define DHT_DATA_BITS 40

typedef struct
{
    float temperature;
    float humity;
} dht_data;

_Noreturn void main_task(void *pvParameters);

gpio_config_t outCfg = {
        .pin_bit_mask = 1 << 18,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = 0,
        .pull_down_en = 0
};
gpio_config_t inCfg = {
        .pin_bit_mask = 1 << 18,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = 0,
        .pull_down_en = 0
};


int DHTgpio = GPIO_NUM_18;
gptimer_handle_t gptimer;



bool await_signal_change_us(uint64_t timeout, int pin, int expectedValue, uint64_t *duration)
{

    uint64_t timerValue0, timerValue1;

    gptimer_get_raw_count(gptimer, &timerValue0); // get start time

    do
    {
        gptimer_get_raw_count(gptimer, &timerValue1);
        *duration = (timerValue1 - timerValue0); // calculate elapsed time
        if (*duration > timeout)
        {
            return false;
        }
    } while (gpio_get_level(DHTgpio) == expectedValue);

    return true;
}


// free floats at high
// 1a. MCU sends low for at least 1 ms
// 1b. Configure to input, wait 20-40 ms for DHT22 response
// 2a. Sensor pulls low 80 usec
// 2b. Sensor pulls high 80 usec
// 3a. Data transmission low 50 usec
// 3b. Data transmissions high; 26-28 usec means 0, 70 usec means high
_Noreturn void DHT_task(void *pvParameter)
{

    gptimer = NULL;
    gptimer_config_t timer_config = {
            .clk_src = GPTIMER_CLK_SRC_DEFAULT,
            .direction = GPTIMER_COUNT_UP,
            .resolution_hz = 1 * 1000 * 1000 // 1 MHz 1 tick = 1 us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    gptimer_enable(gptimer);
    gptimer_start(gptimer);
    uint64_t uSec;
    while (1)
    {
        printf("Reading\n");
        gpio_config(&outCfg);
//        1a.
        gpio_set_level(DHTgpio, 0); // write 0
        esp_rom_delay_us(3000);

//        1b.
        gpio_set_level(DHTgpio, 1); // return to 1
        esp_rom_delay_us(25);
        gpio_config(&inCfg); // configure to input

      /*  if (!await_signal_change_us(45, DHTgpio, 1, &uSec))
        {
            printf("DHT Task: Timeout step 1b\n");
            continue;
        }// Sensor pulls high 20-40 uSec*/

        if (!await_signal_change_us(85, DHTgpio, 0, &uSec))
        {
            printf("DHT Task: Timeout step 2a\n");
            continue;
        }// Sensor pulls low 80 usec, then goes high

        if (!await_signal_change_us(85, DHTgpio, 1, &uSec))
        {
            printf("DHT Task: Timeout step 2b value: %lld\n", uSec);
            continue;
        }// Sensor pulls high 80 usec, then goes low
        printf("DHT Task: sensor ready %lld\n", uSec);

        uint8_t byteIdx = 0;
        uint8_t bitIdx = 7;
        uint8_t dhtData[DHT_DATA_BITS / 8];

        for (int i = 0; i < DHT_DATA_BITS / 8; i++)
        {
            dhtData[i] = 0;
        }
        for (int i = 0; i < DHT_DATA_BITS; i++)
        {
            if (!await_signal_change_us(55, DHTgpio, 0, &uSec))
            {
                printf("DHT Task: Timeout step 3a\n");
                break;
            } // prepare to send bit
            printf("DHT Task: Received prepare to send: time %lld\n", uSec);
            if (!await_signal_change_us(75, DHTgpio, 1, &uSec))
            {
                printf("DHT Task: long Timeout step 3b\n");
                break;
            }// Data bit sent
            if (uSec > 50 && uSec < 75)
            {
                dhtData[byteIdx] |= (1 << bitIdx);
            } // received a 1

            if (bitIdx == 0)
            {
                bitIdx = 7;
                ++byteIdx;
            } else
            {
                bitIdx--;
            }
        }
        float humidity;
        float temperature;

        humidity = dhtData[0];
        humidity *= 0x100;
        humidity += dhtData[1];
        humidity /= 10;
        printf("Humidity: %f\n", humidity);

        vTaskDelay(portTICK_PERIOD_MS * 500);
    }
}


void app_main(void)
{
    xTaskCreatePinnedToCore(DHT_task, "dht_task", 2048, NULL, 0, NULL, 1);
}



