#ifndef __CONTROL_H
#define __CONTROL_H

#include "stm32f10x.h"

/* 引脚宏定义 */
#define RELAY1_PIN      GPIO_Pin_0    // 除湿
#define RELAY2_PIN      GPIO_Pin_1    // 杀菌
#define RELAY3_PIN      GPIO_Pin_5    // 风扇
#define RELAY_PORT      GPIOB

#define BUZZER_PIN      GPIO_Pin_1    // 报警
#define BUZZER_PORT     GPIOA

/* 状态宏定义 */
#define ON  1
#define OFF 0

/* 函数声明 */
void Control_Init(void);
void Relay1_Set(uint8_t State);
void Relay2_Set(uint8_t State);
void Relay3_Set(uint8_t State);
void Buzzer_Set(uint8_t State);

#endif
