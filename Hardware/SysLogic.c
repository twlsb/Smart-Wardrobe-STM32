#include "SysLogic.h"
#include "OLED.h"
#include "AT24C02.h"
#include "Control.h"

Threshold_TypeDef SysThreshold;
uint8_t UI_State = 0; 

// EEPROM 存储地址映射
#define EEPROM_ADDR_TEMP 0x00
#define EEPROM_ADDR_HUMI 0x01
#define EEPROM_ADDR_PPM  0x02

// 初始化逻辑：从 EEPROM 读取历史设定值
void SysLogic_Init(void)
{
    SysThreshold.TempMax = AT24C02_ReadByte(EEPROM_ADDR_TEMP);
    SysThreshold.HumiMax = AT24C02_ReadByte(EEPROM_ADDR_HUMI);
    SysThreshold.PpmMax  = AT24C02_ReadByte(EEPROM_ADDR_PPM);
    
    // 初次烧录芯片防错机制（如果EEPROM为空，读出会是0xFF，强制赋默认值）
    if(SysThreshold.TempMax == 0xFF) SysThreshold.TempMax = 35;
    if(SysThreshold.HumiMax == 0xFF) SysThreshold.HumiMax = 70;
    if(SysThreshold.PpmMax  == 0xFF) SysThreshold.PpmMax  = 10; 
}

// 按键事件分发
void SysLogic_KeyHandler(uint8_t keyNum)
{
    if (keyNum == 1) // K1: 切换菜单状态
    {
        UI_State++;
        if (UI_State > 3) UI_State = 0;
        OLED_Clear();
    }
    else if (keyNum == 2) // K2: 数值加 (增加上限钳位 99)
    {
        if(UI_State == 1 && SysThreshold.TempMax < 99) SysThreshold.TempMax++;
        if(UI_State == 2 && SysThreshold.HumiMax < 99) SysThreshold.HumiMax++;
        if(UI_State == 3 && SysThreshold.PpmMax < 99)  SysThreshold.PpmMax++;
    }
    else if (keyNum == 3) // K3: 数值减 (增加下限钳位 1)
    {
        if(UI_State == 1 && SysThreshold.TempMax > 1) SysThreshold.TempMax--;
        if(UI_State == 2 && SysThreshold.HumiMax > 1) SysThreshold.HumiMax--;
        if(UI_State == 3 && SysThreshold.PpmMax > 0)  SysThreshold.PpmMax--;
    }
    else if (keyNum == 4) // K4: 强制返回主界面并触发一次性保存
    {
        if (UI_State != 0) // 只有从设置界面退出时才写入，保护 EEPROM
        {
            AT24C02_WriteByte(EEPROM_ADDR_TEMP, SysThreshold.TempMax);
            AT24C02_WriteByte(EEPROM_ADDR_HUMI, SysThreshold.HumiMax);
            AT24C02_WriteByte(EEPROM_ADDR_PPM,  SysThreshold.PpmMax);
        }
        UI_State = 0;
        OLED_Clear();
    }
}

// UI 渲染逻辑
void SysLogic_ShowUI(float currentTemp, float currentHumi, float currentPpm)
{
    if (UI_State == 0) // 主状态监测界面
    {
        OLED_Printf(0,  0, OLED_8X16, "T:%.1fC  H:%.1f%%", currentTemp, currentHumi);
        OLED_Printf(0, 16, OLED_8X16, "HCHO:%.1f PPM   ", currentPpm);
        OLED_Printf(0, 32, OLED_8X16, "- System Normal -"); 
    }
    else if (UI_State == 1) // 设置温度上限
    {
        OLED_Printf(0, 0,  OLED_8X16, "== Set Temp ==");
        OLED_Printf(0, 16, OLED_8X16, "Limit: %02d C", SysThreshold.TempMax);
    }
    else if (UI_State == 2) // 设置湿度上限
    {
        OLED_Printf(0, 0,  OLED_8X16, "== Set Humi ==");
        OLED_Printf(0, 16, OLED_8X16, "Limit: %02d %%", SysThreshold.HumiMax);
    }
    else if (UI_State == 3) // 设置甲醛上限
    {
        OLED_Printf(0, 0,  OLED_8X16, "== Set HCHO ==");
        OLED_Printf(0, 16, OLED_8X16, "Limit: %02d PPM", SysThreshold.PpmMax);
    }
    OLED_Update();
}

// 报警与继电器联动执行 (集成迟滞回环控制防止继电器高频抽搐)
void SysLogic_CheckAlarm(float currentTemp, float currentHumi, float currentPpm)
{
    uint8_t isAlarm = 0;
    uint8_t needFan = 0; 
    
    // 引入静态变量存储上一周期的报警状态，构建死区 (Hysteresis)
    static uint8_t state_humi_alarm = 0;
    static uint8_t state_temp_alarm = 0;
    static uint8_t state_ppm_alarm  = 0;

    // ---------------------------------------------------------
    // 1. 湿度判断 -> 独立控制除湿机 (Relay1)
    // ---------------------------------------------------------
    if (currentHumi > SysThreshold.HumiMax) { 
        state_humi_alarm = 1; 
    } else if (currentHumi < (SysThreshold.HumiMax - 2.0f)) { 
        // 湿度下降到阈值 2.0% 以下才解除报警
        state_humi_alarm = 0; 
    }
    
    if (state_humi_alarm) {
        Relay1_Set(ON); 
        isAlarm = 1; 
    } else {
        Relay1_Set(OFF);
    }

    // ---------------------------------------------------------
    // 2. 温度判断 -> 触发风扇需求 (Relay3 统筹)
    // ---------------------------------------------------------
    if (currentTemp > SysThreshold.TempMax) { 
        state_temp_alarm = 1; 
    } else if (currentTemp < (SysThreshold.TempMax - 1.0f)) { 
        // 温度下降到阈值 1.0℃ 以下才解除报警
        state_temp_alarm = 0; 
    }

    if (state_temp_alarm) {
        needFan = 1;
        isAlarm = 1; 
    }

    // ---------------------------------------------------------
    // 3. 甲醛判断 -> 独立控制杀菌灯 (Relay2)，并触发风扇排气需求
    // ---------------------------------------------------------
    if (currentPpm > SysThreshold.PpmMax) { 
        state_ppm_alarm = 1; 
    } else if (currentPpm < (SysThreshold.PpmMax - 1.0f)) { 
        // 甲醛下降到阈值 1.0 PPM 以下才解除报警
        state_ppm_alarm = 0; 
    }

    if (state_ppm_alarm) {
        Relay2_Set(ON); 
        needFan = 1;
        isAlarm = 1; 
    } else {
        Relay2_Set(OFF);
    }

    // ---------------------------------------------------------
    // 4. 执行器统筹输出
    // ---------------------------------------------------------
    // 风扇 (Relay3) 统筹：温度过高 或 甲醛超标，均触发排风
    if (needFan) {
        Relay3_Set(ON);
    } else {
        Relay3_Set(OFF);
    }
    
    // 蜂鸣器统筹：任一指标超标即鸣叫
    if (isAlarm) { 
        Buzzer_Set(ON); 
    } else { 
        Buzzer_Set(OFF); 
    }
}
