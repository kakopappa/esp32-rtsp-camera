//
// Created by Hugo Trippaers on 19/05/2021.
//
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_err.h>

#include "esp-rtsp.h"
#include "esp-rtsp-priv.h"
#include "esp-rtsp-common.h"

#define TAG "rtsp-server"

static void rtsp_server_task(void *pvParameters) {
    rtsp_server_main();
    vTaskDelete(NULL);
}

esp_err_t esp_rtsp_server_start(esp_rtsp_server_handle_t *handle) {
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_rtsp_server_t *server = calloc(1, sizeof(esp_rtsp_server_t));
    if (!server) {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t result = xTaskCreate(rtsp_server_task, "rtsp_tcp_server", SERVER_STACKSIZE, NULL, SERVER_PRIORITY, &server->server_taskhandle);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create rtsp server task: %d", result);
        free(server);
        return ESP_FAIL;
    }

    server->running = true;
    *handle = server;

    return ESP_OK;
}

esp_err_t esp_rtsp_server_stop(esp_rtsp_server_handle_t handle) {
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_rtsp_server_t *server = (esp_rtsp_server_t *)handle;

    if (server->running) {
        vTaskDelete(server->server_taskhandle);
        server->running = false;
    }

    free(server);

    return ESP_OK;
}