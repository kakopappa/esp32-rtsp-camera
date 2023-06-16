#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <esp_ota_ops.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include  <lwip/apps/sntp.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "esp-rtsp.h"
#include "common.h" 

#define TAG "main"

#define WIFI_NAME ""
#define WIFI_PASSWORD ""

static app_config_t app_config;

void app_main() {
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free heap memory: %d bytes (%d internal)", esp_get_free_heap_size(), esp_get_free_internal_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    esp_log_level_set("*", ESP_LOG_VERBOSE);

    // Initialize Event Loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Call our own init functions
    ESP_LOGD(TAG, "[PRE esp32cam_wifi_init] Free internal heap  %d bytes", esp_get_free_internal_heap_size());
    app_config.wifi_config.essid = (unsigned char *)WIFI_NAME;
    app_config.wifi_config.essid_secret = (unsigned char *)WIFI_PASSWORD;

    ESP_ERROR_CHECK(esp32cam_wifi_init(&app_config.wifi_config));
    ESP_LOGD(TAG, "[POST esp32cam_wifi_init] Free internal heap  %d bytes", esp_get_free_internal_heap_size());

    ESP_LOGD(TAG, "[PRE esp32cam_camera_init] Free internal heap  %d bytes", esp_get_free_internal_heap_size());
    ESP_ERROR_CHECK(esp32cam_camera_init());
    ESP_LOGD(TAG, "[POST esp32cam_camera_init] Free internal heap  %d bytes", esp_get_free_internal_heap_size());
     
    esp_rtsp_server_handle_t rtsp_server_handle;
    ESP_ERROR_CHECK(esp_rtsp_server_start(&rtsp_server_handle));
    ESP_LOGD(TAG, "[POST esp_rtsp_server_start] Free internal heap  %d bytes", esp_get_free_internal_heap_size());
}

// pio run -t menuconfig
// platformio device monitor -b 115200 