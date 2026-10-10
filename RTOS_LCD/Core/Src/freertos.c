/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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
// use ： utf-8
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include <stdio.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "color.h" //温度转颜色及LCD显示部分
#include "MLX.h"
#include "app_state.h"
#include "cw2015.h"
#include "boot_screen.h"
#include "i2c.h"

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

#include "gui_guider.h"
#include "events_init.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MLX90640_use // 使用90640

#define MLX90640_EEPROM_id_ADDRESS 0x240C // MLX90640 EEPROM id地址

#define MLX90640_OPEN_time_ms 100 // MLX90640上电稳定时间

#define SYSTEM_EVENT_UI_READY  (1UL << 0)
#define SYSTEM_EVENT_MLX_READY (1UL << 1)

#define HOT_LED_ON_TEMP_C   45.0f
#define HOT_LED_OFF_TEMP_C  43.0f
#define COLD_LED_ON_TEMP_C   5.0f
#define COLD_LED_OFF_TEMP_C  7.0f

#define BOOT_TEXT_DISPLAY_LED  "\xE6\x98\xBE\xE7\xA4\xBA\xE4\xB8\x8E LED \xE6\xA3\x80\xE6\x9F\xA5"
#define BOOT_TEXT_TESTING      "\xE6\xA3\x80\xE6\xB5\x8B\xE4\xB8\xAD"
#define BOOT_TEXT_NORMAL       "\xE6\xAD\xA3\xE5\xB8\xB8"
#define BOOT_TEXT_NOT_FOUND    "\xE6\x9C\xAA\xE8\xBF\x9E\xE6\x8E\xA5"
#define BOOT_TEXT_COMPLETE     "\xE8\x87\xAA\xE6\xA3\x80\xE5\xAE\x8C\xE6\x88\x90"

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
// LCD
extern uint32_t g_back_color;        // LCD背景色
extern _lcd_dev lcddev;              // LCD的id寄存结构体
extern _m_tp_dev tp_dev;             // lcd触屏使用结构体
color_mode dis_mode = colorful_mode; // 显示模式，默认为彩色
// 90640
uint16_t eeMLX90640[ee_data_count]; // 接收MLX90640校准数据
paramsMLX90640 mlx90640;            // 90640信息

float mlx90640To[temp_count];              // 温度
static uint16_t mlx90640Frame[data_count]; // 接收MLX90640数据

float max_temp = -100.0f, min_temp = 100.0f, cent_temp = 0; // 最大和最小温度，中心温度
volatile uint32_t g_thermal_fps_x10 = 0U;                   // 实际完成显示的热图帧率，扩大10倍
volatile uint32_t g_thermal_last_frame_tick = 0U;           // 最近一次完整热图显示时刻

volatile app_page_t g_app_page = APP_PAGE_THERMAL;
volatile uint8_t g_mlx_available = 0U;
volatile uint8_t g_battery_connected = 0U;
volatile uint8_t g_battery_version = 0U;
volatile uint32_t g_battery_soc_x10 = 0U;
volatile uint32_t g_battery_voltage_mv = 0U;
volatile uint32_t g_battery_last_update_tick = 0U;
volatile int32_t g_battery_error = 0;

lv_ui guider_ui; // 用户自定义lvgl使用的UI结构体

