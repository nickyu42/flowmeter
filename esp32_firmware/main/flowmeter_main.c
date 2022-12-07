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
#include "driver/gpio.h"
#include "esp_sleep.h"

#include "INA219/INA219.h"
#include "flow_estimation/circular_buffer.h"
#include "flow_estimation/messages.h"
#include "transmit/transmit.h"

// Voltage level at which the system powers down
#define CRITICAL_VOLTAGE_mV 7000

#define CURRENT_SENSOR_READ_DELAY_MS 10

#define LED_PIN 2

static const char *TAG = "app_main";

extern MessageBufferHandle_t measurements_msg_buffer;

double estimate_flowrate(int16_t power_mean)
{
    double a = 148.450205494820409057865617796779;
    double b = -826.204954144229304802138358354568;
    return (power_mean - b) / a;
}

double estimate_flowrate_esp(int16_t power_mean)
{
    static double a = 74.663536576456507987131772097200;
    static double b = -179.669045694531405388261191546917;
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

    // gpio_set_level(LED_PIN, 1);

    uint8_t led = 0;

    char msg_json_str[200];

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
        current_flow_rate = estimate_flowrate_esp(circ_buffer_mean());
        cumulative_flow += (current_flow_rate / 60 / 1000) * CURRENT_SENSOR_READ_DELAY_MS;

        msg.busvoltage = busvoltage;
        msg.power = power;
        msg.current = current;
        msg.shuntvoltage = shuntvoltage;
        msg.cumulative_flow = cumulative_flow;
        msg.flow_rate = current_flow_rate;

        pack_measurements_message(packed_measurements_msg, &msg);

        measurements_message_to_str(msg_json_str, &msg);
        printf("%s\n", msg_json_str);

        xMessageBufferSend(measurements_msg_buffer, (void *)packed_measurements_msg, MEASUREMENTS_MESSAGE_SIZE, 0);
    }

    vTaskDelete(NULL);
}

static esp_err_t init_gpio(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << LED_PIN | 1ULL << 18;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    return gpio_config(&io_conf);
}

RTC_DATA_ATTR int bootCount = 0;

void app_main(void)
{
    // ++bootCount;
	// if (bootCount == 1) { // if first boot...
	//     esp_sleep_enable_timer_wakeup(1000);
	//     esp_deep_sleep_start();  
  	// }

    ESP_LOGI(TAG, "Hello world!");

    // esp_chip_info_t chip_info;
    // esp_chip_info(&chip_info);
    // ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), WiFi%s%s, ",
    //          CONFIG_IDF_TARGET,
    //          chip_info.cores,
    //          (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
    //          (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    // Init GPIO
    ESP_ERROR_CHECK(init_gpio());

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

    // ESP_LOGI(TAG, "Starting transmit task");
    // TaskHandle_t transmit_task = NULL;
    // xTaskCreatePinnedToCore(
    //     TransmitDataTask,
    //     "TransmitDataTask",
    //     configMINIMAL_STACK_SIZE * 5,
    //     NULL,
    //     (tskIDLE_PRIORITY + 1) | portPRIVILEGE_BIT,
    //     &transmit_task,
    //     0);

    ESP_LOGI(TAG, "Exiting app_main()");
}
