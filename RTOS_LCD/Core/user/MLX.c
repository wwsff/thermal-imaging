#include "MLX.h"

extern paramsMLX90640 mlx90640;
extern float mlx90640To[temp_count]; // 温度
extern _m_tp_dev tp_dev;             // lcd触屏使用
extern color_mode dis_mode;
extern float max_temp, min_temp, cent_temp;

/*
 * @brief 读取MLX90640数据转换为温度数据
 * @param frameData: 存储MLX90640数据的数组
 * @param vdd: 输出的Vdd值
 * @param tr: 输出的tr值
 * @param max_temp: 输出的最大温度
 * @param min_temp: 输出的最小温度
 * @retval None
 */
static int MLX90640_Data_temp(uint16_t *frameData, float *vdd, float *tr,
                              float *frame_max_temp, float *frame_min_temp,
                              float *frame_cent_temp)
{
    int frame_status = MLX90640_GetFrameData(MLX90640_I2C_ADDRESS, frameData);
    if (frame_status < 0)
    {
        return frame_status;
    }

    *vdd = MLX90640_GetVdd(frameData, &mlx90640);                            // 计算 Vdd
    float Ta = MLX90640_GetTa(frameData, &mlx90640);                         // 计算实时外壳温度
    *tr = Ta - TA_SHIFT;                                                     // 计算外部温度
    MLX90640_CalculateTo(frameData, &mlx90640, emissivity, *tr, mlx90640To); // 计算像素点温度
    MLX90640_BadPixelsCorrection(mlx90640.brokenPixels, mlx90640To, 1, &mlx90640);
    MLX90640_BadPixelsCorrection(mlx90640.outlierPixels, mlx90640To, 1, &mlx90640);

    *frame_max_temp = -100.0f;
    *frame_min_temp = 100.0f;

    for (uint16_t i = 0; i < temp_count; i++)
    {
        if (mlx90640To[i] > *frame_max_temp)
        {
            *frame_max_temp = mlx90640To[i];
        }
        if (mlx90640To[i] < *frame_min_temp)
        {
            *frame_min_temp = mlx90640To[i];
        }
    }
    *frame_cent_temp = (mlx90640To[11U * mlx_x + 15U] + mlx90640To[11U * mlx_x + 16U] +
                        mlx90640To[12U * mlx_x + 15U] + mlx90640To[12U * mlx_x + 16U]) * 0.25f;
    return frame_status;
}

int MLX_acquire(uint16_t *frameData)
{
    float vdd;
    float tr;
    float frame_max_temp;
    float frame_min_temp;
    float frame_cent_temp;
    int status = MLX90640_Data_temp(frameData, &vdd, &tr, &frame_max_temp,
                                    &frame_min_temp, &frame_cent_temp);

    if (status >= 0)
    {
        max_temp = frame_max_temp;
        min_temp = frame_min_temp;
        cent_temp = frame_cent_temp;
    }
    return status;
}

void MLX_render(void)
{
    lcd_dis_mode_choose(dis_mode, max_temp, min_temp);
}

/*
 * @brief 使用MLX90640及通过LCD检测
 * @param frameData: 存储MLX90640数据的数组
 * @retval None
 */
void MLX_use(uint16_t *frameData)
{
    if (MLX_acquire(frameData) >= 0)
    {
        MLX_render();
    }
}
