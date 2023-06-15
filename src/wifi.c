//
// Created by Hugo Trippaers on 17/04/2021.
//
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "sdkconfig.h"

#include "common.h"

#define TAG "main_wifi"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_DISCONNECTED_BIT BIT1
#define WIFI_STA_READY_BIT BIT2

static EventGroupHandle_t s_wifi_event_group;
static esp_netif_t *esp_netif;

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGD(TAG,"Event WIFI_EVENT_STA_START");
        xEventGroupSetBits(s_wifi_event_group, WIFI_STA_READY_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGD(TAG,"Event WIFI_EVENT_STA_DISCONNECTED");
        xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGD(TAG,"Event IP_EVENT_STA_GOT_IP");
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t esp32cam_wifi_init(espcam_wifi_config_t *espcam_wifi_config) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &event_handler,
                                               NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                               IP_EVENT_STA_GOT_IP,
                                               &event_handler,
                                               NULL));

    wifi_config_t wifi_config = {
            .sta = {
                    .threshold.authmode = WIFI_AUTH_WPA2_PSK,

                    .pmf_cfg = {
                            .capable = true,
                            .required = false
                    },
            },
    };

    memcpy(wifi_config.sta.ssid, espcam_wifi_config->essid, strlen((char *)espcam_wifi_config->essid));
    memcpy(wifi_config.sta.password, espcam_wifi_config->essid_secret, strlen((char *)espcam_wifi_config->essid_secret));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished, waiting for WIFI_EVENT_STA_START event");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_STA_READY_BIT | WIFI_DISCONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    esp_err_t result = bits & WIFI_STA_READY_BIT ? ESP_OK : ESP_FAIL;
    xEventGroupClearBits(s_wifi_event_group, WIFI_STA_READY_BIT | WIFI_DISCONNECTED_BIT);
    ESP_ERROR_CHECK(result);

    ESP_LOGI(TAG, "Attemting connection to %s", wifi_config.sta.ssid);
    esp_wifi_connect();
    ESP_LOGI(TAG, "wifi_init_connect finished, waiting for WIFI_CONNECTED_BIT event");

    bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    result = bits & WIFI_CONNECTED_BIT ? ESP_OK : ESP_FAIL;
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT);

    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
    esp_event_handler_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP, &event_handler);
    vEventGroupDelete(s_wifi_event_group);

    ESP_LOGI(TAG, "wifi_init completed");
    return result;
}