extern volatile MLX90640_RefreshRate refresh_rate; // 刷新率
static osEventFlagsId_t systemEventFlagsHandle;
static uint8_t thermal_hot_active = 0U;
static uint8_t thermal_cold_active = 0U;
/* USER CODE END Variables */
/* Definitions for Img_Dis_Task */
osThreadId_t Img_Dis_TaskHandle;
const osThreadAttr_t Img_Dis_Task_attributes = {
  .name = "Img_Dis_Task",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for Init_task */
osThreadId_t Init_taskHandle;
const osThreadAttr_t Init_task_attributes = {
  .name = "Init_task",
  .stack_size = 1536 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for LVGL_USE_Task */
osThreadId_t LVGL_USE_TaskHandle;
const osThreadAttr_t LVGL_USE_Task_attributes = {
  .name = "LVGL_USE_Task",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myBinarySem01 */
osSemaphoreId_t myBinarySem01Handle;
const osSemaphoreAttr_t myBinarySem01_attributes = {
  .name = "myBinarySem01"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void thermal_alert_leds_off(void);
static void thermal_alert_leds_update(float maximum_temperature);
static void boot_ui_update(uint8_t percent, const char *status,
                           const char *detail);
static void boot_led_self_test(void);

/* USER CODE END FunctionPrototypes */

void Img_Dis_Fun(void *argument);
void Init_task_Function(void *argument);
void LVGL_USE_Function(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationTickHook(void);

/* USER CODE BEGIN 3 */
volatile uint32_t g_freertos_fault_type = 0U;
volatile TaskHandle_t g_freertos_fault_task = NULL;
volatile const char *g_freertos_fault_task_name = NULL;

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  g_freertos_fault_type = 1U;
  g_freertos_fault_task = xTask;
  g_freertos_fault_task_name = pcTaskName;
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}

void vApplicationMallocFailedHook(void)
{
  g_freertos_fault_type = 2U;
  g_freertos_fault_task = NULL;
  g_freertos_fault_task_name = "malloc";
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}

void vApplicationTickHook( void )
{
   /* This function will be called by each tick interrupt if
   configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
   added here, but the tick hook is called from an interrupt context, so
   code must not attempt to block, and only the interrupt safe FreeRTOS API
   functions can be used (those that end in FromISR()). */
	lv_tick_inc(1);
}

static void thermal_alert_leds_off(void)
{
  thermal_hot_active = 0U;
  thermal_cold_active = 0U;
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
}

static void thermal_alert_leds_update(float maximum_temperature)
{
  if (maximum_temperature >= HOT_LED_ON_TEMP_C)
  {
    thermal_hot_active = 1U;
  }
  else if (maximum_temperature <= HOT_LED_OFF_TEMP_C)
  {
    thermal_hot_active = 0U;
  }

  if (maximum_temperature <= COLD_LED_ON_TEMP_C)
  {
    thermal_cold_active = 1U;
  }
  else if (maximum_temperature >= COLD_LED_OFF_TEMP_C)
  {
    thermal_cold_active = 0U;
  }

  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin,
                    thermal_hot_active ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin,
                    thermal_cold_active ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void boot_ui_update(uint8_t percent, const char *status,
                           const char *detail)
{
  osSemaphoreAcquire(myBinarySem01Handle, osWaitForever);
  boot_screen_set_progress(percent, status, detail);
  osSemaphoreRelease(myBinarySem01Handle);
}

static void boot_led_self_test(void)
{
  static GPIO_TypeDef *const led_ports[4] = {
    LED1_GPIO_Port, LED2_GPIO_Port, LED3_GPIO_Port, LED4_GPIO_Port
  };
  static const uint16_t led_pins[4] = {
    LED1_Pin, LED2_Pin, LED3_Pin, LED4_Pin
  };
  static const uint8_t progress[4] = {18U, 25U, 32U, 39U};
  char detail[12];
  uint8_t index;

  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, LED2_Pin | LED3_Pin | LED4_Pin, GPIO_PIN_SET);

  for (index = 0U; index < 4U; index++)
  {
    (void)snprintf(detail, sizeof(detail), "LED%u", (unsigned int)(index + 1U));
    boot_ui_update(progress[index], BOOT_TEXT_DISPLAY_LED, detail);
    HAL_GPIO_WritePin(led_ports[index], led_pins[index], GPIO_PIN_RESET);
    osDelay(120U);
    HAL_GPIO_WritePin(led_ports[index], led_pins[index], GPIO_PIN_SET);
  }
}
/* USER CODE END 3 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  systemEventFlagsHandle = osEventFlagsNew(NULL);
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of myBinarySem01 */
  myBinarySem01Handle = osSemaphoreNew(1, 1, &myBinarySem01_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Img_Dis_Task */
  Img_Dis_TaskHandle = osThreadNew(Img_Dis_Fun, NULL, &Img_Dis_Task_attributes);

  /* creation of Init_task */
  Init_taskHandle = osThreadNew(Init_task_Function, NULL, &Init_task_attributes);

  /* creation of LVGL_USE_Task */
  LVGL_USE_TaskHandle = osThreadNew(LVGL_USE_Function, NULL, &LVGL_USE_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_Img_Dis_Fun */
/**
 * @brief  用户MLX90640数据读取图像线性插值及LCD显示热成像函数
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_Img_Dis_Fun */
void Img_Dis_Fun(void *argument)
{
  /* USER CODE BEGIN Img_Dis_Fun */
  MLX90640_RefreshRate applied_refresh_rate = MLX90640_32HZ;
  uint32_t fps_window_start;
  uint32_t fps_frame_count = 0U;
  uint32_t battery_next_poll = 0U;
  app_page_t previous_page = APP_PAGE_THERMAL;

  (void)osEventFlagsWait(systemEventFlagsHandle, SYSTEM_EVENT_MLX_READY,
                         osFlagsWaitAny | osFlagsNoClear, osWaitForever);
  fps_window_start = osKernelGetTickCount();

  /* Infinite loop */
  for (;;)
  {
    app_page_t current_page = g_app_page;

    if (current_page != previous_page)
    {
      fps_frame_count = 0U;
      fps_window_start = osKernelGetTickCount();
      g_thermal_fps_x10 = 0U;
      g_thermal_last_frame_tick = 0U;
      if (current_page == APP_PAGE_BATTERY)
      {
        battery_next_poll = 0U;
        thermal_alert_leds_off();
      }
      previous_page = current_page;
    }

    if (current_page == APP_PAGE_BATTERY)
    {
      uint32_t now = osKernelGetTickCount();
      if ((battery_next_poll == 0U) || ((int32_t)(now - battery_next_poll) >= 0))
      {
        cw2015_data_t battery;
        int battery_status = cw2015_read(&battery);
        if (battery_status == 0)
        {
          g_battery_version = battery.version;
          g_battery_soc_x10 = battery.soc_x10;
          g_battery_voltage_mv = battery.voltage_mv;
          g_battery_connected = 1U;
          g_battery_last_update_tick = now;
          g_battery_error = 0;
        }
        else
        {
          g_battery_connected = 0U;
          g_battery_error = battery_status;
        }
        battery_next_poll = now + 1000U;
      }
      osDelay(50U);
      continue;
    }

    if (g_mlx_available == 0U)
    {
      thermal_alert_leds_off();
      osDelay(100U);
      continue;
    }

    MLX90640_RefreshRate requested_refresh_rate = refresh_rate;
    if (requested_refresh_rate != applied_refresh_rate)
    {
      if (MLX90640_SetRefreshRate(MLX90640_I2C_ADDRESS, requested_refresh_rate) == 0)
      {
        applied_refresh_rate = requested_refresh_rate;
      }
    }

    /* I2C采集和温度计算不占用LCD锁，DMA等待期间LVGL仍可运行。 */
    if (MLX_acquire(mlx90640Frame) >= 0)
    {
      uint32_t now;
      uint32_t elapsed;

      thermal_alert_leds_update(max_temp);
      MLX_render();
      if (g_app_page != APP_PAGE_THERMAL)
      {
        continue;
      }
      now = osKernelGetTickCount();
      g_thermal_last_frame_tick = now;
      fps_frame_count++;
      elapsed = now - fps_window_start;
      if (elapsed >= 1000U)
      {
        g_thermal_fps_x10 = (fps_frame_count * 10000U + elapsed / 2U) / elapsed;
        fps_frame_count = 0U;
        fps_window_start = now;
      }
    }
    else if ((g_thermal_last_frame_tick == 0U) ||
             ((osKernelGetTickCount() - g_thermal_last_frame_tick) > 1500U))
    {
      thermal_alert_leds_off();
    }

    /* 避免低刷新率下持续轮询数据就绪寄存器。 */
    switch (applied_refresh_rate)
    {
    case MLX90640_4HZ:
      osDelay(200);
      break;
    case MLX90640_8HZ:
      osDelay(75);
      break;
    case MLX90640_16HZ:
      osDelay(10);
      break;
    case MLX90640_32HZ:
    case MLX90640_64HZ:
    default:
      osDelay(1);
      break;
    }
  }
  /* USER CODE END Img_Dis_Fun */
}

/* USER CODE BEGIN Header_Init_task_Function */
/**
 * @brief Function implementing the Init_task thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Init_task_Function */
void Init_task_Function(void *argument)
{
  /* USER CODE BEGIN Init_task_Function */
  uint16_t chip_id;
  int mlx_status = 0;
  int battery_status;
  cw2015_data_t battery;
  char boot_detail[40];

  /* 1) LCD 初始化与清屏（持有互斥） */
  osSemaphoreAcquire(myBinarySem01Handle, osWaitForever);
  g_back_color = BLACK;
  /* 2) LVGL初始化 */
  lv_init();
  lv_port_disp_init();
  lcd_dma_sync_init();
  lcd_clear(BLACK);
  lv_port_indev_init();
  setup_ui(&guider_ui);    // 初始化UI
  events_init(&guider_ui); // 初始化事件
  boot_screen_create();

  osSemaphoreRelease(myBinarySem01Handle);
  (void)osEventFlagsSet(systemEventFlagsHandle, SYSTEM_EVENT_UI_READY);

  boot_ui_update(8U, BOOT_TEXT_DISPLAY_LED, "LCD:OK");
  osDelay(80U);
  boot_led_self_test();

  /* 3) MLX90640 初始化，增加有限重试避免长时间阻塞 */
#ifdef MLX90640_use
  boot_ui_update(48U, "MLX90640 " BOOT_TEXT_TESTING, "I2C 0x33");
  MLX90640_I2CInit();
  osDelay(MLX90640_OPEN_time_ms);
  MLX90640_I2CFreqSet(400);

  uint8_t tries;
  if (HAL_I2C_IsDeviceReady(&hi2c1, MLX90640_I2C_ADDRESS << 1,
                            3U, 30U) != HAL_OK)
  {
    mlx_status = -1;
  }
  else
  {
    tries = 5;
    do
    {
      mlx_status = MLX90640_SetRefreshRate(MLX90640_I2C_ADDRESS, MLX90640_32HZ);
      if (mlx_status != 0) osDelay(10);
    } while ((mlx_status != 0) && (--tries > 0U));
  }

  if (mlx_status == 0)
  {
    tries = 5;
    do
    {
      mlx_status = MLX90640_SetChessMode(MLX90640_I2C_ADDRESS);
      if (mlx_status != 0) osDelay(10U);
    } while ((mlx_status != 0) && (--tries > 0U));
  }

  if (mlx_status == 0)
  {
    tries = 5;
    do
    {
      mlx_status = MLX90640_DumpEE(MLX90640_I2C_ADDRESS, eeMLX90640);
      if (mlx_status != 0) osDelay(10U);
    } while ((mlx_status != 0) && (--tries > 0U));
  }

  if (mlx_status == 0)
  {
    mlx_status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  }

  if (mlx_status == 0)
  {
    (void)MLX90640_I2CRead(MLX90640_I2C_ADDRESS,
                           MLX90640_EEPROM_id_ADDRESS, 1, &chip_id);
    g_mlx_available = 1U;
  }
  boot_ui_update(70U,
                 (g_mlx_available != 0U) ?
                   "MLX90640 " BOOT_TEXT_NORMAL :
                   "MLX90640 " BOOT_TEXT_NOT_FOUND,
                 (g_mlx_available != 0U) ? "I2C 0x33 OK" : "I2C 0x33 FAIL");
#endif

  boot_ui_update(78U, "CW2015 " BOOT_TEXT_TESTING, "I2C 0x62");
  battery_status = cw2015_read(&battery);
  if (battery_status == 0)
  {
    g_battery_version = battery.version;
    g_battery_soc_x10 = battery.soc_x10;
    g_battery_voltage_mv = battery.voltage_mv;
    g_battery_connected = 1U;
    g_battery_last_update_tick = osKernelGetTickCount();
    g_battery_error = 0;
    (void)snprintf(boot_detail, sizeof(boot_detail),
                   "I2C 0x62 OK  V:%02X", battery.version);
  }
  else
  {
    g_battery_connected = 0U;
    g_battery_error = battery_status;
    (void)snprintf(boot_detail, sizeof(boot_detail),
                   "I2C 0x62 FAIL  E:%d", battery_status);
  }
  boot_ui_update(92U,
                 (battery_status == 0) ?
                   "CW2015 " BOOT_TEXT_NORMAL :
                   "CW2015 " BOOT_TEXT_NOT_FOUND,
                 boot_detail);
  osDelay(180U);

  (void)snprintf(boot_detail, sizeof(boot_detail), "MLX:%s  CW:%s",
                 (g_mlx_available != 0U) ? "OK" : "FAIL",
                 (g_battery_connected != 0U) ? "OK" : "FAIL");
  boot_ui_update(100U, BOOT_TEXT_COMPLETE, boot_detail);
  osDelay(650U);

  osSemaphoreAcquire(myBinarySem01Handle, osWaitForever);
  g_back_color = WHITE;
  boot_screen_close(guider_ui.screen);
  osSemaphoreRelease(myBinarySem01Handle);

  /* 即使MLX初始化失败，也允许传感器任务进入CW2015电量页面。 */
  (void)osEventFlagsSet(systemEventFlagsHandle, SYSTEM_EVENT_MLX_READY);
  //触摸屏部分初始化
  HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET); // 选中SPI从机
  /* 4) 保持一段时间，确保 UI 可见后 */
  /* 先跑几次 LVGL 任务，确保界面首次渲染*/
//  for (uint8_t i = 0; i < 5; i++)
//  {
//    lv_timer_handler();
//    osDelay(5);
//  }
//  osDelay(10000);
  vTaskDelete(NULL);
  /* USER CODE END Init_task_Function */
}

/* USER CODE BEGIN Header_LVGL_USE_Function */
/**
 * @brief Function implementing the LVGL_USE_Task thread.
 * @param argument: Not used
 * @retval None
 */
//LVGL用任务
/* USER CODE END Header_LVGL_USE_Function */
void LVGL_USE_Function(void *argument)
{
  /* USER CODE BEGIN LVGL_USE_Function */
  (void)osEventFlagsWait(systemEventFlagsHandle, SYSTEM_EVENT_UI_READY,
                         osFlagsWaitAny | osFlagsNoClear, osWaitForever);

  /* Infinite loop */
  for (;;)
  {
    uint32_t next_delay;
    osSemaphoreAcquire(myBinarySem01Handle, osWaitForever);
    next_delay = lv_timer_handler();
    osSemaphoreRelease(myBinarySem01Handle);

    if (next_delay < 1U) next_delay = 1U;
    if (next_delay > 10U) next_delay = 10U;
    osDelay(next_delay);
  }
  /* USER CODE END LVGL_USE_Function */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

