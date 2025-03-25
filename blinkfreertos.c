#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "hardware/adc.h"



QueueHandle_t dataQueue; // Fila para comunicação entre as tasks

void setup(void)
{   
    stdio_init_all();
    sleep_ms(2000); // Espera inicial para USB Serial
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // Entrada 4 é o sensor de temperatura
}

float read_onboard_temperature()
{
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t raw = adc_read();
    float voltage = raw * conversion_factor;
    float tempC = 27.0f - (voltage - 0.706f) / 0.001721f; // Fórmula do RP2040
    return tempC;
}

void vSensorTask(void *pvParameters)
{
    float sensorData;
    for (;;)
    {
        sensorData = read_onboard_temperature();
        xQueueSend(dataQueue, &sensorData, portMAX_DELAY);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void vProcessingTask(void *pvParameters)
{
    float receivedData;
    static uint cnt = 0;
    static float avg = 0;

    for (;;)
    {
        if (xQueueReceive(dataQueue, &receivedData, portMAX_DELAY))
        {
            avg += receivedData;
            cnt++;
            if (cnt == 10)
            {   
                avg /= 10;
                printf("Average temperature: %.2f°C\n", avg);
                cnt = 0;
                avg = 0;
            }
        }
    }
}

int main()
{
    setup();

    // Criar fila para comunicação entre tasks
    dataQueue = xQueueCreate(5, sizeof(float));

    if (dataQueue != NULL)
    {
        xTaskCreate(vSensorTask, "SensorTask", 128, NULL, 1, NULL);
        xTaskCreate(vProcessingTask, "ProcessingTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        vTaskStartScheduler(); // Inicia o escalonador do FreeRTOS
    }
    else
    {
        printf("Erro: Falha ao criar fila!\n");
    }

    while (1);
    return 0;
}
