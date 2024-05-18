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
#include "driver/gpio.h"
#include "esp_timer.h"

#define GPIO_INPUT_IO_0 23 // pin23 on the esp32 WROOM-32 dev board module
#define GPIO_INPUT_SEL 1ULL << GPIO_INPUT_IO_0

#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define DELAY_1S DELAY(1000)

#define ESP_INTR_FLAG_DEFAULT 0

static const char *TAG = "esp_listener";

static QueueHandle_t gpio_evt_queue = NULL;

static uint8_t esp_now_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

int64_t time_start = 0;
int64_t time_stop = 0;

//ISR callback function for  Gpio interrupts
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    //start timer upon gpio toggle - start timer with longer timeout that  
    //ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, 5000000));

    time_start = esp_timer_get_time();

    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/// @brief To keep interrupt as short as possible, we use a queue to send the gpio number to the main task for print debugging
/// @param arg 
static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

void init_gpios(void)
{
    gpio_config_t io_conf = {};
    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    // bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio interrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
}

static void espnow_listener_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    // ESP_LOGI(TAG, "Listener receive callback fired");

    //stop timer upon broadcast recevied - call timer callback 
    time_stop = esp_timer_get_time();

    int64_t time_for_broadcast_recv = time_stop - time_start;

    ESP_LOGI(TAG, "Time between gpio and broadcast: %lld", time_for_broadcast_recv);
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

    init_gpios();
}