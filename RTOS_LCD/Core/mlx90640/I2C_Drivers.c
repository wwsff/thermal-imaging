#include "./MLX90640_I2C_Driver.h"
#include "i2c.h"
#include "GPIO.h"
#include "cmsis_os.h"
#ifdef __I2C_H__
extern I2C_HandleTypeDef hi2c1;
#else
#define SCL_HIGH HAL_GPIO_WritePin(SWI2C_SCL_GPIO_Port,SWI2C_SCL_Pin,GPIO_PIN_SET)
#define SCL_LOW  HAL_GPIO_WritePin(SWI2C_SCL_GPIO_Port,SWI2C_SCL_Pin,GPIO_PIN_RESET)

#define SDA_HIGH HAL_GPIO_WritePin(SWI2C_SDA_GPIO_Port,SWI2C_SDA_Pin,GPIO_PIN_SET)
#define SDA_LOW  HAL_GPIO_WritePin(SWI2C_SDA_GPIO_Port,SWI2C_SDA_Pin,GPIO_PIN_RESET)

#define SDA_IN do { \
    GPIO_InitTypeDef GPIO_InitStruct = {0}; \
    GPIO_InitStruct.Pin = SWI2C_SDA_Pin; \
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; \
    GPIO_InitStruct.Pull = GPIO_NOPULL; \
    HAL_GPIO_Init(SWI2C_SDA_GPIO_Port, &GPIO_InitStruct); \
} while(0)

#define SDA_OUT do { \
    GPIO_InitTypeDef GPIO_InitStruct = {0}; \
    GPIO_InitStruct.Pin = SWI2C_SDA_Pin; \
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; \
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; \
    HAL_GPIO_Init(SWI2C_SDA_GPIO_Port, &GPIO_InitStruct); \
} while(0)
#define sda      HAL_GPIO_ReadPin(SWI2C_SDA_GPIO_Port,SWI2C_SDA_Pin)
#define scl      HAL_GPIO_ReadPin(SWI2C_SCL_GPIO_Port,SWI2C_SCL_Pin)

int  I2CSendByte(int8_t);
void I2CReadBytes(int, char *);
void I2CStart(void);
void I2CStop(void);
void I2CRepeatedStart(void);
void I2CSendACK(void);
void I2CSendNack(void);
int  I2CReceiveAck(void);
void Wait(int);
#endif // DEBUG



static int freqCnt = 0;
static uint8_t i2cData[1664] = {0};
static osSemaphoreId_t i2cDmaCompleteSem = NULL;
static volatile HAL_StatusTypeDef i2cDmaResult = HAL_ERROR;

#define MLX90640_DMA_MIN_BYTES  8U
#define MLX90640_DMA_TIMEOUT_MS 100U

static void MLX90640_I2CDmaSyncInit(void)
{
    if ((i2cDmaCompleteSem == NULL) && (osKernelGetState() == osKernelRunning))
    {
        static const osSemaphoreAttr_t attr = { .name = "mlxI2cDma" };
        i2cDmaCompleteSem = osSemaphoreNew(1U, 0U, &attr);
    }
}

static void MLX90640_I2CDmaDrain(void)
{
    if (i2cDmaCompleteSem != NULL)
    {
        while (osSemaphoreAcquire(i2cDmaCompleteSem, 0U) == osOK)
        {
        }
    }
}

void MLX90640_I2CInit()
{   
    #ifndef __I2C_H__
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOB_CLK_ENABLE();  
    
  
    /*Configure GPIO pins : PB6 SCL */
    GPIO_Initure.Pin = SWI2C_SCL_Pin;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SWI2C_SCL_GPIO_Port, &GPIO_Initure);
  
    /*Configure GPIO pins : PB7 SDA*/ 
    GPIO_Initure.Pin = SWI2C_SDA_Pin;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_OD;  
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SWI2C_SDA_GPIO_Port, &GPIO_Initure);
  
    HAL_GPIO_WritePin(SWI2C_SDA_GPIO_Port, SWI2C_SDA_Pin, GPIO_PIN_SET); 
	  HAL_GPIO_WritePin(SWI2C_SCL_GPIO_Port, SWI2C_SCL_Pin, GPIO_PIN_SET); 
	
    I2CStop();
    #endif // __I2C_H__
	MX_I2C1_Init();
    MLX90640_I2CDmaSyncInit();
}
    
