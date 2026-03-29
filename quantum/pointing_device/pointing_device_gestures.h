/* Copyright 2022 Daniel Kao <daniel.m.kao@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <stdint.h>
#include "report.h"

#ifdef POINTING_DEVICE_GESTURES_CURSOR_GLIDE_ENABLE
typedef struct {
    mouse_xy_report_t dx;
    mouse_xy_report_t dy;
    bool              valid;
} cursor_glide_t;

typedef struct {
    uint16_t trigger_px; /* Pixels of movement needed to trigger cursor glide */
    uint16_t coef;       /* Coefficient of friction */
    uint16_t interval;   /* Glide report interval, in milliseconds */
} cursor_glide_config_t;

typedef struct {
    int32_t           v0;
    int32_t           x;
    int32_t           y;
    uint16_t          z;
    uint16_t          timer;
    uint16_t          counter;
    mouse_xy_report_t dx0;
    mouse_xy_report_t dy0;
} cursor_glide_status_t;

typedef struct {
    cursor_glide_config_t config;
    cursor_glide_status_t status;
} cursor_glide_context_t;

/* Check glide report conditions, calculates glide coordinates */
cursor_glide_t cursor_glide_check(cursor_glide_context_t* glide);

/* Start glide reporting, gives first set of glide coordinates */
cursor_glide_t cursor_glide_start(cursor_glide_context_t* glide);

/* Update glide engine on the latest cursor movement, cursor glide is based on the final movement */
void cursor_glide_update(cursor_glide_context_t* glide, mouse_xy_report_t dx, mouse_xy_report_t dy, uint16_t z);
#endif

/* ── Scroll divisor ─────────────────────────────────────────────── */
#ifndef POINTING_DEVICE_GESTURES_SCROLL_DIVISOR
#    define POINTING_DEVICE_GESTURES_SCROLL_DIVISOR 1
#endif

/* ── Natural (inverted) scrolling ────────────────────────────────── */
#ifndef POINTING_DEVICE_GESTURES_NATURAL_SCROLL_ENABLE
#    define POINTING_DEVICE_GESTURES_NATURAL_SCROLL_ENABLE false
#endif

/* ── Tap-to-drag (runtime configurable) ──────────────────────────── */
#ifndef POINTING_DEVICE_GESTURES_TAP_DRAG_ENABLE
#    define POINTING_DEVICE_GESTURES_TAP_DRAG_ENABLE false
#endif
#ifndef POINTING_DEVICE_GESTURES_TAP_DRAG_WINDOW_MS
#    define POINTING_DEVICE_GESTURES_TAP_DRAG_WINDOW_MS 200
#endif

#if POINTING_DEVICE_GESTURES_TAP_DRAG_ENABLE
/**
 * Set tap-drag window at runtime.
 *   0  = tap-drag disabled
 *  >0  = window in milliseconds
 *  -1  = drag lock (hold until second tap)
 */
void pointing_device_gestures_set_tap_drag(int16_t window_ms);
int16_t pointing_device_gestures_get_tap_drag(void);

/**
 * Drivers call this when hardware detects a tap gesture,
 * passing the number of fingers involved.
 */
void pointing_device_gesture_notify_tap(uint8_t finger_count);

/**
 * Drivers call this each cycle to report current finger count.
 */
void pointing_device_gesture_notify_fingers(uint8_t count);
#endif

/* ── Three-finger tap button mapping ─────────────────────────────── */
/* Define to a POINTING_DEVICE_BUTTONx to enable, e.g. POINTING_DEVICE_BUTTON3 for middle click */
/* #define POINTING_DEVICE_GESTURES_THREE_FINGER_TAP_BUTTON POINTING_DEVICE_BUTTON3 */

/* ── Scroll momentum (kinetic scrolling) ─────────────────────────── */
#ifndef POINTING_DEVICE_GESTURES_SCROLL_MOMENTUM_DECAY
#    define POINTING_DEVICE_GESTURES_SCROLL_MOMENTUM_DECAY 19 /* out of 20, ~0.95 */
#endif
#ifndef POINTING_DEVICE_GESTURES_SCROLL_MOMENTUM_INTERVAL_MS
#    define POINTING_DEVICE_GESTURES_SCROLL_MOMENTUM_INTERVAL_MS 16 /* ~60fps */
#endif

/* ── Enable flag ─────────────────────────────────────────────────── */
#if POINTING_DEVICE_GESTURES_SCROLL_DIVISOR > 1 || POINTING_DEVICE_GESTURES_NATURAL_SCROLL_ENABLE || POINTING_DEVICE_GESTURES_TAP_DRAG_ENABLE || defined(POINTING_DEVICE_GESTURES_SCROLL_MOMENTUM_ENABLE) || defined(POINTING_DEVICE_GESTURES_THREE_FINGER_TAP_BUTTON)
#    define POINTING_DEVICE_GESTURES_ENABLE
#endif

/**
 * Process gestures on the mouse report.
 * Called from the pointing device pipeline.
 */
report_mouse_t pointing_device_gestures_process(report_mouse_t mouse_report);
