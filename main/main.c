#define BOARD_ESP32CAM_AITHINKER
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include <arpa/inet.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

static const char *TAG = "main";

static void init_4mb_ext() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
}

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

static void configure_station_mode() {
    esp_netif_create_default_wifi_sta();
    wifi_config_t sta_config = {
        .sta = {
            .ssid = "E-306@The_falls_at_riverwoods",
            .password = "PurPenguin306",
        },
    };
    ESP_LOGI(TAG, "Connecting to %s", sta_config.sta.ssid);
    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
}

static void configure_access_point_mode() {
    esp_netif_create_default_wifi_ap();
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP_32_HUB",
            .password = "esp32hub",
            .ssid_len = 0,
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK},
    };
    ESP_LOGI(TAG, "Access point name %s", ap_config.ap.ssid);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
}

static void init_wifi() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_wifi_init(&(wifi_init_config_t)WIFI_INIT_CONFIG_DEFAULT());
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    configure_station_mode();
    configure_access_point_mode();
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_netif_ip_info_t ip_info;
    ip_info.ip.addr = inet_addr("192.168.10.1");
    ip_info.gw.addr = inet_addr("192.168.10.1");
    ip_info.netmask.addr = ipaddr_addr("255.255.255.0");

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    esp_netif_dhcps_stop(netif);
    esp_netif_set_ip_info(netif, &ip_info);
    esp_netif_dhcps_start(netif);
    esp_wifi_start();
    esp_wifi_connect();
}

static esp_err_t post_handler(httpd_req_t *req) {
    char buf[100];
    int received = httpd_req_recv(req, buf, sizeof(buf));
    if (received > 0) {
        buf[received] = '\0';
        ESP_LOGI(TAG, "Received POST data: %s", buf);
        httpd_resp_send(req, "POST received", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_500(req);
    }
    return ESP_OK;
}

static void init_http_server() {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_uri_t uri_post = {
        .uri = "/upload",
        .method = HTTP_POST,
        .handler = post_handler,
        .user_ctx = NULL};
    httpd_start(&server, &config);
    httpd_register_uri_handler(server, &uri_post);
}

void app_main(void) {
    init_4mb_ext();
    init_wifi();
    init_http_server();
    while (1) {
        ESP_LOGI(TAG, "Looping");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}