#define BOARD_ESP32CAM_AITHINKER
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "main";

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Wi-Fi started, trying to connect...");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Connected to Wi-Fi!");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from Wi-Fi, retrying...");
            esp_wifi_connect();
            break;
        default:
            ESP_LOGI(TAG, "Other Wi-Fi event: %" PRId32, event_id);
            break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
        case IP_EVENT_STA_GOT_IP:
            char ip_str[16];
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Got IP: %s", esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str)));
            break;
        }
    }
}

static esp_netif_t *init_wifi() {
    esp_netif_init();
    esp_netif_t *wifi_info = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "E-306@The_falls_at_riverwoods",
            .password = "PurPenguin306",
        },
    };
    ESP_LOGI(TAG, "Will try connecting to %s", wifi_config.sta.ssid);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
    return wifi_info;
}

static esp_err_t init_4mb_ext() {
    nvs_flash_init();
    return ESP_OK;
}

void app_main(void) {
    esp_event_loop_create_default();
    if (ESP_OK != init_4mb_ext()) {
        ESP_LOGI(TAG, "Failed to initialize external 4mb");
        return;
    }
    init_wifi();
    while(1) {
        ESP_LOGI(TAG, "Looping");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}