/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "driver/i2c.h"

#include "INA219/INA219.h"
#include "flow_estimation/circular_buffer.h"
#include "flow_estimation/messages.h"
#include "transmit/transmit.h"

// Voltage level at which the system powers down
#define CRITICAL_VOLTAGE_mV 7000

#define CURRENT_SENSOR_READ_DELAY_MS 10

static const char *TAG = "app_main";

extern MessageBufferHandle_t measurements_msg_buffer;

double estimate_flowrate(int16_t power_mean)
{
    double a = 148.450205494820409057865617796779;
    double b = -826.204954144229304802138358354568;
    return (power_mean - b) / a;
}

void CurrentReadTask(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting CurrentReadTask()");

    ESP_LOGI(TAG, "Intializing I2C");
    ina219_i2c_master_init();

    ESP_LOGI(TAG, "Calibrating current sensor");
    ina219_calibrate_16v_400mA();

    TickType_t xLastWakeTime;
    TickType_t xFrequency = CURRENT_SENSOR_READ_DELAY_MS / portTICK_PERIOD_MS;

    xLastWakeTime = xTaskGetTickCount();

    size_t critical_voltage_samples = 0;

    float current_flow_rate = 0; // l/m
    float cumulative_flow = 0;   // l

    int16_t busvoltage;
    int16_t power;
    int16_t current;
    int16_t shuntvoltage;

    MeasurementsMessage msg;
    uint8_t packed_measurements_msg[MEASUREMENTS_MESSAGE_SIZE];

    ESP_LOGI(TAG, "Start measuring");
    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        busvoltage = ina219_read_bus_voltage_mV();
        power = ina219_read_power_raw();
        current = ina219_read_current_raw();
        shuntvoltage = ina219_read_shunt_voltage_raw();

        if (busvoltage <= CRITICAL_VOLTAGE_mV)
        {
            critical_voltage_samples++;
        }
        else if (critical_voltage_samples > 0)
        {
            critical_voltage_samples--;
        }

        circ_buffer_push(power);
        current_flow_rate = estimate_flowrate(circ_buffer_mean());
        cumulative_flow += (current_flow_rate / 60 / 1000) * CURRENT_SENSOR_READ_DELAY_MS;

        // printf("%f %d %d\n", current_flow_rate, power, busvoltage);

        msg.busvoltage = busvoltage;
        msg.power = power;
        msg.current = current;
        msg.shuntvoltage = shuntvoltage;
        msg.cumulative_flow = cumulative_flow;
        msg.flow_rate = current_flow_rate;

        pack_measurements_message(packed_measurements_msg, &msg);

        xMessageBufferSend(measurements_msg_buffer, (void *)packed_measurements_msg, MEASUREMENTS_MESSAGE_SIZE, 0);
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello world!");

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), WiFi%s%s, ",
             CONFIG_IDF_TARGET,
             chip_info.cores,
             (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    // uint32_t flash_size;
    // ESP_LOGI(TAG, "silicon revision %d, ", chip_info.revision);
    // if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Get flash size failed");
    //     return;
    // }

    // ESP_LOGI(TAG, "%uMB %s flash", flash_size / (1024 * 1024),
    //        (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    // ESP_LOGI(TAG, "Minimum free heap size: %d bytes", esp_get_minimum_free_heap_size());

    // Message buffer for intertask communication
    measurements_msg_buffer = xMessageBufferCreate(MEASUREMENTS_MESSAGE_BUFFER_SIZE);

    ESP_LOGI(TAG, "Starting read task");
    TaskHandle_t current_read_task = NULL;
    xTaskCreatePinnedToCore(
        CurrentReadTask,
        "CurrentReadTask",
        configMINIMAL_STACK_SIZE * 5,
        NULL,
        (tskIDLE_PRIORITY + 2) | portPRIVILEGE_BIT,
        &current_read_task,
        0);

    ESP_LOGI(TAG, "Starting transmit task");
    TaskHandle_t transmit_task = NULL;
    xTaskCreatePinnedToCore(
        TransmitDataTask,
        "TransmitDataTask",
        configMINIMAL_STACK_SIZE * 5,
        NULL,
        (tskIDLE_PRIORITY + 1) | portPRIVILEGE_BIT,
        &transmit_task,
        1);

    ESP_LOGI(TAG, "Exiting app_main()");
}