int MLX90640_I2CGeneralReset(void)
{
  MLX90640_I2CInit();
  return 0;
}
int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress,uint16_t nMemAddressRead, uint16_t *data)
{
    #ifdef __I2C_H__
    HAL_StatusTypeDef status;
    uint16_t *p = data;
    uint8_t *i2cBuf = (uint8_t *)i2cData;
    uint16_t byteCount = nMemAddressRead * 2U;

    if ((data == NULL) || (nMemAddressRead == 0U) || (byteCount > sizeof(i2cData)))
    {
        return -1;
    }

    MLX90640_I2CDmaSyncInit();

    if ((byteCount >= MLX90640_DMA_MIN_BYTES) &&
        (i2cDmaCompleteSem != NULL) &&
        (osKernelGetState() == osKernelRunning))
    {
        MLX90640_I2CDmaDrain();
        i2cDmaResult = HAL_BUSY;

        status = HAL_I2C_Mem_Read_DMA(&hi2c1,
                                      slaveAddr << 1,
                                      startAddress,
                                      I2C_MEMADD_SIZE_16BIT,
                                      i2cBuf,
                                      byteCount);

        if ((status != HAL_OK) ||
            (osSemaphoreAcquire(i2cDmaCompleteSem, MLX90640_DMA_TIMEOUT_MS) != osOK) ||
            (i2cDmaResult != HAL_OK))
        {
            if (hdma_i2c1_rx.State == HAL_DMA_STATE_BUSY)
            {
                (void)HAL_DMA_Abort(&hdma_i2c1_rx);
            }
            (void)HAL_I2C_DeInit(&hi2c1);
            MX_I2C1_Init();
            MLX90640_I2CDmaDrain();
            return -1;
        }
    }
    else
    {
        status = HAL_I2C_Mem_Read(&hi2c1,
                                  slaveAddr << 1,
                                  startAddress,
                                  I2C_MEMADD_SIZE_16BIT,
                                  i2cBuf,
                                  byteCount,
                                  100);

        if (status != HAL_OK)
        {
            return -1;
        }
    }

    for (uint16_t cnt = 0; cnt < nMemAddressRead; cnt++)
    {
        uint16_t idx = cnt * 2U;
        *p++ = ((uint16_t)i2cBuf[idx] << 8) | i2cBuf[idx + 1U];
    }
    return 0;
    #else

    
    uint8_t sa;
    int ack = 0;
    int cnt = 0;
    int i = 0;
    char cmd[2] = {0,0};
    //char i2cData[1664] = {0};
    uint16_t *p;
    
    p = data;
    sa = (slaveAddr << 1);
    cmd[0] = startAddress >> 8;
    cmd[1] = startAddress & 0x00FF;
    
    I2CStop();
    Wait(freqCnt);  
    I2CStart();
    Wait(freqCnt);
    
    ack = I2CSendByte(sa);
    if(ack != 0)
    {
        return -1;
    } 
    
    ack = I2CSendByte(cmd[0]);   
    if(ack != 0)
    {
        return -1;
    }
    
    ack = I2CSendByte(cmd[1]);    
    if(ack != 0)
    {
        return -1;
    }  
    
    I2CRepeatedStart();
       
    sa = sa | 0x01;
    
    ack = I2CSendByte(sa);
    if(ack != 0)
    {
        return -1;
    } 
        
    I2CReadBytes((nMemAddressRead << 1), (char *)i2cData);
              
    I2CStop();   

    for(cnt=0; cnt < nMemAddressRead; cnt++)
    {
        i = cnt << 1;
        *p++ = ((int16_t)i2cData[i]<<8 )|(int16_t)i2cData[i+1];
    } 
    return 0;
    #endif
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        i2cDmaResult = HAL_OK;
        if (i2cDmaCompleteSem != NULL)
        {
            (void)osSemaphoreRelease(i2cDmaCompleteSem);
        }
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        i2cDmaResult = HAL_ERROR;
        if (i2cDmaCompleteSem != NULL)
        {
            (void)osSemaphoreRelease(i2cDmaCompleteSem);
        }
    }
}

void MLX90640_I2CFreqSet(int freq)
{
    freqCnt = freq>>1;
    UNUSED(freqCnt);
}

