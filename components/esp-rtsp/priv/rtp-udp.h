//
// Created by Hugo Trippaers on 19/05/2021.
//

#ifndef ESPCAM_RTP_UDP_H
#define ESPCAM_RTP_UDP_H

typedef struct {
    int initialized;

    uint16_t rtp_socket;
    uint16_t rtcp_socket;

    uint16_t src_rtp_port;
    uint16_t src_rtcp_port;

    char dst_addr[128];
    uint16_t dst_rtp_port;
    uint16_t dst_rtcp_port;

    uint32_t timestamp;
    uint32_t sequence_number;
} esp_rtp_session_t;

typedef void* esp_rtp_session_handle_t;

typedef struct {
    uint8_t payload_type;
    uint8_t marker;
    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;
} esp_rtp_header_t;

typedef struct {
    uint8_t type_specific;
    uint16_t fragment_offset;
    uint8_t type;
    uint8_t q;
    uint16_t width;
    uint16_t height;
} esp_rtp_jpeg_header_t;

typedef struct {
    char *jpeg_data_start;
    size_t jpeg_data_length;
    char *quant_table_0;
    char *quant_table_1;
} esp_rtsp_jpeg_data_t;

esp_err_t esp_rtsp_jpeg_decode(char *buffer, size_t length, esp_rtsp_jpeg_data_t *rtsp_jpeg_data);

typedef struct {
    uint8_t mbz;
    uint8_t precision;
    uint16_t length;
    char table0[64];
    char table1[64];
} esp_rtp_quant_t;

#define RTP_QUANT_DEFAULT() { \
    .mbz = 0,                 \
    .precision = 0,           \
    .length = 128             \
}

esp_err_t esp_rtp_init(esp_rtp_session_handle_t *rtp_session, int dst_rtp_port, int dst_rtcp_port, char *dst_addr_string);
esp_err_t esp_rtp_teardown(esp_rtp_session_handle_t rtp_session);
esp_err_t esp_rtp_send_jpeg(esp_rtp_session_handle_t rtp_session, uint8_t *frame, size_t frame_length);
int esp_rtp_get_src_rtp_port(esp_rtp_session_handle_t rtp_session);
int esp_rtp_get_src_rtcp_port(esp_rtp_session_handle_t rtp_session);

#endif //ESPCAM_RTP_UDP_H
