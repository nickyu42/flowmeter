#include "transmit.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "../flow_estimation/messages.h"

#define WIFI_CHANNEL 1

static const char *TAG = "transmit";

static uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

extern MessageBufferHandle_t measurements_msg_buffer;

void msg_receive_callback(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    printf("Received message\n");
}

static void msg_send_callback(const uint8_t *mac, esp_now_send_status_t sendStatus)
{
    switch (sendStatus)
    {
    case ESP_NOW_SEND_SUCCESS:
        break;

    case ESP_NOW_SEND_FAIL:
        break;

    default:
        break;
    }
}

static esp_err_t init(void)
{
    // Initialize NVS, necessary for Wifi
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
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE));

    // Initialize espnow
    ESP_ERROR_CHECK(esp_now_init());

    // Add peer
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

void TransmitDataTask(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting TransmitDataTask()");

    ESP_LOGI(TAG, "Init wifi");
    ESP_ERROR_CHECK(init());

    uint8_t received_msg[MEASUREMENTS_MESSAGE_SIZE];
    size_t received_bytes;

    for (;;)
    {
        received_bytes = xMessageBufferReceive(
            measurements_msg_buffer,
            received_msg,
            MEASUREMENTS_MESSAGE_SIZE,
            portMAX_DELAY);

        configASSERT(received_bytes == MEASUREMENTS_MESSAGE_SIZE);

        esp_err_t status = esp_now_send(broadcast_mac, received_msg, MEASUREMENTS_MESSAGE_SIZE);
        ESP_ERROR_CHECK(status);
    }

    vTaskDelete(NULL);
}