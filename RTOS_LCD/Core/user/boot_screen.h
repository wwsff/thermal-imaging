#ifndef BOOT_SCREEN_H
#define BOOT_SCREEN_H

#include <stdint.h>
#include "lvgl.h"

void boot_screen_create(void);
void boot_screen_set_progress(uint8_t percent, const char *status,
                              const char *detail);
void boot_screen_close(lv_obj_t *next_screen);

#endif
