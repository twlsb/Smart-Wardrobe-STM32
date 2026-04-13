#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "MQ138.h"

int main(void)
{
    OLED_Init();
    MQ138_Init();
    
    OLED_ShowString(0, 0, "MQ-138 Test", OLED_8X16);
    
    while (1)
    {
        uint16_t rawAdc = MQ138_GetRawAdc();
        uint16_t fltAdc = MQ138_GetFilteredAdc();
        float vol = MQ138_GetVoltage();
        float ppm = MQ138_GetPPM();
        
        // 打印原始值与滤波值对比，观察滤波效果
        OLED_Printf(0, 16, OLED_8X16, "Raw:%04d", rawAdc);
        OLED_Printf(64,16, OLED_8X16, "Flt:%04d", fltAdc);
        
        OLED_Printf(0, 32, OLED_8X16, "Vol:%.2f V", vol);
        OLED_Printf(0, 48, OLED_8X16, "PPM:%.1f ", ppm);
        
        OLED_Update();
        Delay_ms(100); 
    }
}
