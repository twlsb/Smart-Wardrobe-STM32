#include "SysLogic.h"
#include "OLED.h"
#include "AT24C02.h"
#include "Control.h"

// 全局状态变量
Threshold_TypeDef SysThreshold;
uint8_t UI_State = 0;           // 0:主页, 1-3:设置, 4:手动控制
uint8_t Global_WorkMode = 0;    // 0:自动(AUTO), 1:手动(MANUAL)
uint8_t Global_AlarmState = 0;  // 0:正常, 1:警报中, 2:预热屏蔽中

// 手动模式下的继电器虚拟状态
static uint8_t manual_r1 = 0;
static uint8_t manual_r2 = 0;
static uint8_t manual_r3 = 0;

// 影子变量：实时记录自动模式下底层执行器的真实输出状态
static uint8_t current_r1 = 0;
static uint8_t current_r2 = 0;
static uint8_t current_r3 = 0;

// EEPROM 存储地址映射
#define EEPROM_ADDR_TEMP 0x00
#define EEPROM_ADDR_HUMI 0x01
#define EEPROM_ADDR_PPM  0x02

void SysLogic_Init(void)
{
    // 从 EEPROM 读取掉电前保存的阈值
    SysThreshold.TempMax = AT24C02_ReadByte(EEPROM_ADDR_TEMP);
    SysThreshold.HumiMax = AT24C02_ReadByte(EEPROM_ADDR_HUMI);
    SysThreshold.PpmMax  = AT24C02_ReadByte(EEPROM_ADDR_PPM);
    
    // 首次烧录防溢出处理
    if(SysThreshold.TempMax == 0xFF) SysThreshold.TempMax = 35;
    if(SysThreshold.HumiMax == 0xFF) SysThreshold.HumiMax = 70;
    if(SysThreshold.PpmMax  == 0xFF) SysThreshold.PpmMax  = 6; // 实际阈值 0.06 PPM
}

void SysLogic_KeyHandler(uint8_t keyNum)
{
    if (keyNum == 0) return;

    // --- 状态 4: 手动控制逻辑 (完全独立) ---
    if (UI_State == 4) {
        if (keyNum == 1) { 
            // 退出手动，切回自动模式
            UI_State = 0; 
            Global_WorkMode = 0;
            OLED_Clear();
        }
        else if (keyNum == 2) { manual_r1 = !manual_r1; Relay1_Set(manual_r1); }
        else if (keyNum == 3) { manual_r2 = !manual_r2; Relay2_Set(manual_r2); }
        else if (keyNum == 4) { manual_r3 = !manual_r3; Relay3_Set(manual_r3); }
        
        return; // 手动模式下直接拦截并返回
    }

    // --- 状态 0~3: 自动逻辑与设置菜单 ---
    if (keyNum == 1) {
        UI_State++;
        if (UI_State > 3) UI_State = 0; // K1 仅在 0(主页)->1->2->3->0 间循环
        OLED_Clear();
    }
    else if (keyNum == 2) {
        // 阈值增加 (附带上限钳位)
        if(UI_State == 1 && SysThreshold.TempMax < 99) SysThreshold.TempMax++;
        if(UI_State == 2 && SysThreshold.HumiMax < 99) SysThreshold.HumiMax++;
        if(UI_State == 3 && SysThreshold.PpmMax < 99)  SysThreshold.PpmMax++;
    }
    else if (keyNum == 3) {
        // 阈值减少 (附带下限钳位)
        if(UI_State == 1 && SysThreshold.TempMax > 1) SysThreshold.TempMax--;
        if(UI_State == 2 && SysThreshold.HumiMax > 1) SysThreshold.HumiMax--;
        if(UI_State == 3 && SysThreshold.PpmMax > 0)  SysThreshold.PpmMax--;
    }
    else if (keyNum == 4) {
        if (UI_State == 0) { 
            // 【状态继承】在主页按 K4 切入手动模式
            UI_State = 4;
            Global_WorkMode = 1;
            
            // 拷贝影子变量：确保底层高低电平触发硬件一致，实现“无扰切换”
            manual_r1 = current_r1; 
            manual_r2 = current_r2;
            manual_r3 = current_r3;
            
            // 为安全起见，切入手动模式时关闭蜂鸣器报警声
            Buzzer_Set(OFF);
        } 
        else if (UI_State >= 1 && UI_State <= 3) { 
            // 在设置界面按 K4 触发保存并返回主页
            AT24C02_WriteByte(EEPROM_ADDR_TEMP, SysThreshold.TempMax);
            AT24C02_WriteByte(EEPROM_ADDR_HUMI, SysThreshold.HumiMax);
            AT24C02_WriteByte(EEPROM_ADDR_PPM,  SysThreshold.PpmMax);
            UI_State = 0;
        }
        OLED_Clear();
    }
}

