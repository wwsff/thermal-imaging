#ifndef COLOR_H
#define COLOR_H

#include "main.h"
#include "lcd.h"

// LCD和MLX90640的分辨率
#define lcd_x 230.0f // 230*172,显示图像的分辨率
#define lcd_y 172.0f
#define LCD_THERMAL_WIDTH  230U
#define LCD_THERMAL_HEIGHT 172U
#define mlx_x 32 // 32*24,mlx90640的分辨率
#define mlx_y 24
#define dis_start_x 88 // 显示起始点
#define dis_start_y 66
//光标用
#define HIGH_TEMP_AREA_MAX_COUNT 10U

typedef struct {
    float avg_temp;     // 区域平均温度
    float total_temp;   // 区域总温度（用于计算平均温度）
    int center_x;       // 区域中心x坐标
    int center_y;       // 区域中心y坐标
    int pixel_count;    // 区域像素数量
    int min_x, max_x;   // 区域边界
    int min_y, max_y;   // 区域边界
} HighTempArea;
void LCD_dis_mode_0(float *temp_input, float max_temp, float min_temp);
void LCD_dis_mode_1(float *temp_input, float max_temp, float min_temp);
void LCD_dis_mode_2(float *temp_input, float max_temp, float min_temp);
void LCD_dis_mode_3(float *temp_input, float max_temp, float min_temp);
void lcd_dis_mode_choose(color_mode dis_mode, float max_temp, float min_temp);
void lcd_dis_mode_choose_single(color_mode dis_mode, int32_t x, int32_t y, float max_temp, float min_temp);

void lcd_dis_point(int32_t x, int32_t y,float max_temp);
void LCD_dis_point_curse(void);
#endif
