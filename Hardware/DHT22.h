#ifndef __DHT22_H
#define __DHT22_H

#include "stm32f10x.h"

/*引脚宏定义*********************/
#define DHT22_RCC       RCC_APB2Periph_GPIOA
#define DHT22_PORT      GPIOA
#define DHT22_PIN       GPIO_Pin_8

/*函数声明*********************/

/*初始化函数*/
void DHT22_Init(void);

/*功能函数*/
/*
 * 返回值: 0-成功, 1~5-时序超时错误, 6-校验和错误
 * 参  数: 传入温湿度的浮点型指针，用于带回读取到的真实数据
 */
uint8_t DHT22_ReadData(float *Temperature, float *Humidity);

#endif
