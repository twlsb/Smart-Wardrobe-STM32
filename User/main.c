#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "MQ138.h"
#include "DHT22.h"    
#include "AT24C02.h"  
#include "Key.h"
#include "SysLogic.h" 
#include "Control.h" // 强制引入控制层，接管继电器与蜂鸣器

int main(void)
{
    // ==========================================
    // 1. 底层硬件全量初始化
    // ==========================================
    OLED_Init();
    MQ138_Init();
    DHT22_Init();
    AT24C02_Init(); 
    Key_Init();
    Control_Init();  // 修正项：挂载继电器与蜂鸣器 GPIO
    
    // ==========================================
    // 2. 业务逻辑与存储器挂载
    // ==========================================
    SysLogic_Init(); // 从 AT24C02 读取历史阈值，防错重置为 35/70/10
    
    // 开机自检动画
    OLED_ShowString(0, 0, "System Booting..", OLED_8X16);
    OLED_Update();
    Delay_ms(1000);
    OLED_Clear();
    
    float temp = 0.0, humi = 0.0, ppm  = 0.0;
    uint16_t sensorTimer = 200; // 初始强制为 200，保证开机立测
    uint8_t keyNum = 0;
    
    // 初始化直接显示一次主界面防黑屏
    SysLogic_ShowUI(temp, humi, ppm);
    
    while (1)
    {
        // ==========================================
        // 3. 高频扫描任务：交互层 (10ms 响应)
        // ==========================================
        keyNum = Key_GetNum();
        if (keyNum != 0)
        {
            SysLogic_KeyHandler(keyNum);      // 切换菜单/修改阈值/保存EEPROM
            SysLogic_ShowUI(temp, humi, ppm); // 立即刷新 UI 防迟滞
        }
        
        // ==========================================
        // 4. 低频扫描任务：采集与执行层 (2秒周期)
        // ==========================================
        if (sensorTimer >= 200) 
        {
            sensorTimer = 0;
            
            // [采集] 甲醛模拟量 (内部自带滑动平均滤波)
            ppm = MQ138_GetPPM();
            
            // [采集] 温湿度单总线 (进入临界区防中断干扰)
            __disable_irq(); 
            if(DHT22_ReadData(&temp, &humi) != 0) 
            {
                // 容错：若读取失败 (错误码1-6)，保留上一周期旧值，防止 UI 闪烁 0.0
            }
            __enable_irq();
            
            // [执行] 判定当前值与 EEPROM 阈值，输出继电器/蜂鸣器电平 (含迟滞回环)
            SysLogic_CheckAlarm(temp, humi, ppm);
            
            // [反馈] 更新实时状态
            SysLogic_ShowUI(temp, humi, ppm);
        }
        
        Delay_ms(10);
        sensorTimer++;
    }
}
