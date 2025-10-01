/*
Copyright 2023 Morgan Venable @_claussen

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <complex.h>
#include <stdbool.h>
#include <stdint.h>
#include "svalboard.h"
#include "keymap_support.h"
#include "axis_scale.h"

// in keymap.c:
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
void pointing_device_init_user(void) {
    set_auto_mouse_layer(MH_AUTO_BUTTONS_LAYER); // only required if AUTO_MOUSE_DEFAULT_LAYER is not set to index of <mouse_layer>
    set_auto_mouse_enable(true);         // always required before the auto mouse feature will work
}
#endif

#if (defined MH_AUTO_BUTTONS && defined PS2_MOUSE_ENABLE && defined MOUSEKEY_ENABLE) || defined(POINTING_DEVICE_AUTO_MOUSE_MH_ENABLE)

static uint16_t mh_auto_buttons_timer;
extern int tp_buttons; // mousekey button state set in action.c and used in ps2_mouse.c

void mouse_mode(bool);

#endif

#if defined(POINTING_DEVICE_AUTO_MOUSE_MH_ENABLE)


#define SCROLL_FREQUENCY_MS 10
#define SCROLL_DIVISOR 1
#define SCROLL_MULTIPLIER 1
bool mouse_mode_enabled = false;

axis_scale_t l_x = {1, SCROLL_DIVISOR, SCROLL_MULTIPLIER};
axis_scale_t l_y = {1, SCROLL_DIVISOR, SCROLL_MULTIPLIER};
axis_scale_t r_x = {1, SCROLL_DIVISOR, SCROLL_MULTIPLIER};
axis_scale_t r_y = {1, SCROLL_DIVISOR, SCROLL_MULTIPLIER};


axis_scale_t sniper_x = {1, 1, 0};
axis_scale_t sniper_y = {1, 1, 0};
axis_scale_t sniper_h = {1, 1, 0};
axis_scale_t sniper_v = {1, 1, 0};

uint16_t scroll_timer = 0;
int16_t scroll_accumulator_h = 0;
int16_t scroll_accumulator_v = 0;

bool scroll_timer_running = false;

bool enable_scale_2 = false;
bool enable_scale_3 = false;
bool enable_scale_5 = false;

static bool scroll_hold    = false,
            scroll_toggle  = false;

typedef enum {
    AXIS_SCROLL_VERTICAL = 0,
    AXIS_SCROLL_HORIZONTAL = 1
} axis_scroll_mode_t;

static axis_scroll_mode_t current_axis_mode = AXIS_SCROLL_VERTICAL;
static uint32_t axis_scroll_last_activity_timer = 0;
static uint32_t axis_scroll_direction_timer = 0;
static uint32_t axis_scroll_h_accumulator = 0;
static uint32_t axis_scroll_v_accumulator = 0;
static bool axis_scroll_in_transition = false;

#define AXIS_SCROLL_CANCEL_RATIO 1.25 // To cancel a switch, the original direction's movement must be 2x stronger than the new direction's

static int16_t movement_to_hundredths_inch(int16_t movement, uint16_t dpi) {
    return ((int32_t)movement * 100) / dpi;
}
static void update_axis_scroll_mode(int16_t h_hundredths, int16_t v_hundredths) {
    if (!global_saved_values.axis_scroll_lock) {
        return;
    }

    uint32_t now = timer_read32();

    if (timer_elapsed32(axis_scroll_last_activity_timer) > global_saved_values.axis_scroll_inactivity_reset_ms) {
        if (current_axis_mode != AXIS_SCROLL_VERTICAL) {
            current_axis_mode = AXIS_SCROLL_VERTICAL;
            axis_scroll_h_accumulator = 0;
            axis_scroll_v_accumulator = 0;
            axis_scroll_in_transition = false;
        }
    }

    int16_t abs_h = abs(h_hundredths);
    int16_t abs_v = abs(v_hundredths);

    if (abs_h != 0 || abs_v != 0) {
        axis_scroll_last_activity_timer = now;

        if (current_axis_mode == AXIS_SCROLL_VERTICAL) {
            axis_scroll_h_accumulator += abs_h;
        } else {
            axis_scroll_v_accumulator += abs_v;
        }

        if (current_axis_mode == AXIS_SCROLL_VERTICAL) {
            if (!axis_scroll_in_transition) {
                if (axis_scroll_h_accumulator > global_saved_values.axis_scroll_notch_threshold ||
		    abs_v > (abs_h * AXIS_SCROLL_CANCEL_RATIO)) {
                    axis_scroll_in_transition = true;
                    axis_scroll_direction_timer = now;
                }
            } else {
                if (abs_v > (abs_h * AXIS_SCROLL_CANCEL_RATIO)) {
                    axis_scroll_in_transition = false;
                } else if (timer_elapsed32(axis_scroll_direction_timer) > global_saved_values.axis_scroll_slow_timer_ms) {
                    current_axis_mode = AXIS_SCROLL_HORIZONTAL;
                    axis_scroll_h_accumulator = 0;
                    axis_scroll_v_accumulator = 0;
		    scroll_accumulator_h = 0;
		    scroll_accumulator_v = 0;
                    axis_scroll_in_transition = false;
                }
            }
        } else {
            if (!axis_scroll_in_transition) {
                if (axis_scroll_v_accumulator > global_saved_values.axis_scroll_notch_threshold ||
		    abs_h > (abs_v * AXIS_SCROLL_CANCEL_RATIO)) {
                    axis_scroll_in_transition = true;
                    axis_scroll_direction_timer = now;
                }
            } else {
                if (abs_h > (abs_v * AXIS_SCROLL_CANCEL_RATIO)) {
                    axis_scroll_in_transition = false;
                } else if (timer_elapsed32(axis_scroll_direction_timer) > global_saved_values.axis_scroll_slow_timer_ms) {
                    current_axis_mode = AXIS_SCROLL_VERTICAL;
                    axis_scroll_h_accumulator = 0;
                    axis_scroll_v_accumulator = 0;
		    scroll_accumulator_h = 0;
		    scroll_accumulator_v = 0;
                    axis_scroll_in_transition = false;
                }
            }
        }
    }
}
void handle_sniper_key(bool pressed, uint8_t divisor) {
    if (!pressed) {
        div_div_axis(&sniper_x, divisor);
        div_div_axis(&sniper_y, divisor);
        div_div_axis(&sniper_h, divisor);
        div_div_axis(&sniper_v, divisor);
    } else {
        mult_div_axis(&sniper_x, divisor);
        mult_div_axis(&sniper_y, divisor);
        mult_div_axis(&sniper_h, divisor);
        mult_div_axis(&sniper_v, divisor);
    }
}
report_mouse_t pointing_device_task_combined_user(report_mouse_t reportMouse1, report_mouse_t reportMouse2) {
    report_mouse_t ret_mouse;

    if (enable_scale_2 || enable_scale_3 || enable_scale_5) {
        reportMouse1.x = add_to_axis(&sniper_x, reportMouse1.x);
        reportMouse1.y = add_to_axis(&sniper_y, reportMouse1.y);
        reportMouse1.h = add_to_axis(&sniper_h, reportMouse1.h);
        reportMouse1.v = add_to_axis(&sniper_v, reportMouse1.v);

        reportMouse2.x = add_to_axis(&sniper_x, reportMouse2.x);
        reportMouse2.y = add_to_axis(&sniper_y, reportMouse2.y);
        reportMouse2.h = add_to_axis(&sniper_h, reportMouse2.h);
        reportMouse2.v = add_to_axis(&sniper_v, reportMouse2.v);
    }

    if (reportMouse1.x == 0 && reportMouse1.y == 0 && reportMouse2.x == 0 && reportMouse2.y == 0)
        return pointing_device_combine_reports(reportMouse1, reportMouse2);

    // Temporary variables to hold the final scroll values in "counts"
    int16_t h1_counts = 0, v1_counts = 0;
    int16_t h2_counts = 0, v2_counts = 0;

    if ((global_saved_values.left_scroll != scroll_hold) != scroll_toggle) {
        h1_counts = reportMouse1.x;
        reportMouse1.h = add_to_axis(&l_x, reportMouse1.x);
        v1_counts = reportMouse1.y;
        reportMouse1.v = add_to_axis(&l_y, -reportMouse1.y);

        reportMouse1.x = 0;
        reportMouse1.y = 0;
    }
    if ((global_saved_values.right_scroll != scroll_hold) != scroll_toggle) {
        h2_counts = reportMouse2.x;
        reportMouse2.h = add_to_axis(&r_x, reportMouse2.x);
        v2_counts = reportMouse2.y;
        reportMouse2.v = add_to_axis(&r_y, -reportMouse2.y);

        reportMouse2.x = 0;
        reportMouse2.y = 0;
    }

    if ((reportMouse1.h != 0 || reportMouse1.v != 0 || reportMouse2.h != 0 || reportMouse2.v != 0) && !scroll_timer_running) {
        scroll_timer_running = true;
        scroll_timer = timer_read();
    }

    if (scroll_timer_running) {
        int16_t h1_hundredths = movement_to_hundredths_inch(h1_counts, get_left_dpi());
        int16_t v1_hundredths = movement_to_hundredths_inch(v1_counts, get_left_dpi());
        int16_t h2_hundredths = movement_to_hundredths_inch(h2_counts, get_right_dpi());
        int16_t v2_hundredths = movement_to_hundredths_inch(v2_counts, get_right_dpi());

        int16_t total_h_hundredths = h1_hundredths + h2_hundredths;
        int16_t total_v_hundredths = v1_hundredths + v2_hundredths;

        update_axis_scroll_mode(total_h_hundredths, total_v_hundredths);

        if (!axis_scroll_in_transition) {
            int16_t total_h_counts = reportMouse1.h + reportMouse2.h;
            int16_t total_v_counts = reportMouse1.v + reportMouse2.v;
            scroll_accumulator_h += total_h_counts;
            scroll_accumulator_v += total_v_counts;
        }
        reportMouse1.h = reportMouse2.h = 0;
        reportMouse1.v = reportMouse2.v = 0;
    }

    if (scroll_timer_running && timer_elapsed(scroll_timer) > SCROLL_FREQUENCY_MS) {
        if (global_saved_values.axis_scroll_lock) {
            if (current_axis_mode == AXIS_SCROLL_VERTICAL) {
                reportMouse1.v = scroll_accumulator_v;
                reportMouse1.h = 0;
            } else { // AXIS_SCROLL_HORIZONTAL
                reportMouse1.h = scroll_accumulator_h;
                reportMouse1.v = 0;
            }
        } else {
            reportMouse1.h = scroll_accumulator_h;
            reportMouse1.v = scroll_accumulator_v;
        }

        scroll_timer_running = false;
        scroll_accumulator_h = 0;
        scroll_accumulator_v = 0;
    }

    mouse_mode(true);
    ret_mouse = pointing_device_combine_reports(reportMouse1, reportMouse2);

    return pointing_device_task_user(ret_mouse);
}

// Enhanced axis scroll lock system settings
void adjust_axis_scroll_notch_threshold(bool increase) {
    if (increase && global_saved_values.axis_scroll_notch_threshold < 255) {
        global_saved_values.axis_scroll_notch_threshold += 10; // Adjust by 0.1 inch
    } else if (!increase && global_saved_values.axis_scroll_notch_threshold > 10) {
        global_saved_values.axis_scroll_notch_threshold -= 10;
    }
    uprintf("Axis scroll notch threshold: %d hundredths of inch\n", global_saved_values.axis_scroll_notch_threshold);
    write_eeprom_kb();
}

void adjust_axis_scroll_timer(bool increase) {
    if (increase && global_saved_values.axis_scroll_slow_timer_ms < 5000) {
        global_saved_values.axis_scroll_slow_timer_ms += 50;
    } else if (!increase && global_saved_values.axis_scroll_slow_timer_ms > 50) {
        global_saved_values.axis_scroll_slow_timer_ms -= 50;
    }
    uprintf("Axis scroll slow timer: %d ms\n", global_saved_values.axis_scroll_slow_timer_ms);
    write_eeprom_kb();
}

void adjust_axis_scroll_inactivity_reset(bool increase) {
    if (increase && global_saved_values.axis_scroll_inactivity_reset_ms < 5000) {
        global_saved_values.axis_scroll_inactivity_reset_ms += 100;
    } else if (!increase && global_saved_values.axis_scroll_inactivity_reset_ms > 100) {
        global_saved_values.axis_scroll_inactivity_reset_ms -= 100;
    }
    uprintf("Axis scroll inactivity timer: %d ms\n", global_saved_values.axis_scroll_inactivity_reset_ms);
    write_eeprom_kb();
}

void toggle_axis_scroll_lock(void) {
    global_saved_values.axis_scroll_lock = !global_saved_values.axis_scroll_lock;
    // Reset axis scroll state when toggling
    current_axis_mode = AXIS_SCROLL_VERTICAL;
    axis_scroll_h_accumulator = 0;
    axis_scroll_v_accumulator = 0;
    axis_scroll_in_transition = false;
    axis_scroll_last_activity_timer = timer_read32();
    write_eeprom_kb();
}

report_mouse_t pointing_device_task_user(report_mouse_t reportMouse) {
   if (reportMouse.x == 0 && reportMouse.y == 0 && reportMouse.h == 0 && reportMouse.v == 0)
        return reportMouse;

    mouse_mode(true);

    return reportMouse;
}
#endif

void mh_change_timeouts(void) {
    if (sizeof(mh_timer_choices) / sizeof(int16_t) - 1 <= global_saved_values.mh_timer_index) {
        global_saved_values.mh_timer_index = 0;
    } else {
        global_saved_values.mh_timer_index++;
    }
    uprintf("mh_timer:%d\n", mh_timer_choices[global_saved_values.mh_timer_index]);
    write_eeprom_kb();
}

#define LAYER_2345_MASK (0x3C)
void check_layer_67(void) {
    if ((layer_state & LAYER_2345_MASK) == LAYER_2345_MASK) {
        layer_on(6);
        layer_on(7);
    } else {
        layer_off(6);
        layer_off(7);
    }
}

bool in_mod_tap = false;
int8_t in_mod_tap_layer = -1;
int8_t mouse_keys_pressed = 0;
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {

    // Abort additional processing if userspace code did
    if (!process_record_user(keycode, record)) { return false;}

    // We are in a mod tap, with a KC_TRANSPARENT, lets make it transparent...
    if (IS_QK_MOD_TAP(keycode) && ((keycode & 0xFF) == KC_TRANSPARENT) &&
        record->tap.count > 0 && !in_mod_tap &&
        in_mod_tap_layer == -1 && record->event.pressed) {

        in_mod_tap_layer = get_highest_layer(layer_state);
        layer_state = layer_state & ~(1 << in_mod_tap_layer);

        in_mod_tap = true;

        return true;
    }

    // Fix things up on the release for the mod_tap case.
    if (!record->event.pressed && in_mod_tap) {
        in_mod_tap = false;
        layer_state = layer_state | (1 << in_mod_tap_layer);
        in_mod_tap_layer = -1;
        return true;
    }

    // If console is enabled, it will print the matrix position and status of each key pressed
#ifdef CONSOLE_ENABLE
    uprintf("KL: kc: 0x%04X, col: %2u, row: %2u, pressed: %u, time: %5u, int: %u, count: %u\n", keycode, record->event.key.col, record->event.key.row, record->event.pressed, record->event.time, record->tap.interrupted, record->tap.count);
#endif

    if (mouse_mode_enabled && layer_state & (1 << MH_AUTO_BUTTONS_LAYER)) {
        // The keycodes below are all that are forced to drop you out of mouse mode.
        // The intent is for this list to eventually become just KC_NO, and KC_TRNS
        // as more functionality is exported to keybard, and those keys are removed
        // from the firmmware. - ilc - 2024-10-05
#define BAD_KEYCODE_CONDITONAL  (keycode == KC_NO ||  \
	                    keycode == KC_TRNS || \
		            keycode == SV_LEFT_DPI_INC || \
	                    keycode == SV_LEFT_DPI_DEC || \
	                    keycode == SV_RIGHT_DPI_INC || \
	                    keycode == SV_RIGHT_DPI_DEC || \
	                    keycode == SV_LEFT_SCROLL_TOGGLE || \
		            keycode == SV_RIGHT_SCROLL_TOGGLE || \
		            keycode == SV_AXIS_SCROLL_LOCK || \
	                    keycode == SV_MH_CHANGE_TIMEOUTS || \
                        keycode == SV_TOGGLE_AUTOMOUSE)

        uint16_t layer_keycode = keymap_key_to_keycode(MH_AUTO_BUTTONS_LAYER, record->event.key);
        if (BAD_KEYCODE_CONDITONAL ||
	    layer_keycode != keycode) {
#ifdef CONSOLE_ENABLE
            uprintf("process_record - mh_auto_buttons: off\n");
#endif
            if (BAD_KEYCODE_CONDITONAL) {
                mouse_mode(false);
	        return false;
            } else {
                mouse_mode(false);
            }
        } else {
            if (record->event.pressed) {
                mouse_keys_pressed++;
                mouse_mode(true);
            } else {
                // keys that are held before the mouse layer is active can mess this up.
                if (mouse_keys_pressed > 0) {
                    mouse_keys_pressed--;
                }
                mouse_mode(true);
            }
        }
    }

    if (record->event.pressed) { // key pressed
        switch (keycode) {
            case SV_LEFT_DPI_INC:
                increase_left_dpi();
                return false;
            case SV_LEFT_DPI_DEC:
                decrease_left_dpi();
                return false;
            case SV_RIGHT_DPI_INC:
                increase_right_dpi();
                return false;
            case SV_RIGHT_DPI_DEC:
                decrease_right_dpi();
                return false;
            case SV_LEFT_SCROLL_TOGGLE:
                global_saved_values.left_scroll = !global_saved_values.left_scroll;
                write_eeprom_kb();
                return false;
            case SV_RIGHT_SCROLL_TOGGLE:
                global_saved_values.right_scroll = !global_saved_values.right_scroll;
                write_eeprom_kb();
                return false;
            case SV_RECALIBRATE_POINTER:
                recalibrate_pointer();
                return false;
            case SV_MH_CHANGE_TIMEOUTS:
                mh_change_timeouts();
                return false;
            case SV_CAPS_WORD:
                caps_word_toggle();
                return false;
            case SV_AXIS_SCROLL_LOCK:
                toggle_axis_scroll_lock();
                return false;
            case SV_AXIS_SCROLL_NOTCH_INC:
                adjust_axis_scroll_notch_threshold(true);
                return false;
            case SV_AXIS_SCROLL_NOTCH_DEC:
                adjust_axis_scroll_notch_threshold(false);
                return false;
            case SV_AXIS_SCROLL_TIMER_INC:
                adjust_axis_scroll_timer(true);
                return false;
            case SV_AXIS_SCROLL_TIMER_DEC:
                adjust_axis_scroll_timer(false);
                return false;
            case SV_AXIS_SCROLL_INACTIVE_INC:
                adjust_axis_scroll_inactivity_reset(true);
                return false;
            case SV_AXIS_SCROLL_INACTIVE_DEC:
                adjust_axis_scroll_inactivity_reset(false);
                return false;
            case SV_TOGGLE_23_67:
                layer_on(2);
                layer_on(3);
                check_layer_67();
                return false;
            case SV_TOGGLE_45_67:
                layer_on(4);
                layer_on(5);
                check_layer_67();
                return false;
            case SV_SNIPER_2:
	            enable_scale_2 = true;
				handle_sniper_key(true, 2);
                return false;
            case SV_SNIPER_3:
                enable_scale_3 = true;
                handle_sniper_key(true, 3);
                return false;
            case SV_SNIPER_5:
                enable_scale_5 = true;
                handle_sniper_key(true, 5);
                return false;
            case SV_SCROLL_HOLD:
                scroll_hold = true;
                return false;
            case SV_SCROLL_TOGGLE:
                return false;
            case SV_OUTPUT_STATUS:
                output_keyboard_info();
                return false;
            case SV_TOGGLE_AUTOMOUSE:
                //if we disable automouse, manually kick out of mouse mode in case timer was running
                //needs to go first to avoid the lockout
                if (global_saved_values.auto_mouse) {
                    mouse_mode(false);
                }
                global_saved_values.auto_mouse = !global_saved_values.auto_mouse;
                write_eeprom_kb();
                return false;
        }
    } else { // key released
        switch (keycode) {
            // These keys are all holds and require un-setting upon release.
            case SV_TOGGLE_23_67:
                layer_off(2);
                layer_off(3);
                check_layer_67();
                return false;
            case SV_TOGGLE_45_67:
                layer_off(4);
                layer_off(5);
                check_layer_67();
                return false;
            case SV_SNIPER_2:
                enable_scale_2 = false;
                handle_sniper_key(false, 2);
                return false;
            case SV_SNIPER_3:
                enable_scale_3 = false;
                handle_sniper_key(false, 3);
                return false;
            case SV_SNIPER_5:
                enable_scale_5 = false;
                handle_sniper_key(false, 5);
                return false;
            case SV_SCROLL_HOLD:
                scroll_hold = false;
                return false;
            case SV_SCROLL_TOGGLE:
                scroll_toggle ^= true;
                return false;
        }
    }

    // Neither the user nor the keyboard handled the event, so continue with normal handling
    return true;
};

#if defined MH_AUTO_BUTTONS && defined PS2_MOUSE_ENABLE && defined MOUSEKEY_ENABLE
void ps2_mouse_moved_user(report_mouse_t *mouse_report) {
    mouse_mode(true);
}
#endif

void matrix_scan_kb(void) {
    if ((mh_timer_choices[global_saved_values.mh_timer_index] >= 0) && mouse_mode_enabled && (timer_elapsed(mh_auto_buttons_timer) > mh_timer_choices[global_saved_values.mh_timer_index]) && mouse_keys_pressed == 0) {
        if (!tp_buttons) {
            mouse_mode(false);
#if defined CONSOLE_ENABLE
            print("matrix - mh_auto_buttons: off\n");
#endif
        }
    }

    matrix_scan_user();
}

void mouse_mode(bool on) {
    if (global_saved_values.auto_mouse) {
        if (on) {
            layer_on(MH_AUTO_BUTTONS_LAYER);
            mh_auto_buttons_timer = timer_read();
            mouse_mode_enabled = true;
        } else {
            layer_off(MH_AUTO_BUTTONS_LAYER);
            mh_auto_buttons_timer = 0;
            mouse_mode_enabled = false;
            mouse_keys_pressed = 0;
        }
    }
}
