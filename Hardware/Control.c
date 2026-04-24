#include "Control.h"

/**
  * 函    数：执行模块初始化
  * 说    明：配置 PB0, PB1, PB5 和 PA1 为推挽输出 
  */
void Control_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    // 初始化 GPIOB (继电器)
    GPIO_InitStructure.GPIO_Pin = RELAY1_PIN | RELAY2_PIN | RELAY3_PIN;
    GPIO_Init(RELAY_PORT, &GPIO_InitStructure);
    
    // 初始化 GPIOA (蜂鸣器)
    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_Init(BUZZER_PORT, &GPIO_InitStructure);
    
    // 默认关闭所有执行器件
    Relay1_Set(OFF);
    Relay2_Set(OFF);
    Relay3_Set(OFF);
    Buzzer_Set(OFF);
}

void Relay1_Set(uint8_t State)
{
    GPIO_WriteBit(RELAY_PORT, RELAY1_PIN, (BitAction)State);
}

void Relay2_Set(uint8_t State)
{
    GPIO_WriteBit(RELAY_PORT, RELAY2_PIN, (BitAction)State);
}

void Relay3_Set(uint8_t State)
{
    GPIO_WriteBit(RELAY_PORT, RELAY3_PIN, (BitAction)State);
}

/**
  * 函    数：蜂鸣器控制
  * 说    明：PA1通过NPN三极管驱动，高电平响 
  */
void Buzzer_Set(uint8_t State)
{
    //  1 为响，0 为静音
    GPIO_WriteBit(BUZZER_PORT, BUZZER_PIN, (BitAction)State); 
}
