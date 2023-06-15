//
// Created by Hugo Trippaers on 21/05/2021.
//
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#include "esp_log.h"

#include "esp-rtsp-common.h"

#define PARSE_STATE_INIT() { \
     .parse_complete = false, \
     .state = RTSP_PARSER_PARSE_METHOD, \
     .intermediate_len = 0   \
     }

#define RTSP_PARSER_PARSE_METHOD 0
#define RTSP_PARSER_PARSE_URL 1
#define RTSP_PARSER_PARSE_PROTOCOL 2
#define RTSP_PARSER_OPTIONAL_HEADER 9
#define RTSP_PARSER_PARSE_HEADER 10
#define RTSP_PARSER_PARSE_HEADER_VALUE 11
#define RTSP_PARSER_PARSE_HEADER_WS 12

#define TAG "rtsp-parser"

typedef struct {
    int state;
    int parse_complete;
    int error;
    char intermediate[1024];
    size_t intermediate_len;
    rtsp_req_t *request;
} rtsp_parser_state_t;

inline int min(int a, int b) { return (a < b) ? a : b; }

static bool valid_header_name_char(char c) {
    if (c > 127) return false;
    if (c <=31 || c == 127 ) return false; // CTLs
    if (c == '(' || c == ')' || c == '<' || c == '>' || c == '@' ||
        c == ',' || c == ';' || c == ':' || c == '\\' || c == '"' ||
        c == '/' || c == '[' || c == ']' || c == '?' || c == '=' ||
        c == '{' || c == '}' || c == ' ' || c == '\t' )
        return false; // separators

    return true;
}

static int safe_atoi(char *value) {
    char *end;
    long lv = strtol(value, &end, 10);
    if (*end != '\0' || lv < INT_MIN || lv > INT_MAX) {
        ESP_LOGE(TAG, "Invalid numerical value: %s", value);
        return -1;
    }
    return (int)lv;
}

int rtsp_parser_init(rtsp_parser_handle_t *handle) {
    rtsp_parser_state_t *state = calloc(1, sizeof(rtsp_parser_state_t));
    if (!state) {
        return PARSER_NOMEM;
    }

    memset(state, 0 , sizeof(rtsp_parser_state_t));

    rtsp_req_t *request = calloc(1, sizeof(rtsp_req_t));
    if (!request) {
        free(state);
        return PARSER_NOMEM;
    }

    memset(request, 0 , sizeof(rtsp_req_t));

    state->request = request;
    *handle = state;

    return PARSER_OK;
}

