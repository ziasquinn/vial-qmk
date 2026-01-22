#pragma once

#define I2C_DRIVER I2CD0
#define I2C1_SDA_PIN GP16
#define I2C1_SCL_PIN GP17

#define AZOTEQ_IQS5XX_TPS43
#define AZOTEQ_IQS5XX_TIMEOUT_MS 10
#define AZOTEQ_IQS5XX_PRESS_AND_HOLD_ENABLE true

#define AZOTEQ_IQS5XX_TAP_TIME 75
// TAP_TIME default is 150ms
#define AZOTEQ_IQS5XX_HOLD_TIME 150 
// HOLD_TIME default is 300ms
// Here, we decrease the TAP_TIME to account for the reduced in HOLD_TIME so they aren't confused with each other.
// HOLD_TIME being reduced from 300ms to 150ms decreases the required 'wait' time to activate dragging/selecting on a touchpad, less sensitive than 20ms, but not tedious like 300ms. 

//#define POINTING_DEVICE_MOTION_PIN GP18

#define SPLIT_POINTING_ENABLE
#define POINTING_DEVICE_COMBINED
#define POINTING_DEVICE_AUTO_MOUSE_MH_ENABLE
