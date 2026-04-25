#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "MQ138.h"
#include "DHT22.h"    
#include "AT24C02.h"  
#include "Key.h"
#include "SysLogic.h" 
#include "Control.h" 

// 预热时间定义 (秒)
#define SENSOR_PREHEAT_SEC 180 

int main(void)
{
    OLED_Init();
    MQ138_Init();
    DHT22_Init();
    AT24C02_Init(); 
    Key_Init();
    Control_Init();  
    SysLogic_Init(); 
    
    OLED_Printf(0, 0, OLED_8X16, "系统启动中...   ");
    OLED_Update();
    Delay_ms(1000);
    OLED_Clear();
    
    float temp = 0.0, humi = 0.0, ppm  = 0.0;
    uint16_t sensorTimer = 200; 
    uint16_t uptime_sec = 0;    // 系统运行秒数记录
    uint8_t isPreheating = 1;   // 预热屏蔽锁
    uint8_t keyNum = 0;
    
    SysLogic_ShowUI(temp, humi, ppm);
    
    while (1)
    {
        keyNum = Key_GetNum();
        if (keyNum != 0)
        {
            SysLogic_KeyHandler(keyNum);      
            SysLogic_ShowUI(temp, humi, ppm); 
        }
        
        if (sensorTimer >= 200) 
        {
            sensorTimer = 0;
            
            // 维护时间戳 (200次循环 = 200*10ms = 2秒)
            uptime_sec += 2;
            if (uptime_sec >= SENSOR_PREHEAT_SEC) {
                isPreheating = 0; // 解除预热锁
            }
            
            ppm = MQ138_GetPPM();
            
            /* ==========================================================
             * [调试模式] 获取并显示洁净空气基准电阻 (R0)
             * 操作步骤:
             * 1. 取消下方代码注释。
             * 2. 在【绝对洁净空气】中通电预热 10 分钟。
             * 3. 观察屏幕数值，待其稳定后记录，将其写入 MQ138 驱动的 R0_CleanAir 宏/变量。
             * 4. 重新注释下方代码，恢复业务逻辑。
             * ========================================================== */
//            float current_Rs = MQ138_GetRs(); 
//            OLED_Clear();
//            OLED_Printf(0, 0, OLED_8X16, "R0 Calibration");
//            OLED_Printf(0, 20, OLED_8X16, "Rs: %.2f", current_Rs);
//            OLED_Update();
//            continue; // 强制拦截并跳过后续常规报警与 UI 刷新逻辑
            /* ========================================================== */
            
            __disable_irq(); 
            DHT22_ReadData(&temp, &humi);
            __enable_irq();
            
            // 压入判定链路，屏蔽期强制传入 1
            SysLogic_CheckAlarm(temp, humi, ppm, isPreheating);
            SysLogic_ShowUI(temp, humi, ppm);
        }
        
        Delay_ms(10);
        sensorTimer++;
    }
}