int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{
    #ifdef __I2C_H__
    HAL_StatusTypeDef status;
    uint8_t txData[2];
    uint16_t dataCheck;

    // 准备数据（高位在前）
    txData[0] = data >> 8;    // 高字节
    txData[1] = data & 0xFF;  // 低字节

    // 执行写操作（包含16位寄存器地址）
    status = HAL_I2C_Mem_Write(&hi2c1,
                              slaveAddr << 1,
                              writeAddress,
                              I2C_MEMADD_SIZE_16BIT,
                              txData,
                              2,      // 写入2字节数据
                              100);
    
    if (status != HAL_OK) {
        return -1;  // 写操作失败
    }

    // 回读校验（严格保持原逻辑）
    if (MLX90640_I2CRead(slaveAddr, writeAddress, 1, &dataCheck) != 0) {
        return -1;  // 读操作失败
    }
    
    if (dataCheck != data) {
        return -2;  // 校验失败
    }
    
    return 0;
    #else
    uint8_t sa;
    int ack = 0;
    char cmd[4] = {0,0,0,0};
    static uint16_t dataCheck;

    sa = (slaveAddr << 1);
    cmd[0] = writeAddress >> 8;
    cmd[1] = writeAddress & 0x00FF;
    cmd[2] = data >> 8;
    cmd[3] = data & 0x00FF;

    I2CStop();
    Wait(freqCnt);
    I2CStart();
    ack = I2CSendByte(sa);
    if (ack != 0x00)
    {
        return 1; 
    }  
    
    for(int i = 0; i<4; i++)
    {
        ack = I2CSendByte(cmd[i]);
    
        if (ack != 0x00)
        {
            return -1;
        }  
    }           
    I2CStop();   
    
    MLX90640_I2CRead(slaveAddr,writeAddress,1, &dataCheck);
    
    if ( dataCheck != data)
    {
        return -2;
    }    
    
    return 0;
    #endif // DEBUG
    
}
#ifndef __I2C_H__


int I2CSendByte(int8_t data)
{
   int ack = 1;
   int8_t byte = data; 
   SDA_OUT;
   for(int i=0;i<8;i++)
   {
       Wait(freqCnt);
       
       if(byte & 0x80)
       {
           SDA_HIGH;
       }
       else
       {
           SDA_LOW;
       }
       Wait(freqCnt);
       SCL_HIGH;
       Wait(freqCnt);
       Wait(freqCnt);
       SCL_LOW;
       byte = byte<<1;        
   }    
   
   Wait(freqCnt);
   ack = I2CReceiveAck();
   
   return ack; 
}

void I2CReadBytes(int nBytes, char *dataP)
{
    char data;
    for(int j=0;j<nBytes;j++)
    {
        Wait(freqCnt);
        SDA_IN;    
        
        data = 0;
        for(int i=0;i<8;i++){
            Wait(freqCnt);
            SCL_HIGH;
            Wait(freqCnt);
            data = data<<1;
            if(sda == 1){
                data = data+1;  
            }
            Wait(freqCnt);
            SCL_LOW;
            Wait(freqCnt);
        }  
        
        if(j == (nBytes-1))
        {
            I2CSendNack();
        }
        else
        {                  
            I2CSendACK();
        }
            
        *(dataP+j) = data; 
    }    
    
}
        
void Wait(int freqCnt)
{
    int cnt;
//    for(int i = 0;i<freqCnt;i++)
//    {
//        cnt = cnt++; 
//    }    
	  while(freqCnt--)
    for(cnt=7;cnt>0;cnt--); 
} 

void I2CStart(void)
{
		SDA_OUT;
    SDA_HIGH;
    SCL_HIGH;
    Wait(freqCnt);
    Wait(freqCnt);
    SDA_LOW;
    Wait(freqCnt);
    SCL_LOW;
    Wait(freqCnt);    
    
}

void I2CStop(void)
{
		SDA_OUT;
    SCL_LOW;
    SDA_LOW;
    Wait(freqCnt);
    SCL_HIGH;
    Wait(freqCnt);
    SDA_HIGH;
    Wait(freqCnt);
} 
 
void I2CRepeatedStart(void)
{
		SDA_OUT;
    SCL_LOW;
    Wait(freqCnt);
    SDA_HIGH;
    Wait(freqCnt);
    SCL_HIGH;
    Wait(freqCnt);
    SDA_LOW;
    Wait(freqCnt);
    SCL_LOW;
           
}

void I2CSendACK(void)
{
		SDA_OUT;
    SDA_LOW;
    Wait(freqCnt);
    SCL_HIGH;
    Wait(freqCnt);
    Wait(freqCnt);
    SCL_LOW;
    Wait(freqCnt);
    SDA_HIGH;
    
}

void I2CSendNack(void)
{
		SDA_OUT;
    SDA_HIGH;
    Wait(freqCnt);
    SCL_HIGH;
    Wait(freqCnt);
    Wait(freqCnt);
    SCL_LOW;
    Wait(freqCnt);
    SDA_HIGH;
    
}

int I2CReceiveAck(void)
{
    int ack;
    
    SDA_IN;
    Wait(freqCnt);
    SCL_HIGH;
    Wait(freqCnt);
    if(sda == 0)
    {
        ack = 0;
    }
    else
    {
        ack = 1;
    }
    Wait(freqCnt);        
    SCL_LOW;
    SDA_LOW;
    
    return ack;    
}
#endif // !__I2C_H__
