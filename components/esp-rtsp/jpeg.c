//
// Created by Hugo Trippaers on 19/05/2021.
//

/* This file contains some stuff to manipulate JPEG images
 * so they can be used in an RTP stream.
 *
 * Original source: https://github.com/bnbe-club/rtsp-video-streamer-diy-14/blob/master/diy-e14/src/CStreamer.cpp
 */

#include <esp_log.h>
#include <esp_err.h>

#include "rtp-udp.h"

#define TAG "esp-rtsp-jpeg"

#define JPEG_SOI 0xD8
#define JPEG_EOI 0xD9
#define JPEG_SOS 0xDA
#define JPEG_DQT 0xDB

static int find_jpeg_marker(char *buffer, size_t len, uint8_t marker, char **marker_start);
static int block_length(const char *blockptr, size_t len);

esp_err_t esp_rtsp_jpeg_decode(char *buffer, size_t length, esp_rtsp_jpeg_data_t *rtsp_jpeg_data) {
    assert(rtsp_jpeg_data != NULL);

    if (length < 4) {
        ESP_LOGE(TAG, "Invalid length: %d", length);
        return ESP_FAIL;
    }

    // Check magic
    if ((uint8_t)buffer[0] != 0xFF && (uint8_t)buffer[1] != JPEG_SOI) {
        ESP_LOGE(TAG, "Probably not a JPEG");
        return ESP_FAIL;
    }

    char *marker;
    size_t remaining;
    if (find_jpeg_marker(buffer, length, JPEG_DQT, &marker) < 0) {
        ESP_LOGE(TAG, "Failed to find marker 0x%02x", JPEG_DQT);
        return ESP_FAIL;
    }
    rtsp_jpeg_data->quant_table_0 = marker;

    remaining = length - (marker-buffer);
    char *next = marker + block_length(marker, remaining);
    if (find_jpeg_marker(next, length - (next-buffer), JPEG_DQT, &marker) < 0) {
        ESP_LOGE(TAG, "Failed to find marker 0x%02x", JPEG_DQT);
        return ESP_FAIL;
    }
    rtsp_jpeg_data->quant_table_1 = marker;

    remaining = length - (marker-buffer);
    next = marker + block_length(marker, remaining);
    if (find_jpeg_marker(next, length - (next-buffer), JPEG_SOS, &marker) < 0) {
        ESP_LOGE(TAG, "Failed to find marker 0x%02x", JPEG_SOS);
        return ESP_FAIL;
    }
    rtsp_jpeg_data->jpeg_data_start = marker + block_length(marker, remaining); // Don't include the SOS header

    remaining = length - (marker-buffer);
    next = marker + block_length(marker, remaining);
    if (find_jpeg_marker(next, length - (next-buffer), JPEG_EOI, &marker) < 0) {
        ESP_LOGE(TAG, "Failed to find marker 0x%02x", JPEG_EOI);
        return ESP_FAIL;
    }
    rtsp_jpeg_data->jpeg_data_length = marker - rtsp_jpeg_data->jpeg_data_start;

//    ESP_LOGD(TAG, "JPEG: Q1 : %d, Q2 : %d, SOS: %d, LEN: %d",
//             rtsp_jpeg_data->quant_table_0 - buffer,
//             rtsp_jpeg_data->quant_table_1 - buffer,
//             rtsp_jpeg_data->jpeg_data_start - buffer,
//             rtsp_jpeg_data->jpeg_data_length);

    return ESP_OK;
}

static int block_length(const char *blockptr, size_t len) {
    assert(len > 4);
    assert((uint8_t)blockptr[0] == 0xff);

    return 2 + (blockptr[2] << 8 | blockptr[3]);
}

static int find_jpeg_marker(char *buffer, size_t len, uint8_t marker, char **marker_start) {
    long position = 0;
    while(position < len - 1) {
        char *current = buffer+position;

        uint8_t framing = current[0];
        uint8_t typecode = current[1];

        if(framing != 0xff) {
            position += 1;
            continue;
        }

        if(typecode == 0x00 || typecode == 0xFF) {
            // red herring, skip
            position += 2;
            continue;
        }

        if(typecode == marker) {
            *marker_start = current;
            return 0;
        }

        int skip_len;
        switch(typecode) {
            case 0xd8:   // start of image
            case 0xd9:   // end of image
                skip_len = 2;
                break;

            case 0xe0:   // app0
            case 0xdb:   // dqt
            case 0xc4:   // dht
            case 0xc0:   // sof0
            case 0xda:   // sos
            {
                /* If we are not interested in these blocks, try to fast forward
                 * by moving the position up.
                 */
                if (len - position >= 2) {
                    // We need to be sure the length bytes are available in the buffer
                    skip_len = (current[2] << 8 | current[3]) + 2;
                } else {
                    skip_len = 2;
                }
                break;
            }
            default:
                skip_len = 2;
                ESP_LOGE(TAG, "Unexpected jpeg typecode 0x%x\n", typecode);
                break;
        }

        position += skip_len;
    }

    ESP_LOGE(TAG, "Failed to find jpeg marker 0x%x", marker);
    return -1;
}
