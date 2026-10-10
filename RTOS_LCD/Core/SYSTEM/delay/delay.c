/**
 ****************************************************************************************************
 * @file        delay.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2023-02-25
 * @brief       使用SysTick的普通计数模式对延迟进行管理(支持ucosii)
 *              提供delay_init初始化函数， delay_us和delay_ms等延时函数
 * @license     Copyright (c) 2022-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 探索者 F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20230206
 * 第一次发布
 * V1.1 20230225
 * 修改SYS_SUPPORT_OS部分代码, 默认仅支持UCOSII 2.93.01版本, 其他OS请参考实现
 * 修改delay_init不再使用8分频,全部统一使用MCU时钟
 * 修改delay_us使用时钟摘取法延时, 兼容OS
 * 修改delay_ms直接使用delay_us延时实现.
 *
 ****************************************************************************************************
 */

#include "../SYSTEM/sys/sys.h"
#include "../SYSTEM/delay/delay.h"
#include "main.h"

static uint32_t g_fac_us = 0;       /* us延时倍乘数 */

/* 如果SYS_SUPPORT_OS定义了,说明要支持OS了(不限于UCOS) */
#if SYS_SUPPORT_OS

/* 添加公共头文件 ( ucos需要用到) */
#include "cmsis_os.h"

/* 定义g_fac_ms变量, 表示ms延时的倍乘数, 代表每个节拍的ms数, (仅在使能os的时候,需要用到) */
static uint16_t g_fac_ms = 0;

/*
 *  当delay_us/delay_ms需要支持OS的时候需要三个与OS相关的宏定义和函数来支持
 *  首先是3个宏定义:
 *      delay_osrunning    :用于表示OS当前是否正在运行,以决定是否可以使用相关函数
 *      delay_ostickspersec:用于表示OS设定的时钟节拍,delay_init将根据这个参数来初始化systick
 *      delay_osintnesting :用于表示OS中断嵌套级别,因为中断里面不可以调度,delay_ms使用该参数来决定如何运行
 *  然后是3个函数:
 *      delay_osschedlock  :用于锁定OS任务调度,禁止调度
 *      delay_osschedunlock:用于解锁OS任务调度,重新开启调度
 *      delay_ostimedly    :用于OS延时,可以引起任务调度.
 *
 *  本例程仅作UCOSII的支持,其他OS,请自行参考着移植
 */

/* 使用 CMSIS-RTOS v2 (FreeRTOS) 标记与参数 */
#define delay_osrunning     (osKernelGetState() == osKernelRunning)            /* OS是否在运行 */
#define delay_ostickspersec (osKernelGetTickFreq())                             /* OS时钟节拍频率(Hz) */
/* 中断环境检测：IPSR非0表示当前在异常/中断上下文；用于替代UCOS的嵌套计数 */
#define delay_osintnesting  (__get_IPSR())                                      /* 非0表示在中断 */


/**
 * @brief     us级延时时,关闭任务调度(防止打断us级延迟)
 * @param     无
 * @retval    无
 */
void delay_osschedlock(void)
{
    /* FreeRTOS/CMSIS-RTOS v2: 不建议在应用层锁调度，保持空实现 */
}

/**
 * @brief     us级延时时,恢复任务调度
 * @param     无
 * @retval    无
 */
void delay_osschedunlock(void)
{
    /* FreeRTOS/CMSIS-RTOS v2: 不建议在应用层锁调度，保持空实现 */
}

/**
 * @brief     us级延时时,恢复任务调度
 * @param     ticks: 延时的节拍数
 * @retval    无
 */
void delay_ostimedly(uint32_t ticks)
{
    /* ticks 为 OS tick 数，CMSIS-RTOS v2 的 osDelay 接受毫秒单位。
       这里换算为毫秒：ticks * (1000 / delay_ostickspersec) */
    uint32_t ms = (1000u / delay_ostickspersec) * ticks;
    if (ms == 0 && ticks > 0) ms = 1;               /* 至少延时1ms */
    osDelay(ms);
}

/**
 * @brief     systick中断服务函数,使用OS时用到
 * @param     ticks : 延时的节拍数  
 * @retval    无
 */  
