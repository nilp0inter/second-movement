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

#ifndef RECOVERY_LFS_FACE_H_
#define RECOVERY_LFS_FACE_H_

/*
 * Recovery Codes LFS face
 *
 * Stores recovery/backup codes in a plain text file on LFS and transmits
 * them via FESK (audio FSK). Codes are never shown on the display.
 *
 * File format (recovery.txt): one code per line as LABEL:code
 *   GO:abcd1234efgh
 *   PP:123456
 *
 * ALARM short press: next code
 * LIGHT short press: previous code
 * ALARM long press: send code via FESK, auto-mark as used
 * LIGHT long press: toggle used/unused status
 */

#include "movement.h"
#include "fesk_session.h"

typedef struct {
    uint8_t current_index;
    uint32_t used_mask;
    fesk_session_t fesk_session;
} recovery_lfs_state_t;

void recovery_lfs_face_setup(uint8_t watch_face_index, void ** context_ptr);
void recovery_lfs_face_activate(void *context);
bool recovery_lfs_face_loop(movement_event_t event, void *context);
void recovery_lfs_face_resign(void *context);

#define recovery_lfs_face ((const watch_face_t){ \
    recovery_lfs_face_setup, \
    recovery_lfs_face_activate, \
    recovery_lfs_face_loop, \
    recovery_lfs_face_resign, \
    NULL, \
})

#endif // RECOVERY_LFS_FACE_H_
