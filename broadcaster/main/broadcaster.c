#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h" // I think the WiFi lib needs nvs flash to store wifi credentials

#include "esp_now.h"

#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define DELAY_1S DELAY(1000)

static void esp_now_broadcast_task(void *pvParameter);

static const char *TAG = "esp_broadcaster";

static void init_app_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void espnow_broadcast_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    printf("broadcast send callback fired \n");
}

/// @brief  Task that loops, constantly broadcasting data to any listening devices
/// @param pvParameter
static void esp_now_broadcast_task(void *pvParameter)
{

    for (;;)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        printf("Sending broadcast data\n");
    }
}

static esp_err_t espnow_init(void)
{

    ESP_ERROR_CHECK(esp_now_init());

    // register a callback function - this is fired upon calling the esp_now_send() fxn
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_broadcast_send_cb));

    xTaskCreate(
        esp_now_broadcast_task,
        "esp_now_broadcast_task",
        2048,
        NULL,
        1,
        NULL);

    return ESP_OK;
}

void app_main(void)
{
    printf("Starting ESP NOW broadcaster \n");

    // init NVS (non volatile storage) - seems like wifi needs this to store credentials
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // init wifi
    init_app_wifi();

    // init esp now
    espnow_init();

    // create task for esp now broadcast

    while (1)
    {
        ESP_LOGE(TAG, "hello esp :) ");
        DELAY_1S;
    }
}
