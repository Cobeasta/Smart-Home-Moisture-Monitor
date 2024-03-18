#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/cdefs.h>

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "dht.h"

#define DHT_TIMER_GROUP 0
#define DHT_TIMER_IDX 0
#define DHT_DATA_BITS 40


int DHTgpio = GPIO_NUM_18;
gptimer_handle_t gptimer;


_Noreturn void dht_check_task(void *pvParameters)
{
    float humidity, temperature;
    while (1)
    {
        printf("Reading DHT22 %lu\n", xTaskGetTickCount());

        dht_read_float_data(DHT_TYPE_AM2301, DHTgpio, &humidity, &temperature);
        temperature = (temperature * 9 / 5) + 32; // convert to fahrenheit

        printf("\tTemperature: %lf\n", temperature);
        printf("\tHumidity: %lf\n", humidity);

        vTaskDelay(portTICK_PERIOD_MS * 50);
    }
}

void app_main(void)
{
    xTaskCreate(dht_check_task, "dht_check_task", 2048, NULL, 0, NULL );
}



