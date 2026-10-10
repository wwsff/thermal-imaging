#ifndef MLX_H
#define MLX_H

#include "main.h"
#include "../LCD/lcd.h"
#include "touch.h"
#include "stdio.h"
#include "color.h"

#define TA_SHIFT 8.0f
#define emissivity 0.95f // 环境光

int MLX_acquire(uint16_t *frameData);
void MLX_render(void);
void MLX_use(uint16_t *frameData);

#endif
