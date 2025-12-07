#pragma once

#define I2C_DRIVER I2CD0
#define I2C1_SDA_PIN GP16
#define I2C1_SCL_PIN GP17

#define AZOTEQ_IQS5XX_TPS43
#define AZOTEQ_IQS5XX_TIMEOUT_MS 10
//#define AZOTEQ_IQS5XX_TAP_DISTANCE 10
#define AZOTEQ_IQS5XX_PRESS_AND_HOLD_ENABLE true
//This decreases the required 'wait' time to activate dragging/selecting on a touchpad from default 300ms to 200ms.
#define AZOTEQ_IQS5XX_HOLD_TIME 200

//This lowers the maximum time for recognizing a tap from default 150ms to 100ms.
#define AZOTEQ_IQS5XX_TAP_TIME 100
//Maximum Tap distance before a tap doesn't count, in pixels. Default is 25 pixels.
#define AZOTEQ_IQS5XX_TAP_DISTANCE 25
//Increases the distance needed to trigger 'scrolling,' must interact with the tap distance. Default is 50 pixels, increasing buffer between tap and scroll distance should help.
#define AZOTEQ_IQS5XX_SCROLL_INITIAL_DISTANCE 75
//#define POINTING_DEVICE_MOTION_PIN GP18

//Some interesting thoughts in ploopyco's trackpad repo...


#define SPLIT_POINTING_ENABLE
#define POINTING_DEVICE_COMBINED
#define POINTING_DEVICE_AUTO_MOUSE_MH_ENABLE
