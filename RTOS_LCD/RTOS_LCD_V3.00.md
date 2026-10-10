# RTOS_LCD V3.00

## CubeMX配置

- I2C1：Fast Mode，400 kHz，Duty Cycle 2，PB6/PB7，外部2.2 kΩ上拉到3.3 V。
- I2C1 RX DMA：DMA1 Stream0 / Channel1，Peripheral-to-Memory，Byte，Memory Increment，High priority。
- LCD块传输DMA：DMA2 Stream0，Memory-to-Memory，Half Word，源地址递增、LCD数据地址固定，Very High priority。
- DMA1 Stream0、DMA2 Stream0、I2C1 EV/ER中断优先级均为5，可安全调用FreeRTOS ISR接口。
- 初始化顺序：GPIO -> DMA -> FSMC -> I2C1 -> FreeRTOS。

## 性能改动

- MLX90640长数据读取改为I2C DMA，任务等待期间不占用LCD/LVGL锁。
- 热图从逐像素设置LCD坐标改为8行分块插值和FSMC DMA连续写入。
- LVGL刷新从逐点写屏改为矩形块DMA写屏。
- LVGL任务加入1~10 ms调度延时，避免无延时空转。
- 初始化、LVGL和采集任务使用就绪事件同步；采集不会再抢跑初始化。
- 刷新率按钮只更新请求值，由采集任务异步写入MLX90640，不再在LVGL回调中无限重试。
- 修正中心温度四像素索引，并在帧读取失败时跳过无效帧。

## 验证结果

- Keil ARMCC 5.05最终增量构建成功：0 errors、0 warnings；全量源码编译仍会显示原工程已有的LVGL和中文编码警告。
- 镜像尺寸：Code 274180 B，RO-data 85124 B，RW-data 848 B，ZI-data 110300 B。
- 生成文件：`MDK-ARM/RTOS_LCD/RTOS_LCD.axf` 与 `MDK-ARM/RTOS_LCD/RTOS_LCD.hex`。

## 上板检查

1. 示波器确认SCL约400 kHz，SDA/SCL上升时间小于300 ns。
2. 若I2C出现NACK，先检查模块是否已有上拉；不要让并联上拉低于约1 kΩ。
3. MLX90640设为32/64 Hz时，实际显示帧率仍受400 kHz总线、温度计算和LCD写入速度限制。
4. 若LCD控制器不支持窗口内连续写入，DMA失败时程序会自动退回CPU块写入。
