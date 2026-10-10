/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
//颜色显示模式
typedef enum {
  colorful_mode = 0,	//伪彩模式
  white_hot_mode,		//白热模式
  black_hot_mode,		//黑热模式
  medical_mode			//医疗模式，高对比度
}color_mode;

typedef enum{
  MLX90640_05HZ = 0x00,	//0.5Hz
  MLX90640_1HZ = 0x01,	//1Hz
  MLX90640_2HZ = 0x02,	//2Hz
  MLX90640_4HZ = 0x03,	//4Hz
  MLX90640_8HZ = 0x04,	//8Hz
  MLX90640_16HZ = 0x05,	//16Hz
  MLX90640_32HZ = 0x06,	//32Hz
  MLX90640_64HZ = 0x07	//64Hz
}MLX90640_RefreshRate;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SPI_CS_Pin GPIO_PIN_4
#define SPI_CS_GPIO_Port GPIOA
#define SPI_SCK_Pin GPIO_PIN_5
#define SPI_SCK_GPIO_Port GPIOA
#define SPI_MISO_Pin GPIO_PIN_6
#define SPI_MISO_GPIO_Port GPIOA
#define SPI_MOSI_Pin GPIO_PIN_7
#define SPI_MOSI_GPIO_Port GPIOA
#define SP_PEN_Pin GPIO_PIN_4
#define SP_PEN_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_5
#define LED1_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_0
#define LED2_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_1
#define LED3_GPIO_Port GPIOB
#define LED4_Pin GPIO_PIN_2
#define LED4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define MLX90640_I2C_ADDRESS 0x33 // MLX90640 IIC地址
#define ee_data_count 832         // 校准数据长度
#define data_count 834            // 数据长度
#define temp_count 768            // 温度数据长度
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