int parse_request(rtsp_parser_handle_t handle, const char *buffer, const size_t len) {
    if (!handle) {
        ESP_LOGD(TAG, "Error; Invalid handle");
        return PARSER_INVALID_ARGS;
    }
    rtsp_parser_state_t *state = (rtsp_parser_state_t *)handle;
    rtsp_req_t *request = state->request;

    if (state->parse_complete) {
        ESP_LOGD(TAG, "Error; Can't add data to completed request");
        return PARSER_INVALID_STATE;
    }

    

    int position = 0;
    int header_marker = 0;
    int header_value_marker = 0;
    for (int i = 0; i < len; i++) {
        char current = buffer[i];
        state->intermediate[state->intermediate_len + 1] = 0x0; // workaround
        ESP_LOGD(TAG, "Parsing '%c' at %d in state %d", current, position, state->state);
        ESP_LOGD(TAG, "Intermediate: %s (%d bytes)", state->intermediate, state->intermediate_len);

        if (state->intermediate_len >= 1023) {
            ESP_LOGE(TAG, "Parse failed, no space in intermediate buffer");
            return PARSER_NOMEM;
        }

        // Handle CR/LF
        if (current == '\r' && i < len - 1) {
            current = buffer[i++];
            position++;
        }

        switch(state->state) {
            case RTSP_PARSER_PARSE_METHOD:
                if (current == ' ') {
                    if (strncmp(state->intermediate, "OPTIONS", min(state->intermediate_len,7)) == 0) {
                        request->request_type = OPTIONS;
                    } else if (strncmp(state->intermediate, "SETUP", min(state->intermediate_len,5)) == 0) {
                        request->request_type = SETUP;
                    } else if (strncmp(state->intermediate, "DESCRIBE", min(state->intermediate_len,8)) == 0) {
                        request->request_type = DESCRIBE;
                    } else if (strncmp(state->intermediate, "PLAY", min(state->intermediate_len,4)) == 0) {
                        request->request_type = PLAY;
                    } else if (strncmp(state->intermediate, "TEARDOWN", min(state->intermediate_len,8)) == 0) {
                        request->request_type = TEARDOWN;
                    } else {
                        request->request_type = UNSUPPORTED;
                    }

                    state->state = RTSP_PARSER_PARSE_URL;
                    state->intermediate_len = 0;
                    position++;

                    ESP_LOGD(TAG, "Method: %d", request->request_type);
                    continue;
                } else if ((current < 'A' || current > 'Z') && current != '_') {
                    ESP_LOGE(TAG, "Invalid character in method: %c", current);
                    state->error = 400;
                    return i;
                } else {
                    state->intermediate[state->intermediate_len] = current;
                    position++;
                    state->intermediate_len++;
                }
                break;
            case RTSP_PARSER_PARSE_URL:
                if (current == ' ') {
                    strncpy(request->url, state->intermediate, min(state->intermediate_len, URL_MAX_LENGTH));
                    request->url[min(state->intermediate_len, URL_MAX_LENGTH)] = 0x0;
                    ESP_LOGD(TAG, "Parsed url: %s", request->url);

                    state->state = RTSP_PARSER_PARSE_PROTOCOL;
                    state->intermediate_len = 0;
                    position++;

                    continue;
                } else if (current == '\r' || current == '\n') {
                    ESP_LOGE(TAG, "Invalid character in url: %c", current);
                    state->error = 400;
                    return i;
                } else {
                    state->intermediate[state->intermediate_len] = current;
                    position++;
                    state->intermediate_len++;
                }
                break;
            case RTSP_PARSER_PARSE_PROTOCOL:
                if (current == '\r' || current == '\n') {
                    if (strncmp(state->intermediate, "RTSP/1.0", min(state->intermediate_len,9)) != 0) {
                        ESP_LOGW(TAG, "Only supporting RTSP/1.0 but got: %s", state->intermediate);
                        state->error = 400;
                    }

                    state->state = RTSP_PARSER_OPTIONAL_HEADER;
                    state->intermediate_len = 0;
                    position++;
                } else {
                    state->intermediate[state->intermediate_len] = current;
                    position++;
                    state->intermediate_len++;
                }
                break;
            case RTSP_PARSER_OPTIONAL_HEADER:
                if (current == '\r' || current == '\n') {
                    ESP_LOGD(TAG, "Setting parse_complete");
                    state->parse_complete = true;
                    return len;
                }
            case RTSP_PARSER_PARSE_HEADER:
                if (current == ':') {
                    state->intermediate[state->intermediate_len] = 0x0;
                    position++;
                    state->intermediate_len++;
                    header_marker = 0;
                    header_value_marker = state->intermediate_len;
                    state->state = RTSP_PARSER_PARSE_HEADER_WS;
                } else if (!valid_header_name_char(current)) {
                    ESP_LOGW(TAG, "Invalid characters in header name: %c", current);
                    state->error = 400;
                    return i;
                } else {
                    state->intermediate[state->intermediate_len] = current;
                    position++;
                    state->intermediate_len++;
                }
                break;
            case RTSP_PARSER_PARSE_HEADER_WS:
                if (current == ' ') {
                    state->state = RTSP_PARSER_PARSE_HEADER_VALUE;
                    continue;
                }
                ESP_LOGW(TAG, "Unexpected character in header: %c", current);
                state->error = 400;
                return i;
            case RTSP_PARSER_PARSE_HEADER_VALUE:
                if (current == '\r' || current == '\n') {
                    char *header = state->intermediate + header_marker;
                    char *value = state->intermediate + header_value_marker;

                    ESP_LOGD(TAG, "Header> %s: %s", header, value);
                    if (strcasecmp(header, "cseq") == 0) {
                        char *end;
                        long lv = strtol(value, &end, 10);
                        if (*end != '\0' || lv < INT_MIN || lv > INT_MAX) {
                            ESP_LOGW(TAG, "Invalid numerical value for header %s: %s", header, value);
                            state->error = 400;
                            return i;
                        }
                        request->cseq = (int)lv;
                    } else if (strcasecmp(header, "transport") == 0) {
                        char *saveptr;

                        char *token = strtok_r(value, ";", &saveptr);
                        if (token == NULL || strcmp(token, "RTP/AVP") != 0) {
                            ESP_LOGW(TAG, "Unsupported stream transport: %s", token);
                            state->error = 461;
                            return i;
                        }

                        token = strtok_r(NULL, ";", &saveptr);
                        if (token == NULL || strcmp(token, "unicast") != 0) {
                            ESP_LOGW(TAG, "Unsupported direction transport: %s", token);
                            state->error = 400;
                            return i;
                        }

                        token = strtok_r(NULL, ";", &saveptr);
                        if (token == NULL || strncmp(token, "client_port=", min(strlen(token), 7)) != 0) {
                            ESP_LOGE(TAG, "Expected client_port, got : %s", token);
                            state->error = 400;
                            return i;
                        }

                        char *porta = strtok_r(token+12, "-", &saveptr);
                        request->dst_rtp_port = safe_atoi(porta);

                        char *portb = strtok_r(NULL, "-", &saveptr);
                        request->dst_rtcp_port = safe_atoi(portb);

                        if (request->dst_rtp_port < 0 || request->dst_rtcp_port < 0) {
                            ESP_LOGW(TAG, "Invalid client_port values: %s", token);
                            state->error = 400;
                            return i;
                        }

                    }
                    state->state = RTSP_PARSER_OPTIONAL_HEADER;
                    state->intermediate_len = 0;
                    position++;
                } else {
                    state->intermediate[state->intermediate_len] = current;
                    position++;
                    state->intermediate_len++;
                }
                break;

            default:
                continue;
        };
    }
    return len;
};

int parser_free(rtsp_parser_handle_t handle) {
    rtsp_parser_state_t *state = (rtsp_parser_state_t *)handle;

    state->request = NULL; // Freeing request is left to the caller
    free(state);

    return 0;
}

int parser_get_error(rtsp_parser_handle_t handle) {
    rtsp_parser_state_t *state = (rtsp_parser_state_t *)handle;
    return state->error;
}

int parser_is_complete(rtsp_parser_handle_t handle) {
    rtsp_parser_state_t *state = (rtsp_parser_state_t *)handle;
    return state->parse_complete;
}

rtsp_req_t *parser_get_request(rtsp_parser_handle_t handle) {
    rtsp_parser_state_t *state = (rtsp_parser_state_t *)handle;

    return state->request;
}
