#ifndef __SYS_LOGIC_H
#define __SYS_LOGIC_H

#include "stm32f10x.h"

typedef struct {
    uint8_t TempMax;
    uint8_t HumiMax;
    uint8_t PpmMax;
} Threshold_TypeDef;

extern Threshold_TypeDef SysThreshold;
extern uint8_t UI_State;          // 0:主页, 1-3:设置, 4:手动控制
extern uint8_t Global_WorkMode;   // 0:自动(AUTO), 1:手动(MANUAL)
extern uint8_t Global_AlarmState; // 0:正常, 1:警报中, 2:预热屏蔽中

void SysLogic_Init(void);
void SysLogic_KeyHandler(uint8_t keyNum);
void SysLogic_ShowUI(float currentTemp, float currentHumi, float currentPpm);
void SysLogic_CheckAlarm(float currentTemp, float currentHumi, float currentPpm, uint8_t isPreheating);

#endif
