#ifndef __SYS_LOGIC_H
#define __SYS_LOGIC_H

#include "stm32f10x.h"

// 暴露给外部的阈值结构体
typedef struct {
    uint8_t TempMax;    // 温度上限 (使用整型，避免EEPROM存取浮点数的复杂转换)
    uint8_t HumiMax;    // 湿度上限
    uint8_t PpmMax;     // 甲醛上限
} Threshold_TypeDef;

extern Threshold_TypeDef SysThreshold;
extern uint8_t UI_State; // 0:主界面, 1:设温度, 2:设湿度, 3:设甲醛

void SysLogic_Init(void);
void SysLogic_KeyHandler(uint8_t keyNum);
void SysLogic_CheckAlarm(float currentTemp, float currentHumi, float currentPpm);
void SysLogic_ShowUI(float currentTemp, float currentHumi, float currentPpm);

#endif
