#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "DHT22.h"
#include <stdio.h>

float Temp, Humi;
uint8_t Status;

int main(void)
{
    OLED_Init();
    DHT22_Init();
    
    OLED_ShowString(0, 0, "DHT22 Test", OLED_8X16);
    
    while (1)
    {
        Status = DHT22_ReadData(&Temp, &Humi);
        
        if (Status == 0)
        {
            /* %05.1f 格式化：占据5个字符宽度，保留1位小数 */
            OLED_Printf(0, 16, OLED_8X16, "T:%05.1f C", Temp);
            OLED_Printf(0, 32, OLED_8X16, "H:%05.1f %%", Humi);
            OLED_ShowString(0, 48, "Status: OK ", OLED_8X16);
        }
        else
        {
            OLED_Printf(0, 48, OLED_8X16, "Status: E%d ", Status); // 打印错误码排错
        }
        
        OLED_Update();
        Delay_ms(2000); // 核心约束：DHT22采样周期不得低于2秒，否则会锁死或返回乱码
    }
}