/* 注意：SysTick_Handler 由 HAL/FreeRTOS 端口或 stm32f4xx_it.c 提供。
   为避免与 CMSIS-RTOS2 (cmsis_os2.o) 冲突，这里不再定义 SysTick_Handler。 */
#endif

/**
 * @brief     初始化延迟函数
 * @param     sysclk: 系统时钟频率, 即CPU频率(rcc_c_ck), 168MHz
 * @retval    无
 */  
void delay_init(uint16_t sysclk)
{
#if SYS_SUPPORT_OS                                      /* 如果需要支持OS */
    uint32_t reload;
#endif
    g_fac_us = sysclk;                                  /* 由于在HAL_Init中已对systick做了配置，所以这里无需重新配置 */
#if SYS_SUPPORT_OS                                      /* 如果需要支持OS. */
    reload = sysclk;                                    /* 每秒钟的计数次数 单位为M */
    reload *= 1000000 / delay_ostickspersec;            /* 根据delay_ostickspersec设定溢出时间,reload为24位
                                                         * 寄存器,最大值:16777216,在168M下,约合0.09986s左右
                                                         */
    g_fac_ms = 1000 / delay_ostickspersec;              /* 代表OS可以延时的最少单位 */
    SysTick->CTRL |= 1 << 1;                            /* 开启SYSTICK中断 */
    SysTick->LOAD = reload;                             /* 每1/delay_ostickspersec秒中断一次 */
    SysTick->CTRL |= 1 << 0;                            /* 开启SYSTICK */
#endif 
}

/**
 * @brief     延时nus
 * @note      无论是否使用OS, 都是用时钟摘取法来做us延时
 * @param     nus: 要延时的us数
 * @note      nus取值范围: 0 ~ (2^32 / fac_us) (fac_us一般等于系统主频, 自行套入计算)
 * @retval    无
 */
void delay_us(uint32_t nus)
{
    /* 使用当前 SysTick 递减计数做时间摘取，避免修改 SysTick 配置导致卡死 */
    uint32_t ticks = nus * g_fac_us;      /* 需要的节拍数（1 tick = 1/sysclk 秒） */
    uint32_t told = SysTick->VAL;         /* 进入时刻计数器值 */
    uint32_t tnow = 0;
    uint32_t tcnt = 0;
    uint32_t reload = SysTick->LOAD;      /* 当前重装值（24bit 最大 0xFFFFFF） */

    while (tcnt < ticks)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            /* SysTick 递减计数，发生翻转时补偿 */
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
        }
    }
}

/**
 * @brief     延时nms
 * @param     nms: 要延时的ms数 (0< nms <= (2^32 / fac_us / 1000))(fac_us一般等于系统主频, 自行套入计算)
 * @retval    无
 */
void delay_ms(uint16_t nms)
{
    
#if SYS_SUPPORT_OS  /* 如果需要支持OS, 则根据情况调用os延时以释放CPU */
    if (delay_osrunning && delay_osintnesting == 0)     /* 如果OS已经在跑了,并且不是在中断里面(中断里面不能任务调度) */
    {
        if (nms >= g_fac_ms)                            /* 延时的时间大于OS的最少时间周期 */
        {
            delay_ostimedly(nms / g_fac_ms);            /* OS延时 */
        }

        nms %= g_fac_ms;                                /* OS已经无法提供这么小的延时了,采用普通方式延时 */
    }
#endif
    
}

/**
 * @brief       HAL库内部函数用到的延时
 * @note        HAL库的延时默认用Systick，如果我们没有开Systick的中断会导致调用这个延时后无法退出
 * @param       Delay : 要延时的毫秒数
 * @retval      None
 */
/* 覆盖 HAL 的弱符号实现，确保在 RTOS 环境中可用 */
void HAL_Delay(uint32_t Delay)
{
#if SYS_SUPPORT_OS
    if (delay_osrunning && delay_osintnesting == 0)
    {
        /* 在任务上下文，使用 RTOS 延时释放CPU */
        osDelay(Delay);
        return;
    }
#endif
    /* 在中断或RTOS未运行时，使用忙等实现 */
    delay_us(Delay * 1000u);
}










