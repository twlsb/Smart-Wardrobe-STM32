// AT24C02.h
#ifndef __AT24C02_H
#define __AT24C02_H

#include "stm32f10x.h"

/*引脚宏定义*********************/
#define AT24C02_I2C_RCC     RCC_APB2Periph_GPIOB
#define AT24C02_SCL_PORT    GPIOB
#define AT24C02_SCL_PIN     GPIO_Pin_10
#define AT24C02_SDA_PORT    GPIOB
#define AT24C02_SDA_PIN     GPIO_Pin_11

/*函数声明*********************/

/*初始化函数*/
void AT24C02_Init(void);

/*功能函数*/
void AT24C02_WriteByte(uint8_t WordAddress, uint8_t Data);
uint8_t AT24C02_ReadByte(uint8_t WordAddress);

#endif