void SysLogic_CheckAlarm(float currentTemp, float currentHumi, float currentPpm, uint8_t isPreheating)
{
    // 若系统处于手动控制模式，截断传感器对底层继电器的接管权
    if (Global_WorkMode == 1) return; 

    uint8_t isAlarm = 0;
    uint8_t needFan = 0; 
    
    // 迟滞回环死区状态存储
    static uint8_t state_humi_alarm = 0;
    static uint8_t state_temp_alarm = 0;
    static uint8_t state_ppm_alarm  = 0;

    // ---------------------------------------------------------
    // 1. 湿度逻辑 -> 控制 Relay1 (除湿)
    // ---------------------------------------------------------
    if (currentHumi > SysThreshold.HumiMax) state_humi_alarm = 1; 
    else if (currentHumi <= (SysThreshold.HumiMax - 2.0f)) state_humi_alarm = 0; 
    
    if (state_humi_alarm) { 
        Relay1_Set(ON); current_r1 = 1; isAlarm = 1; 
    } else { 
        Relay1_Set(OFF); current_r1 = 0; 
    }

    // ---------------------------------------------------------
    // 2. 温度逻辑 -> 统筹 Relay3 (风扇)
    // ---------------------------------------------------------
    if (currentTemp > SysThreshold.TempMax) state_temp_alarm = 1; 
    else if (currentTemp <= (SysThreshold.TempMax - 1.0f)) state_temp_alarm = 0; 

    if (state_temp_alarm) { needFan = 1; isAlarm = 1; }

    // ---------------------------------------------------------
    // 3. 甲醛逻辑 -> 控制 Relay2 (杀菌) & 统筹 Relay3 (风扇)
    // ---------------------------------------------------------
    if (isPreheating) {
        state_ppm_alarm = 0; // 预热期间屏蔽甲醛报警
    } else {
        float ppm_max_f = SysThreshold.PpmMax / 100.0f;
        float ppm_hyst_f = (SysThreshold.PpmMax >= 1) ? ((SysThreshold.PpmMax - 1.0f) / 100.0f) : 0.0f;

        if (currentPpm >= ppm_max_f) state_ppm_alarm = 1; 
        else if (currentPpm <= ppm_hyst_f) state_ppm_alarm = 0; 
    }

    if (state_ppm_alarm) { 
        Relay2_Set(ON); current_r2 = 1; needFan = 1; isAlarm = 1; 
    } else { 
        Relay2_Set(OFF); current_r2 = 0; 
    }

    // ---------------------------------------------------------
    // 4. 统筹执行与状态更新
    // ---------------------------------------------------------
    if (needFan) { Relay3_Set(ON); current_r3 = 1; } 
    else { Relay3_Set(OFF); current_r3 = 0; }
    
    if (isAlarm) Buzzer_Set(ON); 
    else Buzzer_Set(OFF);
    
    // 刷新全局状态机供 UI 层读取
    if (isAlarm) Global_AlarmState = 1; 
    else if (isPreheating) Global_AlarmState = 2; 
    else Global_AlarmState = 0; 
}

void SysLogic_ShowUI(float currentTemp, float currentHumi, float currentPpm)
{
    if (UI_State == 0) 
    {
        OLED_Printf(0,  0, OLED_8X16, "温度: %.1f C    ", currentTemp);
        OLED_Printf(0, 16, OLED_8X16, "湿度: %.1f %%   ", currentHumi);
        OLED_Printf(0, 32, OLED_8X16, "甲醛: %.3f PPM  ", currentPpm);
        
        // 宽度核算: 3汉字(48px) + 4空格(32px) + [自动](48px) = 128px (完美满行)
        if (Global_AlarmState == 2) {
            OLED_Printf(0, 48, OLED_8X16, "预热中    [自动]"); 
        } else if (Global_AlarmState == 1) {
            OLED_Printf(0, 48, OLED_8X16, "警报!     [自动]"); 
        } else {
            OLED_Printf(0, 48, OLED_8X16, "正常      [自动]"); 
        }
    }
    else if (UI_State == 1) 
    {
        OLED_Printf(0, 0,  OLED_8X16, "== 设置温度 ==  ");
        OLED_Printf(0, 16, OLED_8X16, "阈值: %02d C    ", SysThreshold.TempMax);
    }
    else if (UI_State == 2) 
    {
        OLED_Printf(0, 0,  OLED_8X16, "== 设置湿度 ==  ");
        OLED_Printf(0, 16, OLED_8X16, "阈值: %02d %%    ", SysThreshold.HumiMax);
    }
    else if (UI_State == 3) 
    {
        OLED_Printf(0, 0,  OLED_8X16, "== 设置甲醛 ==  ");
        OLED_Printf(0, 16, OLED_8X16, "阈值: %.2f PPM  ", SysThreshold.PpmMax / 100.0f);
    }
    else if (UI_State == 4) 
    {
        OLED_Printf(0, 0,  OLED_8X16, "== 手动模式 ==  ");
        // 将 ON/OFF 缩减为 开/关，节省横向空间
        OLED_Printf(0, 16, OLED_8X16, "K2除湿: %s      ", manual_r1 ? "开" : "关");
        OLED_Printf(0, 32, OLED_8X16, "K3杀菌: %s      ", manual_r2 ? "开" : "关");
        OLED_Printf(0, 48, OLED_8X16, "K4风扇: %s      ", manual_r3 ? "开" : "关");
    }
    OLED_Update();
}
