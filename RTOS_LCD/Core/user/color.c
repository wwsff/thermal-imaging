#include "cmsis_os.h"
#include "color.h"
#include "app_state.h"
#include "stdio.h"
#include "string.h"
extern float mlx90640To[temp_count]; // 温度，32*24
extern osSemaphoreId_t myBinarySem01Handle;

typedef struct
{
  uint8_t x;
  uint8_t y;
} HighTempPoint;

/* 仅由图像任务使用。放在静态区，避免每帧在任务栈上申请约 7 KB。 */
static uint8_t high_temp_visited[mlx_x * mlx_y];
static HighTempPoint high_temp_queue[mlx_x * mlx_y];
/*
 * @brief 将温度数据转换为颜色，并返回RGB565格式的颜色值，根据最大最小值自动调整颜色范围
 * @param temp_input: 温度数据
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval uint16_t: 转换后的颜色值
 */
uint16_t temp_colorful_auto(float temp_input, float max_temp, float min_temp)
{
  // 确保温度在有效范围内
  if (temp_input < min_temp)
    temp_input = min_temp;

  // 计算温度在[min_temp, max_temp]区间内的比例(0.0-1.0)
  float ratio = (temp_input - min_temp) / (max_temp - min_temp);

  // 根据比例计算RGB分量
  uint8_t r, g, b;

  // 从蓝色(低温)到红色(高温)的渐变
  if (ratio < 0.5f)
  {
    // 蓝色到绿色渐变
    r = 0;
    g = (uint8_t)(ratio * 2 * 255);
    b = (uint8_t)((1.0f - ratio * 2) * 255);
  }
  else
  {
    // 绿色到红色渐变
    r = (uint8_t)((ratio - 0.5f) * 2 * 255);
    g = (uint8_t)((1.0f - (ratio - 0.5f) * 2) * 255);
    b = 0;
  }

  // 将RGB888转换为RGB565格式
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
/*
 * @brief 绘制温度数据到LCD,LCD彩虹模式
 * @param temp_input: 温度数据数组
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval None
 */
void LCD_dis_mode_0(float *temp_input, float max_temp, float min_temp)
{
  // 计算缩放比例
  const float x_scale = (mlx_x - 1.0f) / (lcd_x - 1.0f); // 注意这里是减1，因为LCD的坐标是从0开始的
  const float y_scale = (mlx_y - 1.0f) / (lcd_y - 1.0f);

  // 遍历lcd_x*lcd_y的每个像素点,线性插值部分
  for (int y = 0; y < lcd_y; y++)
  {
    for (int x = 0; x < lcd_x; x++)
    {
      // 计算对应的原始坐标
      float x_orig = (lcd_x - 1.0f - x) * x_scale; // 与当前LCD横屏扫描方向匹配
      float y_orig = y * y_scale;

      // 获取最近的四个原始点
      int x0 = (int)x_orig;
      int y0 = (int)y_orig;
      int x1 = x0 + 1;
      int y1 = y0 + 1;

      // 边界检查
      if (x1 >= mlx_x)
        x1 = mlx_x - 1;
      if (y1 >= mlx_y)
        y1 = mlx_y - 1;

      // 计算权重
      float x_weight = x_orig - x0;
      float y_weight = y_orig - y0;

      // 双线性插值计算温度值
      float temp =
          temp_input[y0 * (uint8_t)mlx_x + x0] * (1 - x_weight) * (1 - y_weight) +
          temp_input[y0 * (uint8_t)mlx_x + x1] * x_weight * (1 - y_weight) +
          temp_input[y1 * (uint8_t)mlx_x + x0] * (1 - x_weight) * y_weight +
          temp_input[y1 * (uint8_t)mlx_x + x1] * x_weight * y_weight;

      // 绘制像素点
      lcd_draw_point(x + dis_start_x, y + dis_start_y, temp_colorful_auto(temp, max_temp, min_temp));
    }
  }
}
/*
 * @brief 将温度数据转换为灰度值，白色高温，并返回RGB565格式的颜色值，根据最大最小值自动调整灰度范围
 * @param temp_input: 温度数据
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval uint16_t: 转换后的颜色值
 */
uint16_t temp_white_auto(float temp_input, float max_temp, float min_temp)
{
  // 确保温度在有效范围内
  if (temp_input < min_temp)
    temp_input = min_temp;
  if (temp_input > max_temp)
    temp_input = max_temp;

  // 计算温度在[min_temp, max_temp]区间内的比例(0.0-1.0)
  float ratio = (temp_input - min_temp) / (max_temp - min_temp);

  // 根据比例计算灰度值(0-255)
  uint8_t gray = (uint8_t)(ratio * 255);

  // 将灰度值转换为RGB565格式(白色渐变)
  return ((gray & 0xF8) << 8) | ((gray & 0xFC) << 3) | (gray >> 3);
}
/*
 * @brief 绘制温度数据到LCD,LCD黑白模式
 * @param temp_input: 温度数据数组
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval None
 */
void LCD_dis_mode_1(float *temp_input, float max_temp, float min_temp)
{
  UNUSED(temp_input);
  UNUSED(max_temp);
  UNUSED(min_temp);
  // 计算缩放比例
  const float x_scale = (mlx_x - 1.0f) / (lcd_x - 1.0f); // 注意这里是减1，因为LCD的坐标是从0开始的
  const float y_scale = (mlx_y - 1.0f) / (lcd_y - 1.0f);

  // 遍历160x120的每个像素点,线性插值部分
  for (int y = 0; y < lcd_y; y++)
  {
    for (int x = 0; x < lcd_x; x++)
    {
      // 计算对应的原始坐标
      float x_orig = (lcd_x - 1.0f - x) * x_scale; // 与当前LCD横屏扫描方向匹配
      float y_orig = y * y_scale;

      // 获取最近的四个原始点
      int x0 = (int)x_orig;
      int y0 = (int)y_orig;
      int x1 = x0 + 1;
      int y1 = y0 + 1;

      // 边界检查
      if (x1 >= mlx_x)
        x1 = mlx_x - 1;
      if (y1 >= mlx_y)
        y1 = mlx_y - 1;

      // 计算权重
      float x_weight = x_orig - x0;
      float y_weight = y_orig - y0;

      // 双线性插值计算温度值
      float temp =
          temp_input[y0 * (uint8_t)mlx_x + x0] * (1 - x_weight) * (1 - y_weight) +
          temp_input[y0 * (uint8_t)mlx_x + x1] * x_weight * (1 - y_weight) +
          temp_input[y1 * (uint8_t)mlx_x + x0] * (1 - x_weight) * y_weight +
          temp_input[y1 * (uint8_t)mlx_x + x1] * x_weight * y_weight;

      // 绘制像素点
      lcd_draw_point(x + dis_start_x, y + dis_start_y, temp_white_auto(temp, max_temp, min_temp));
    }
  }
}
/*
 * @brief 将温度数据转换为灰度值，黑色高温，并返回RGB565格式的颜色值，根据最大最小值自动调整灰度范围
 * @param temp_input: 温度数据
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval uint16_t: 转换后的颜色值
 */
uint16_t temp_black_auto(float temp_input, float max_temp, float min_temp)
{
  // 确保温度在有效范围内
  if (temp_input < min_temp)
    temp_input = min_temp;
  if (temp_input > max_temp)
    temp_input = max_temp;

  // 计算温度在[min_temp, max_temp]区间内的比例(0.0-1.0)
  float ratio = (temp_input - min_temp) / (max_temp - min_temp);

  // 根据比例计算灰度值(0-255)，温度越高值越小(越黑)
  uint8_t gray = (uint8_t)((1.0f - ratio) * 255);

  // 将灰度值转换为RGB565格式(黑色渐变)
  return ((gray & 0xF8) << 8) | ((gray & 0xFC) << 3) | (gray >> 3);
}
/*
 * @brief 绘制温度数据到LCD,LCD黑白模式
 * @param temp_input: 温度数据数组
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval None
 */
void LCD_dis_mode_2(float *temp_input, float max_temp, float min_temp)
{
  UNUSED(temp_input);
  UNUSED(max_temp);
  UNUSED(min_temp);
  const float x_scale = (mlx_x - 1.0f) / (lcd_x - 1.0f); // 注意这里是减1，因为LCD的坐标是从0开始的
  const float y_scale = (mlx_y - 1.0f) / (lcd_y - 1.0f);

  // 遍历160x120的每个像素点,线性插值部分
  for (int y = 0; y < lcd_y; y++)
  {
    for (int x = 0; x < lcd_x; x++)
    {
      // 计算对应的原始坐标
      float x_orig = (lcd_x - 1.0f - x) * x_scale; // 与当前LCD横屏扫描方向匹配
      float y_orig = y * y_scale;

      // 获取最近的四个原始点
      int x0 = (int)x_orig;
      int y0 = (int)y_orig;
      int x1 = x0 + 1;
      int y1 = y0 + 1;

      // 边界检查
      if (x1 >= mlx_x)
        x1 = mlx_x - 1;
      if (y1 >= mlx_y)
        y1 = mlx_y - 1;

      // 计算权重
      float x_weight = x_orig - x0;
      float y_weight = y_orig - y0;

      // 双线性插值计算温度值
      float temp =
          temp_input[y0 * (uint8_t)mlx_x + x0] * (1 - x_weight) * (1 - y_weight) +
          temp_input[y0 * (uint8_t)mlx_x + x1] * x_weight * (1 - y_weight) +
          temp_input[y1 * (uint8_t)mlx_x + x0] * (1 - x_weight) * y_weight +
          temp_input[y1 * (uint8_t)mlx_x + x1] * x_weight * y_weight;

      // 绘制像素点
      lcd_draw_point(x + dis_start_x, y + dis_start_y, temp_black_auto(temp, max_temp, min_temp));
    }
  }
}
/*
 * @brief 将温度数据转换为颜色，并返回RGB565格式的颜色值，根据最大最小值自动调整颜色范围
 * @param temp_input: 温度数据
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval uint16_t: 转换后的颜色值
 */
uint16_t temp_medical_auto(float temp_input, float max_temp, float min_temp)
{
  // 确保温度在有效范围内
  if (temp_input < min_temp)
    temp_input = min_temp;
  if (temp_input > max_temp)
    temp_input = max_temp;

  // 计算温度在[min_temp, max_temp]区间内的比例(0.0-1.0)
  float ratio = (temp_input - min_temp) / (max_temp - min_temp);

  // 医疗模式颜色映射 - 突出显示微小温差
  uint8_t r, g, b;

  // 温度区间划分(可根据需要调整)
  if (ratio < 0.2f)
  {
    // 低温区: 深蓝色
    r = 0;
    g = 0;
    b = (uint8_t)(128 + ratio * 127 / 0.2f);
  }
  else if (ratio < 0.4f)
  {
    // 中低温区: 蓝绿色
    r = 0;
    g = (uint8_t)((ratio - 0.2f) * 255 / 0.2f);
    b = 255;
  }
  else if (ratio < 0.6f)
  {
    // 中温区: 绿色
    r = 0;
    g = 255;
    b = (uint8_t)(255 - (ratio - 0.4f) * 255 / 0.2f);
  }
  else if (ratio < 0.8f)
  {
    // 中高温区: 黄色
    r = (uint8_t)((ratio - 0.6f) * 255 / 0.2f);
    g = 255;
    b = 0;
  }
  else
  {
    // 高温区: 红色
    r = 255;
    g = (uint8_t)(255 - (ratio - 0.8f) * 255 / 0.2f);
    b = 0;
  }

  // 将RGB888转换为RGB565格式
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
/*
 * @brief 绘制温度数据到LCD,LCD医疗模式
 * @param temp_input: 温度数据数组
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval None
 */
void LCD_dis_mode_3(float *temp_input, float max_temp, float min_temp)
{
  UNUSED(temp_input);
  UNUSED(max_temp);
  UNUSED(min_temp);
  const float x_scale = (mlx_x - 1.0f) / (lcd_x - 1.0f); // 注意这里是减1，因为LCD的坐标是从0开始的
  const float y_scale = (mlx_y - 1.0f) / (lcd_y - 1.0f);

  // 遍历160x120的每个像素点,线性插值部分
  for (int y = 0; y < lcd_y; y++)
  {
    for (int x = 0; x < lcd_x; x++)
    {
      // 计算对应的原始坐标
      float x_orig = (lcd_x - 1.0f - x) * x_scale; // 与当前LCD横屏扫描方向匹配
      float y_orig = y * y_scale;

      // 获取最近的四个原始点
      int x0 = (int)x_orig;
      int y0 = (int)y_orig;
      int x1 = x0 + 1;
      int y1 = y0 + 1;

      // 边界检查
      if (x1 >= mlx_x)
        x1 = mlx_x - 1;
      if (y1 >= mlx_y)
        y1 = mlx_y - 1;

      // 计算权重
      float x_weight = x_orig - x0;
      float y_weight = y_orig - y0;

      // 双线性插值计算温度值
      float temp =
          temp_input[y0 * (uint8_t)mlx_x + x0] * (1 - x_weight) * (1 - y_weight) +
          temp_input[y0 * (uint8_t)mlx_x + x1] * x_weight * (1 - y_weight) +
          temp_input[y1 * (uint8_t)mlx_x + x0] * (1 - x_weight) * y_weight +
          temp_input[y1 * (uint8_t)mlx_x + x1] * x_weight * y_weight;

      // 绘制像素点
      lcd_draw_point(x + dis_start_x, y + dis_start_y, temp_medical_auto(temp, max_temp, min_temp));
    }
  }
}

typedef uint16_t (*temperature_color_mapper_t)(float temperature, float max_temperature,
                                                float min_temperature);

#define THERMAL_STRIPE_ROWS 8U

static void lcd_render_thermal_fast(float *temperature_data, float max_temperature,
                                    float min_temperature, color_mode mode)
{
  static uint8_t mapping_ready = 0U;
  static uint8_t x0_map[LCD_THERMAL_WIDTH];
  static uint8_t x1_map[LCD_THERMAL_WIDTH];
  static float x_weight_map[LCD_THERMAL_WIDTH];
  static uint8_t y0_map[LCD_THERMAL_HEIGHT];
  static uint8_t y1_map[LCD_THERMAL_HEIGHT];
  static float y_weight_map[LCD_THERMAL_HEIGHT];
  static uint16_t stripe_buffer[LCD_THERMAL_WIDTH * THERMAL_STRIPE_ROWS];
  temperature_color_mapper_t mapper = temp_colorful_auto;

  if (mapping_ready == 0U)
  {
    const float x_scale = (mlx_x - 1.0f) / (LCD_THERMAL_WIDTH - 1.0f);
    const float y_scale = (mlx_y - 1.0f) / (LCD_THERMAL_HEIGHT - 1.0f);

    for (uint16_t x = 0U; x < LCD_THERMAL_WIDTH; x++)
    {
      float source_x = (LCD_THERMAL_WIDTH - 1.0f - x) * x_scale;
      uint8_t source_x0 = (uint8_t)source_x;
      uint8_t source_x1 = source_x0 + 1U;
      if (source_x1 >= mlx_x)
      {
        source_x1 = mlx_x - 1U;
      }
      x0_map[x] = source_x0;
      x1_map[x] = source_x1;
      x_weight_map[x] = source_x - source_x0;
    }

    for (uint16_t y = 0U; y < LCD_THERMAL_HEIGHT; y++)
    {
      float source_y = y * y_scale;
      uint8_t source_y0 = (uint8_t)source_y;
      uint8_t source_y1 = source_y0 + 1U;
      if (source_y1 >= mlx_y)
      {
        source_y1 = mlx_y - 1U;
      }
      y0_map[y] = source_y0;
      y1_map[y] = source_y1;
      y_weight_map[y] = source_y - source_y0;
    }
    mapping_ready = 1U;
  }

  switch (mode)
  {
  case white_hot_mode:
    mapper = temp_white_auto;
    break;
  case black_hot_mode:
    mapper = temp_black_auto;
    break;
  case medical_mode:
    mapper = temp_medical_auto;
    break;
  case colorful_mode:
  default:
    mapper = temp_colorful_auto;
    break;
  }

  if ((max_temperature - min_temperature) < 0.01f)
  {
    max_temperature = min_temperature + 0.01f;
  }

  for (uint16_t stripe_y = 0U; stripe_y < LCD_THERMAL_HEIGHT; stripe_y += THERMAL_STRIPE_ROWS)
  {
    uint16_t rows = LCD_THERMAL_HEIGHT - stripe_y;
    if (rows > THERMAL_STRIPE_ROWS)
    {
      rows = THERMAL_STRIPE_ROWS;
    }

    if (g_app_page != APP_PAGE_THERMAL)
    {
      return;
    }

    for (uint16_t local_y = 0U; local_y < rows; local_y++)
    {
      uint16_t y = stripe_y + local_y;
      uint8_t y0 = y0_map[y];
      uint8_t y1 = y1_map[y];
      float wy = y_weight_map[y];
      float inv_wy = 1.0f - wy;

      for (uint16_t x = 0U; x < LCD_THERMAL_WIDTH; x++)
      {
        uint8_t x0 = x0_map[x];
        uint8_t x1 = x1_map[x];
        float wx = x_weight_map[x];
        float top = temperature_data[y0 * mlx_x + x0] * (1.0f - wx) +
                    temperature_data[y0 * mlx_x + x1] * wx;
        float bottom = temperature_data[y1 * mlx_x + x0] * (1.0f - wx) +
                       temperature_data[y1 * mlx_x + x1] * wx;
        float temperature = top * inv_wy + bottom * wy;

        stripe_buffer[local_y * LCD_THERMAL_WIDTH + x] =
            mapper(temperature, max_temperature, min_temperature);
      }
    }

    /*
     * 当前LCD横屏GRAM为换轴扫描。一次发送多行会让一行末尾的数据绕到
     * 区域左侧，形成按键旁边的错误色条。逐行开窗可保证每次DMA恰好在
     * 230像素边界结束，同时仍保留像素搬运DMA。
     */
    osSemaphoreAcquire(myBinarySem01Handle, osWaitForever);
    if (g_app_page != APP_PAGE_THERMAL)
    {
      osSemaphoreRelease(myBinarySem01Handle);
      return;
    }

    for (uint16_t local_y = 0U; local_y < rows; local_y++)
    {
      uint16_t row_y = dis_start_y + stripe_y + local_y;
      uint16_t *row = &stripe_buffer[local_y * LCD_THERMAL_WIDTH];

      if (lcd_color_fill_dma(dis_start_x,
                             row_y,
                             dis_start_x + LCD_THERMAL_WIDTH - 1U,
                             row_y,
                             row,
                             10U) != HAL_OK)
      {
        lcd_color_fill(dis_start_x,
                       row_y,
                       dis_start_x + LCD_THERMAL_WIDTH - 1U,
                       row_y,
                       row);
      }
    }
    osSemaphoreRelease(myBinarySem01Handle);
  }
}

/*
 * @brief 显示模式选择
 * @param temp_input: 温度数据数组
 * @param max_temp: ??大温??
 * @param min_temp: ??小温??
 * @retval None
 * @note 0: 彩色
 *       1: 白热
 *       2: 黑热
 *       3: 医疗
 */
void lcd_dis_mode_choose(color_mode dis_mode, float max_temp, float min_temp)
{
  lcd_render_thermal_fast(mlx90640To, max_temp, min_temp, dis_mode);
  if (g_app_page != APP_PAGE_THERMAL)
  {
    return;
  }

  osSemaphoreAcquire(myBinarySem01Handle, osWaitForever);
  if (g_app_page == APP_PAGE_THERMAL)
  {
    LCD_dis_point_curse();
  }
  osSemaphoreRelease(myBinarySem01Handle);
}

/*
 * @brief 绘制单个像素点的温度数据到LCD(彩虹模式)
 * @param temp_input: 温度数据数组
 * @param x: x坐标
 * @param y: y坐标
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval None
 */
void LCD_dis_mode_0_single(float *temp_input, uint16_t x, uint16_t y, float max_temp, float min_temp)
{
  const float x_scale = (mlx_x - 1.0f) / (lcd_x - 1.0f);
  const float y_scale = (mlx_y - 1.0f) / (lcd_y - 1.0f);

  float x_orig = (lcd_x - 1.0f - x) * x_scale;
  float y_orig = y * y_scale;

  int x0 = (int)x_orig;
  int y0 = (int)y_orig;
  int x1 = x0 + 1;
  int y1 = y0 + 1;

  if (x1 >= mlx_x)
    x1 = mlx_x - 1;
  if (y1 >= mlx_y)
    y1 = mlx_y - 1;

  float x_weight = x_orig - x0;
  float y_weight = y_orig - y0;

  float temp =
      temp_input[y0 * (uint8_t)mlx_x + x0] * (1 - x_weight) * (1 - y_weight) +
      temp_input[y0 * (uint8_t)mlx_x + x1] * x_weight * (1 - y_weight) +
      temp_input[y1 * (uint8_t)mlx_x + x0] * (1 - x_weight) * y_weight +
      temp_input[y1 * (uint8_t)mlx_x + x1] * x_weight * y_weight;

  lcd_draw_point(x + dis_start_x, y + dis_start_y, temp_colorful_auto(temp, max_temp, min_temp));
}

/*
 * @brief 绘制单个像素点的温度数据到LCD(白热模式)
 * @param temp_input: 温度数据数组
 * @param x: x坐标
 * @param y: y坐标
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval None
 */
void LCD_dis_mode_1_single(float *temp_input, uint16_t x, uint16_t y, float max_temp, float min_temp)
{
  const float x_scale = (mlx_x - 1.0f) / (lcd_x - 1.0f);
  const float y_scale = (mlx_y - 1.0f) / (lcd_y - 1.0f);

  float x_orig = (lcd_x - 1.0f - x) * x_scale;
  float y_orig = y * y_scale;

  int x0 = (int)x_orig;
  int y0 = (int)y_orig;
  int x1 = x0 + 1;
  int y1 = y0 + 1;

  if (x1 >= mlx_x)
    x1 = mlx_x - 1;
  if (y1 >= mlx_y)
    y1 = mlx_y - 1;

  float x_weight = x_orig - x0;
  float y_weight = y_orig - y0;

  float temp =
      temp_input[y0 * (uint8_t)mlx_x + x0] * (1 - x_weight) * (1 - y_weight) +
      temp_input[y0 * (uint8_t)mlx_x + x1] * x_weight * (1 - y_weight) +
      temp_input[y1 * (uint8_t)mlx_x + x0] * (1 - x_weight) * y_weight +
      temp_input[y1 * (uint8_t)mlx_x + x1] * x_weight * y_weight;

  lcd_draw_point(x + dis_start_x, y + dis_start_y, temp_white_auto(temp, max_temp, min_temp));
}

/*
 * @brief 绘制单个像素点的温度数据到LCD(黑热模式)
 * @param temp_input: 温度数据数组
 * @param x: x坐标
 * @param y: y坐标
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval None
 */
void LCD_dis_mode_2_single(float *temp_input, uint16_t x, uint16_t y, float max_temp, float min_temp)
{
  const float x_scale = (mlx_x - 1.0f) / (lcd_x - 1.0f);
  const float y_scale = (mlx_y - 1.0f) / (lcd_y - 1.0f);

  float x_orig = (lcd_x - 1.0f - x) * x_scale;
  float y_orig = y * y_scale;

  int x0 = (int)x_orig;
  int y0 = (int)y_orig;
  int x1 = x0 + 1;
  int y1 = y0 + 1;

  if (x1 >= mlx_x)
    x1 = mlx_x - 1;
  if (y1 >= mlx_y)
    y1 = mlx_y - 1;

  float x_weight = x_orig - x0;
  float y_weight = y_orig - y0;

  float temp =
      temp_input[y0 * (uint8_t)mlx_x + x0] * (1 - x_weight) * (1 - y_weight) +
      temp_input[y0 * (uint8_t)mlx_x + x1] * x_weight * (1 - y_weight) +
      temp_input[y1 * (uint8_t)mlx_x + x0] * (1 - x_weight) * y_weight +
      temp_input[y1 * (uint8_t)mlx_x + x1] * x_weight * y_weight;

  lcd_draw_point(x + dis_start_x, y + dis_start_y, temp_black_auto(temp, max_temp, min_temp));
}

/*
 * @brief 绘制单个像素点的温度数据到LCD(医疗模式)
 * @param temp_input: 温度数据数组
 * @param x: x坐标
 * @param y: y坐标
 * @param max_temp: 最大温度
 * @param min_temp: 最小温度
 * @retval None
 */
void LCD_dis_mode_3_single(float *temp_input, uint16_t x, uint16_t y, float max_temp, float min_temp)
{
  const float x_scale = (mlx_x - 1.0f) / (lcd_x - 1.0f);
  const float y_scale = (mlx_y - 1.0f) / (lcd_y - 1.0f);

  float x_orig = (lcd_x - 1.0f - x) * x_scale;
  float y_orig = y * y_scale;

  int x0 = (int)x_orig;
  int y0 = (int)y_orig;
  int x1 = x0 + 1;
  int y1 = y0 + 1;

  if (x1 >= mlx_x)
    x1 = mlx_x - 1;
  if (y1 >= mlx_y)
    y1 = mlx_y - 1;

  float x_weight = x_orig - x0;
  float y_weight = y_orig - y0;

  float temp =
      temp_input[y0 * (uint8_t)mlx_x + x0] * (1 - x_weight) * (1 - y_weight) +
      temp_input[y0 * (uint8_t)mlx_x + x1] * x_weight * (1 - y_weight) +
      temp_input[y1 * (uint8_t)mlx_x + x0] * (1 - x_weight) * y_weight +
      temp_input[y1 * (uint8_t)mlx_x + x1] * x_weight * y_weight;

  lcd_draw_point(x + dis_start_x, y + dis_start_y, temp_medical_auto(temp, max_temp, min_temp));
}

/*
 * @brief 显示模式选择
 * @param temp_input: 温度数据数组
 * @param max_temp: ??大温??
 * @param min_temp: ??小温??
 * @retval None
 * @note 0: 彩色
 *       1: 白热
 *       2: 黑热
 *       3: 医疗
 */
void lcd_dis_mode_choose_single(color_mode dis_mode, int32_t x, int32_t y, float max_temp, float min_temp)
{
  switch (dis_mode)
  {
  case colorful_mode:
    LCD_dis_mode_0_single(mlx90640To, x, y, max_temp, min_temp); // 彩色
    break;
  case white_hot_mode:
    LCD_dis_mode_1_single(mlx90640To, x, y, max_temp, min_temp); // 白热
    break;
  case black_hot_mode:
    LCD_dis_mode_2_single(mlx90640To, x, y, max_temp, min_temp); // 黑热
    break;
  case medical_mode:
    LCD_dis_mode_3_single(mlx90640To, x, y, max_temp, min_temp); // 医疗
    break;
  }
}
/*
 * @brief 绘制高温度点和温度值
 * @param x: x坐标
 * @param y: y坐标
 * @param max_temp: 最大温度
 * @retval None
 */
void lcd_dis_point(int32_t x, int32_t y, float max_temp)
{
  const int32_t cursor_size = 9;
  const int32_t cursor_arm = 4;
  const float x_scale = (mlx_x - 1.0f) / (lcd_x - 1.0f); 
  const float y_scale = (mlx_y - 1.0f) / (lcd_y - 1.0f);
  // 光标图案
  // 将 MLX 坐标映射到LCD；随后留出光标大小的边距，避免越界
  int32_t sx = dis_start_x + (int32_t)LCD_THERMAL_WIDTH - 1 -
               (int32_t)(x / x_scale) - cursor_size / 2;
  int32_t sy = dis_start_y + (int32_t)(y / y_scale) - cursor_size / 2;
  // 对9x9光标进行边界夹紧
  int32_t min_x = dis_start_x;
  int32_t min_y = dis_start_y;
  int32_t max_x = dis_start_x + lcd_x - cursor_size;
  int32_t max_y = dis_start_y + lcd_y - cursor_size;
  if (sx < min_x) sx = min_x;
  if (sy < min_y) sy = min_y;
  if (sx > max_x) sx = max_x;
  if (sy > max_y) sy = max_y;
  uint8_t str[20] = {0};

  // 显式绘制等宽的四个白色角，避免位图方向造成某一角亮度不一致
  for (int32_t thickness = 0; thickness < 2; thickness++)
  {
    for (int32_t arm = 0; arm < cursor_arm; arm++)
    {
      lcd_draw_point(sx + arm, sy + thickness, WHITE);
      lcd_draw_point(sx + thickness, sy + arm, WHITE);
      lcd_draw_point(sx + cursor_size - 1 - arm, sy + thickness, WHITE);
      lcd_draw_point(sx + cursor_size - 1 - thickness, sy + arm, WHITE);
      lcd_draw_point(sx + arm, sy + cursor_size - 1 - thickness, WHITE);
      lcd_draw_point(sx + thickness, sy + cursor_size - 1 - arm, WHITE);
      lcd_draw_point(sx + cursor_size - 1 - arm, sy + cursor_size - 1 - thickness, WHITE);
      lcd_draw_point(sx + cursor_size - 1 - thickness, sy + cursor_size - 1 - arm, WHITE);
    }
  }
  lcd_draw_point(sx + cursor_size / 2, sy + cursor_size / 2, WHITE);

  // 显示温度值
  sprintf((char *)str, "%.2f", max_temp);
  // 文本优先放在光标右下；靠近边缘时自动翻到左侧或上方
  int32_t tx = sx + cursor_size + 2;
  int32_t ty = sy + cursor_size + 2;
  if ((tx + 48) > (dis_start_x + lcd_x)) tx = sx - 50;
  if ((ty + 16) > (dis_start_y + lcd_y)) ty = sy - 18;
  if (tx < dis_start_x) tx = dis_start_x;
  if (ty < dis_start_y) ty = dis_start_y;
  lcd_show_string_2(tx, ty, 3 * 16, 16, 16, (char *)str, WHITE);
}
/*
 * @brief 使用BFS算法查找温度较高的区域
 * @param high_temp_areas: 存储高温区域信息的数组指针
 * @param area_count: 高温区域个数的指针
 * @param threshold_ratio: 温度阈值比例(相对于整体温度范围)
 * @retval None
 * @note 此函数使用BFS算法识别温度较高的连续区域
 */
/*
 * @brief 使用BFS算法查找温度较高的区域
 * @param high_temp_areas: 存储高温区域信息的数组指针
 * @param area_count: 高温区域个数的指针
 * @param threshold_ratio: 温度阈值比例(相对于整体温度范围)，默认建议提高到0.8-0.9
 * @param min_pixel_count: 最小像素数量，只有达到这个数量才被视为有效高温区域
 * @retval None
 * @note 此函数使用BFS算法识别温度较高的连续区域
 */
void find_high_temp_areas(HighTempArea *high_temp_areas, int *area_count, float threshold_ratio, int min_pixel_count)
{
    // 确保参数有效
    if (high_temp_areas == NULL || area_count == NULL)
        return;
    
    // 初始化区域计数
    *area_count = 0;
    
    // 计算整体温度范围
    float current_max_temp = -100.0f;
    float current_min_temp = 200.0f;
    for (int i = 0; i < temp_count; i++)
    {
        if (mlx90640To[i] > current_max_temp)
            current_max_temp = mlx90640To[i];
        if (mlx90640To[i] < current_min_temp)
            current_min_temp = mlx90640To[i];
    }
    
    // 计算温度阈值 - 提高阈值以降低灵敏度
    float temp_threshold = current_min_temp + threshold_ratio * (current_max_temp - current_min_temp);
    
    // 清空静态访问标记；BFS工作区不再占用图像任务栈
    memset(high_temp_visited, 0, sizeof(high_temp_visited));
    int queue_front, queue_rear;
    
    // 定义8个方向的偏移量(上、下、左、右、四个对角线)
    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    
    // 遍历所有像素点
    for (int y = 0; y < mlx_y; y++)
    {
        for (int x = 0; x < mlx_x; x++)
        {
            // 计算数组索引
            int index = y * mlx_x + x;
            
            // 如果当前点未被访问且温度高于阈值
            if (!high_temp_visited[index] && mlx90640To[index] >= temp_threshold)
            {
                // 初始化BFS队列
                queue_front = 0;
                queue_rear = 0;
                
                // 将当前点加入队列
                high_temp_queue[queue_rear].x = (uint8_t)x;
                high_temp_queue[queue_rear].y = (uint8_t)y;
                queue_rear++;
                high_temp_visited[index] = 1U;
                
                // 初始化区域信息
                HighTempArea area;
                area.pixel_count = 0;
                area.total_temp = 0.0f;
                area.min_x = x;
                area.max_x = x;
                area.min_y = y;
                area.max_y = y;
                
                // BFS遍历
                while (queue_front < queue_rear)
                {
                    // 获取队首元素
                    HighTempPoint p = high_temp_queue[queue_front];
                    queue_front++;
                    
                    // 更新区域信息
                    area.pixel_count++;
                    area.total_temp += mlx90640To[p.y * mlx_x + p.x];
                    if (p.x < area.min_x) area.min_x = p.x;
                    if (p.x > area.max_x) area.max_x = p.x;
                    if (p.y < area.min_y) area.min_y = p.y;
                    if (p.y > area.max_y) area.max_y = p.y;
                    
                    // 检查所有相邻点
                    for (int i = 0; i < 8; i++)
                    {
                        int nx = p.x + dx[i];
                        int ny = p.y + dy[i];
                        
                        // 检查边界
                        if (nx >= 0 && nx < mlx_x && ny >= 0 && ny < mlx_y)
                        {
                            int n_index = ny * mlx_x + nx;
                            
                            // 如果相邻点未被访问且温度高于阈值
                            if (!high_temp_visited[n_index] && mlx90640To[n_index] >= temp_threshold)
                            {
                                // 每个像素只入队一次，因此队列最多为 mlx_x * mlx_y
                                if (queue_rear < (mlx_x * mlx_y))
                                {
                                    high_temp_queue[queue_rear].x = (uint8_t)nx;
                                    high_temp_queue[queue_rear].y = (uint8_t)ny;
                                    queue_rear++;
                                    high_temp_visited[n_index] = 1U;
                                }
                            }
                        }
                    }
                }
                
                // 只有当区域像素数量达到最小要求时，才视为有效高温区域
                if ((area.pixel_count >= min_pixel_count) &&
                    (*area_count < (int)HIGH_TEMP_AREA_MAX_COUNT))
                {
                    // 计算区域中心坐标和平均温度
                    area.center_x = (area.min_x + area.max_x) / 2;
                    area.center_y = (area.min_y + area.max_y) / 2;
                    area.avg_temp = area.total_temp / area.pixel_count;
                    
                    // 保存区域信息
                    high_temp_areas[*area_count] = area;
                    (*area_count)++;
                }
            }
        }
    }
}

void LCD_dis_point_curse(void)
{
  HighTempArea high_temp_areas[HIGH_TEMP_AREA_MAX_COUNT]; // 存储高温区域信息的数组
  int area_count = 0;                       // 高温区域个数
  float threshold_ratio = 0.8f;             // 温度阈值比例(相对于整体温度范围)
  uint8_t min_pixel_count = 8;              // 最小像素数量，只有达到这个数量才被视为有效高温区域
  find_high_temp_areas(high_temp_areas, &area_count,threshold_ratio, min_pixel_count); // 查找高温区域
  // 绘制高温区域
  for (int i = 0; i < area_count; i++)
  {
    HighTempArea area = high_temp_areas[i];
    lcd_dis_point(area.center_x, area.center_y, area.avg_temp); // 绘制高温区域
  }
}

