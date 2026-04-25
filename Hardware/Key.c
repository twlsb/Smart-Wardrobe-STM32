#include "Key.h"
#include "Delay.h"

// 端口初始化：PA4-PA7 设为内部上拉输入
void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// 获取键码 (整合阻塞防抖与非阻塞连发)
uint8_t Key_GetNum(void)
{
    // 静态变量：用于记忆 K2/K3 的持续按下时间
    static uint16_t k2_hold_time = 0;
    static uint16_t k3_hold_time = 0;
    
    // ==========================================
    // K1 (PA7, 切换) 与 K4 (PA4, 返回): 维持单次触发阻塞逻辑
    // ==========================================
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == 0)
    {
        Delay_ms(20); 
        while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == 0); 
        Delay_ms(20); 
        return 1;
    }
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == 0)
    {
        Delay_ms(20);
        while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == 0);
        Delay_ms(20);
        return 4;
    }

    // ==========================================
    // K2 (PA6, 加) 与 K3 (PA5, 减): 非阻塞连发逻辑
    // ==========================================
    
    // K2 逻辑
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0)
    {
        k2_hold_time++;
        if (k2_hold_time == 2) // 刚按下累积满 20ms，执行单次单击触发
        {
            return 2;
        }
        else if (k2_hold_time > 40) // 持续按下超过约 400ms，激活连发模式
        {
            k2_hold_time = 35; // 游标回退，利用 5个主循环节拍 (约50ms) 作为连发间隔
            return 2;
        }
    }
    else
    {
        k2_hold_time = 0; // 松手立刻清零状态
    }

    // K3 逻辑
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == 0)
    {
        k3_hold_time++;
        if (k3_hold_time == 2) 
        {
            return 3;
        }
        else if (k3_hold_time > 40) 
        {
            k3_hold_time = 35; 
            return 3;
        }
    }
    else
    {
        k3_hold_time = 0;
    }
    
    return 0; // 无按键动作或处于连发等待间隙
}
