#ifndef __MQ138_H
#define __MQ138_H

#include "stm32f10x.h"

/* 引脚宏定义 */
#define MQ138_ADC_RCC       RCC_APB2Periph_ADC1
#define MQ138_GPIO_RCC      RCC_APB2Periph_GPIOA
#define MQ138_PORT          GPIOA
#define MQ138_PIN           GPIO_Pin_0
#define MQ138_ADC_CH        ADC_Channel_0

/* 硬件参数配置 (可移植性核心) */
#define FILTER_N            5      // 滤波滑动窗口大小 (采样n次取平均)
#define RL_VALUE            1.0f    // 负载电阻RL阻值(单位:kΩ)。需查看传感器底板背面电阻丝印(102=1.0, 472=4.7)

/* 函数声明 */
void MQ138_Init(void);
void MQ138_CalibrateR0(void);       
uint16_t MQ138_GetRawAdc(void);
uint16_t MQ138_GetFilteredAdc(void);
float MQ138_GetVoltage(void);
float MQ138_GetPPM(void);
float MQ138_GetRs(void);            // 暴露实时电阻获取接口

#endif
