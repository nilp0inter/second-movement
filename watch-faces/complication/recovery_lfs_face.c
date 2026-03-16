/*
 * MIT License
 *
 * Copyright (c) 2026
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "watch.h"
#include "filesystem.h"

#include "recovery_lfs_face.h"
#include "fesk_session.h"

#define MAX_RECOVERY_RECORDS 30
#define RECOVERY_FILE "recovery.txt"
#define MAX_ENCODED_SIZE 128

struct recovery_record {
    char label[4];
    uint16_t file_offset;
    uint16_t code_length;
};

static struct recovery_record records[MAX_RECOVERY_RECORDS];
static uint8_t num_records = 0;

static void recovery_lfs_face_read_file(char *filename) {
    if (!filesystem_file_exists(filename)) {
        printf("Recovery file error: %s\n", filename);
        return;
    }

    char line[256];
    int32_t offset = 0, old_offset = 0;
    while (old_offset = offset, filesystem_read_line(filename, line, &offset, 255) && strlen(line)) {
        if (num_records == MAX_RECOVERY_RECORDS) {
            printf("Recovery max records: %d\n", MAX_RECOVERY_RECORDS);
            break;
        }

        char *colon = strchr(line, ':');
        if (colon == NULL) {
            continue;
        }

        size_t label_len = colon - line;
        if (label_len == 0 || label_len > 3) {
            continue;
        }

        char *code = colon + 1;
        size_t code_len = strlen(code);
        if (code_len == 0) {
            continue;
        }

        struct recovery_record *rec = &records[num_records];
        memset(rec->label, ' ', 3);
        rec->label[3] = '\0';
        memcpy(rec->label, line, label_len);
        rec->file_offset = old_offset + (code - line);
        rec->code_length = code_len;
        num_records++;
    }
}

static void recovery_face_display(recovery_lfs_state_t *state) {
    char buf[7];

    if (num_records == 0) {
        watch_display_text(WATCH_POSITION_FULL, "no rECCd");
        return;
    }

    uint8_t index = state->current_index;
    struct recovery_record *rec = &records[index];
    bool used = (state->used_mask >> index) & 1;

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, rec->label, rec->label);
    sprintf(buf, "%2d", index + 1);
    watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, buf, buf);

    if (used) {
        watch_display_text(WATCH_POSITION_BOTTOM, " USEd ");
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    } else {
        watch_display_text(WATCH_POSITION_BOTTOM, "      ");
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    }
}

static const char FESK_CHARSET[] = "abcdefghijklmnopqrstuvwxyz0123456789 ,:'\"\n";

static bool is_fesk_char(char c) {
    return strchr(FESK_CHARSET, c) != NULL;
}

static char encoded_buffer[MAX_ENCODED_SIZE];

static bool recovery_encode_code(const char *code, size_t code_len) {
    size_t out = 0;
    for (size_t i = 0; i < code_len; i++) {
        char c = code[i];
        if (c >= 'A' && c <= 'Z') {
            if (out + 2 > MAX_ENCODED_SIZE - 1) return false;
            encoded_buffer[out++] = '\'';
            encoded_buffer[out++] = c - 'A' + 'a';
        } else {
            if (out + 1 > MAX_ENCODED_SIZE - 1) return false;
            if (!is_fesk_char(c)) return false;
            encoded_buffer[out++] = c;
        }
    }
    encoded_buffer[out] = '\0';
    return true;
}

static void _recovery_fesk_bottom_display(const char *text) {
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text(WATCH_POSITION_BOTTOM, text);
}

static void _recovery_fesk_on_countdown_tick(uint8_t seconds_remaining, void *user_data) {
    (void)user_data;
    char buf[7];
    if (seconds_remaining > 0) {
        snprintf(buf, sizeof(buf), "    %2u", (unsigned int)seconds_remaining);
    } else {
        snprintf(buf, sizeof(buf), "  GO  ");
    }
    _recovery_fesk_bottom_display(buf);
}

static void _recovery_fesk_on_countdown_complete(void *user_data) {
    (void)user_data;
    _recovery_fesk_bottom_display("  GO  ");
}

static void _recovery_fesk_on_tx_start(void *user_data) {
    (void)user_data;
    _recovery_fesk_bottom_display(" SEnd ");
}

static void _recovery_fesk_on_tx_end(void *user_data) {
    recovery_lfs_state_t *state = (recovery_lfs_state_t *)user_data;
    state->used_mask |= (1u << state->current_index);
    _recovery_fesk_bottom_display(" dONE ");
}

static void _recovery_start_fesk(recovery_lfs_state_t *state) {
    if (num_records == 0) return;

    struct recovery_record *rec = &records[state->current_index];
    char code_buf[256];
    int32_t file_offset = rec->file_offset;

    if (!filesystem_read_line(RECOVERY_FILE, code_buf, &file_offset, rec->code_length + 1)) {
        _recovery_fesk_bottom_display("Error ");
        return;
    }
    code_buf[rec->code_length] = '\0';

    if (!recovery_encode_code(code_buf, rec->code_length)) {
        _recovery_fesk_bottom_display("Error ");
        return;
    }

    fesk_session_config_t config = fesk_session_config_defaults();
    config.static_message = encoded_buffer;
    config.enable_countdown = true;
    config.mode = FESK_MODE_4FSK;
    config.on_countdown_tick = _recovery_fesk_on_countdown_tick;
    config.on_countdown_complete = _recovery_fesk_on_countdown_complete;
    config.on_transmission_start = _recovery_fesk_on_tx_start;
    config.on_transmission_end = _recovery_fesk_on_tx_end;
    config.user_data = state;
    fesk_session_init(&state->fesk_session, &config);
    fesk_session_start(&state->fesk_session);
}

void recovery_lfs_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(recovery_lfs_state_t));
    }

#if !(__EMSCRIPTEN__)
    if (num_records == 0) {
        recovery_lfs_face_read_file(RECOVERY_FILE);
    }
#endif
}

void recovery_lfs_face_activate(void *context) {
    recovery_lfs_state_t *state = (recovery_lfs_state_t *)context;
    uint32_t saved_mask = state->used_mask;
    memset(context, 0, sizeof(recovery_lfs_state_t));
    state->used_mask = saved_mask;

#if __EMSCRIPTEN__
    if (num_records == 0) {
        recovery_lfs_face_read_file(RECOVERY_FILE);
    }
#endif
}

bool recovery_lfs_face_loop(movement_event_t event, void *context) {
    recovery_lfs_state_t *state = (recovery_lfs_state_t *)context;

    switch (event.event_type) {
        case EVENT_TICK:
            if (fesk_session_is_idle(&state->fesk_session)) {
                recovery_face_display(state);
            }
            break;
        case EVENT_ACTIVATE:
            recovery_face_display(state);
            break;
        case EVENT_TIMEOUT:
            if (fesk_session_is_idle(&state->fesk_session)) {
                movement_move_to_face(0);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (num_records > 0) {
                state->current_index = (state->current_index + 1) % num_records;
                recovery_face_display(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (num_records > 0) {
                state->current_index = (state->current_index + num_records - 1) % num_records;
                recovery_face_display(state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (fesk_session_is_idle(&state->fesk_session)) {
                _recovery_start_fesk(state);
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            if (num_records > 0) {
                state->used_mask ^= (1u << state->current_index);
                recovery_face_display(state);
            }
            break;
        case EVENT_ALARM_BUTTON_DOWN:
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void recovery_lfs_face_resign(void *context) {
    (void) context;
}
