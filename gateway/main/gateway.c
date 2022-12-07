#include <stdio.h>

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#define WIFI_CHANNEL 1

static const char *TAG = "transmit";

static uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

MessageBufferHandle_t msg_buffer;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define EXAMPLE_ESP_WIFI_SSID "Skynet"
#define EXAMPLE_ESP_WIFI_PASS "youshallnotpass"

#define NODEMCU_LED_PIN 2

static EventGroupHandle_t s_wifi_event_group;

typedef struct MeasurementsMessage_t
{
    int16_t busvoltage;
    int16_t power;
    int16_t current;
    int16_t shuntvoltage;
    float cumulative_flow;
    float flow_rate;
} MeasurementsMessage;

void unpack_measurements_message(uint8_t *buffer, MeasurementsMessage *msg)
{
    msg->busvoltage = ((int16_t)buffer[1] << 8) | buffer[0];
    msg->power = ((int16_t)buffer[3] << 8) | buffer[2];
    msg->current = ((int16_t)buffer[5] << 8) | buffer[4];
    msg->shuntvoltage = ((int16_t)buffer[7] << 8) | buffer[6];

    uint32_t cumulative_flow = (((uint32_t)buffer[8] << 0) |
                                ((uint32_t)buffer[9] << 8) |
                                ((uint32_t)buffer[10] << 16) |
                                ((uint32_t)buffer[11] << 24));

    msg->cumulative_flow = *((float *)&cumulative_flow);

    uint32_t flow_rate = (((uint32_t)buffer[12] << 0) |
                          ((uint32_t)buffer[13] << 8) |
                          ((uint32_t)buffer[14] << 16) |
                          ((uint32_t)buffer[15] << 24));

    msg->flow_rate = *((float *)&flow_rate);
}

void measurements_message_to_str(char *str, MeasurementsMessage *msg)
{
    sprintf(
        str,
        "{\"busvoltage\":%d,\"power\":%d,\"current\":%d,\"shuntvoltage\":%d,\"cumulative_flow\":%f,\"flow_rate\":%f}",
        msg->busvoltage,
        msg->power,
        msg->current,
        msg->shuntvoltage,
        msg->cumulative_flow,
        msg->flow_rate);
}

void msg_receive_callback(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    xMessageBufferSend(msg_buffer, (void *)data, len, 0);
}

void PrintTask() 
{
    size_t received_bytes;
    uint8_t data[16];

    MeasurementsMessage msg;
    char msg_json_str[200];

    for (;;) 
    {
        received_bytes = xMessageBufferReceive(
            msg_buffer,
            data,
            16,
            portMAX_DELAY);

        configASSERT(received_bytes == 16);

        unpack_measurements_message(data, &msg);
        measurements_message_to_str(msg_json_str, &msg);
        printf("%s\n", msg_json_str);

        taskYIELD();
    }
}

static void msg_send_callback(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
}

static esp_err_t init(void)
{
    // Initialize NVS, necessary for Wifi
    ESP_LOGI(TAG, "Init NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Wifi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Initialize espnow
    ESP_ERROR_CHECK(esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_LOGI(TAG, "Init espnow");
    ESP_ERROR_CHECK(esp_now_init());

    // Add peer
    ESP_LOGI(TAG, "Add espnow peer");
    esp_now_peer_info_t peer_info;
    peer_info.channel = WIFI_CHANNEL;
    peer_info.ifidx = ESP_IF_WIFI_STA;
    peer_info.encrypt = false;
    memcpy(peer_info.peer_addr, broadcast_mac, 6);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));

    // Register received message callback
    ESP_ERROR_CHECK(esp_now_register_recv_cb(msg_receive_callback));
    ESP_ERROR_CHECK(esp_now_register_send_cb(msg_send_callback));

    return ESP_OK;
}

static void init_gpio(void) {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL << NODEMCU_LED_PIN;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

void HeartbeatTask() 
{
    init_gpio();

    uint8_t led = 0;
    for (;;) 
    {
        led = !led;
        gpio_set_level(NODEMCU_LED_PIN, led);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    msg_buffer = xMessageBufferCreate(512);
    s_wifi_event_group = xEventGroupCreate();

    ESP_LOGI(TAG, "Init wifi");
    ESP_ERROR_CHECK(init());

    TaskHandle_t print_task = NULL;
    xTaskCreatePinnedToCore(
        PrintTask,
        "PrintTask",
        configMINIMAL_STACK_SIZE * 5,
        NULL,
        (tskIDLE_PRIORITY + 1) | portPRIVILEGE_BIT,
        &print_task,
        1);

    TaskHandle_t heartbeat_task = NULL;
    xTaskCreate(
        HeartbeatTask,
        "HeartbeatTask",
        configMINIMAL_STACK_SIZE * 5,
        NULL,
        (tskIDLE_PRIORITY + 1) | portPRIVILEGE_BIT,
        &heartbeat_task);
}
