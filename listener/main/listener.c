/*
Written by Devin Headrick

This is code to be ran on a listener esp32, initialized as a wifi 'access point' (ap) interface.

The listener listens to broadcasted data from the broadcaster, without an initial 'pairing'.

References:
    -
*/

#include <string.h> //for memcpy and memset
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h" // I think the WiFi lib needs nvs flash to store wifi credentials
#include "esp_now.h"

#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define DELAY_1S DELAY(1000)

static const char *TAG = "esp_listener";

static uint8_t esp_now_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void espnow_listener_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    // ESP_LOGI(TAG, "Listener receive callback fired");
}

/// @brief Initalize wifi settings for Access Point (AP) mode, for listener
/// @param void
static void init_app_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/// @brief  Initialize espnow, register receive callback
/// @param  void
/// @return esp_err_t
static esp_err_t espnow_init(void)
{
    ESP_ERROR_CHECK(esp_now_init());

    // Register listener recv callback
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_listener_recv_cb));

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 0;
    peer->ifidx = ESP_IF_WIFI_AP;
    peer->encrypt = false;
    memcpy(peer->peer_addr, esp_now_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    // Create esp now listener task

    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP NOW listener started \n");

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
}