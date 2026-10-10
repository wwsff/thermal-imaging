#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>

typedef enum
{
    APP_PAGE_THERMAL = 0,
    APP_PAGE_BATTERY = 1
} app_page_t;

extern volatile app_page_t g_app_page;
extern volatile uint8_t g_mlx_available;

extern volatile uint8_t g_battery_connected;
extern volatile uint8_t g_battery_version;
extern volatile uint32_t g_battery_soc_x10;
extern volatile uint32_t g_battery_voltage_mv;
extern volatile uint32_t g_battery_last_update_tick;
extern volatile int32_t g_battery_error;

#endif
