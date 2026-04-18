#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "MQ138.h"
#include "DHT22.h"    
#include "AT24C02.h"  
#include "Key.h"
#include "SysLogic.h" 

int main(void)
{
    // 1. 底层初始化
    OLED_Init();
    MQ138_Init();
    DHT22_Init();
    AT24C02_Init(); 
    Key_Init();
    
    // 2. 业务逻辑初始化 (读取 AT24C02 阈值)
    SysLogic_Init();
    
    OLED_ShowString(0, 0, "Booting...", OLED_8X16);
    OLED_Update();
    Delay_ms(500);
    OLED_Clear();
    
    float temp = 0.0, humi = 0.0, ppm  = 0.0;
    uint16_t sensorTimer = 200; 
    uint8_t keyNum = 0;
    
    // 初始化直接显示一次主界面，避免启动时的短暂黑屏
    SysLogic_ShowUI(temp, humi, ppm);
    
    while (1)
    {
        // 3. 高频扫描：按键触发测试
        keyNum = Key_GetNum();
        if (keyNum != 0)
        {
            SysLogic_KeyHandler(keyNum);      // 切换状态或加减阈值并写入 EEPROM
            SysLogic_ShowUI(temp, humi, ppm); // 立即刷新中文界面
        }
        
        // 4. 低频扫描：传感器数据更新与报警比对
        if (sensorTimer >= 200) 
        {
            sensorTimer = 0;
            
            ppm = MQ138_GetPPM();
            if(DHT22_ReadData(&temp, &humi) != 0) 
            {
                temp = 0.0; humi = 0.0;
            }
            
            SysLogic_CheckAlarm(temp, humi, ppm);
            SysLogic_ShowUI(temp, humi, ppm);
        }
        
        Delay_ms(10);
        sensorTimer++;
    }
}
