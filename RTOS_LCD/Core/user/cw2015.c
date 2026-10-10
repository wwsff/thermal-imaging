#include "cw2015.h"

#include "i2c.h"
#include "cmsis_os.h"

#define CW2015_I2C_ADDRESS_7BIT 0x62U
#define CW2015_REG_VERSION      0x00U
#define CW2015_REG_VCELL        0x02U
#define CW2015_REG_SOC          0x04U
#define CW2015_REG_MODE         0x0AU

#define CW2015_MODE_NORMAL      0x00U
#define CW2015_MODE_SLEEP_MASK  0xC0U
#define CW2015_MODE_QUICK_START 0x30U

static uint8_t cw2015_initialized = 0U;

static int cw2015_write_register(uint8_t reg, uint8_t value)
{
    return (HAL_I2C_Mem_Write(&hi2c1, CW2015_I2C_ADDRESS_7BIT << 1,
                              reg, I2C_MEMADD_SIZE_8BIT, &value, 1U,
                              30U) == HAL_OK) ? 0 : -1;
}

static int cw2015_initialize(uint8_t *version)
{
    uint8_t mode;

    if (HAL_I2C_IsDeviceReady(&hi2c1, CW2015_I2C_ADDRESS_7BIT << 1,
                              3U, 30U) != HAL_OK)
    {
        return -2;
    }

    if (HAL_I2C_Mem_Read(&hi2c1, CW2015_I2C_ADDRESS_7BIT << 1,
                         CW2015_REG_MODE, I2C_MEMADD_SIZE_8BIT,
                         &mode, 1U, 30U) != HAL_OK)
    {
        return -3;
    }

    if ((mode & CW2015_MODE_SLEEP_MASK) == CW2015_MODE_SLEEP_MASK)
    {
        if (cw2015_write_register(CW2015_REG_MODE, CW2015_MODE_NORMAL) != 0)
        {
            return -4;
        }
        osDelay(20U);
    }

    /* 仅在首次初始化时快速启动一次，随后清除QSTRT控制位。 */
    if (cw2015_write_register(CW2015_REG_MODE, CW2015_MODE_QUICK_START) != 0)
    {
        return -5;
    }
    osDelay(20U);
    if (cw2015_write_register(CW2015_REG_MODE, CW2015_MODE_NORMAL) != 0)
    {
        return -6;
    }

    /* 电压寄存器每250ms更新；等待第一组有效转换结果。 */
    osDelay(300U);
    if (HAL_I2C_Mem_Read(&hi2c1, CW2015_I2C_ADDRESS_7BIT << 1,
                         CW2015_REG_VERSION, I2C_MEMADD_SIZE_8BIT,
                         version, 1U, 30U) != HAL_OK)
    {
        return -7;
    }
    if ((*version == 0x00U) || (*version == 0xFFU))
    {
        return -8;
    }

    cw2015_initialized = 1U;
    return 0;
}

int cw2015_read(cw2015_data_t *data)
{
    uint8_t version;
    uint8_t voltage_bytes[2];
    uint8_t soc_bytes[2];
    uint16_t raw_voltage;
    uint16_t raw_soc;
    uint32_t soc_x10;
    uint32_t voltage_mv;

    if (data == NULL)
    {
        return -1;
    }

    if (cw2015_initialized == 0U)
    {
        int init_status = cw2015_initialize(&version);
        if (init_status != 0)
        {
            return init_status;
        }
    }
    else if (HAL_I2C_Mem_Read(&hi2c1, CW2015_I2C_ADDRESS_7BIT << 1,
                              CW2015_REG_VERSION, I2C_MEMADD_SIZE_8BIT,
                              &version, 1U, 30U) != HAL_OK)
    {
        cw2015_initialized = 0U;
        return -3;
    }

    if (HAL_I2C_Mem_Read(&hi2c1, CW2015_I2C_ADDRESS_7BIT << 1,
                         CW2015_REG_VCELL, I2C_MEMADD_SIZE_8BIT,
                         voltage_bytes, sizeof(voltage_bytes), 30U) != HAL_OK)
    {
        cw2015_initialized = 0U;
        return -4;
    }
    if (HAL_I2C_Mem_Read(&hi2c1, CW2015_I2C_ADDRESS_7BIT << 1,
                         CW2015_REG_SOC, I2C_MEMADD_SIZE_8BIT,
                         soc_bytes, sizeof(soc_bytes), 30U) != HAL_OK)
    {
        cw2015_initialized = 0U;
        return -5;
    }

    raw_voltage = ((uint16_t)voltage_bytes[0] << 8) | voltage_bytes[1];
    raw_soc = ((uint16_t)soc_bytes[0] << 8) | soc_bytes[1];

    if ((version == 0x00U) || (version == 0xFFU) ||
        (raw_voltage == 0U) || (soc_bytes[0] > 100U))
    {
        cw2015_initialized = 0U;
        return -9;
    }

    /* CW2015: VCELL分辨率305uV；SOC低字节单位为1/256%。 */
    voltage_mv = ((uint32_t)raw_voltage * 305U + 500U) / 1000U;
    soc_x10 = ((uint32_t)raw_soc * 10U + 128U) / 256U;
    if (soc_x10 > 1000U)
    {
        soc_x10 = 1000U;
    }

    data->version = version;
    data->voltage_mv = (uint16_t)voltage_mv;
    data->soc_x10 = (uint16_t)soc_x10;
    return 0;
}
