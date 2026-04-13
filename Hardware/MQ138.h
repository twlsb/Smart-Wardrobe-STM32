#ifndef __MQ138_H
#define __MQ138_H

#include "stm32f10x.h"

/* 引脚宏定义 */
#define MQ138_ADC_RCC       RCC_APB2Periph_ADC1
#define MQ138_GPIO_RCC      RCC_APB2Periph_GPIOA
#define MQ138_PORT          GPIOA
#define MQ138_PIN           GPIO_Pin_0
#define MQ138_ADC_CH        ADC_Channel_0

/* 滤波参数配置 */
#define FILTER_N            10  // 滤波滑动窗口大小 (采样10次取平均)

/* 函数声明 */
void MQ138_Init(void);
uint16_t MQ138_GetRawAdc(void);
uint16_t MQ138_GetFilteredAdc(void);
float MQ138_GetVoltage(void);
float MQ138_GetPPM(void);

#endif
