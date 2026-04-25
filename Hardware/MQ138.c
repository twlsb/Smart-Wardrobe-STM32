#include "MQ138.h"
#include "Delay.h"
#include <math.h>

static float R0_CleanAir = 19.5f;   // 洁净空气基准电阻全局变量

/**
  * 函    数：MQ138 ADC初始化
  * 说    明：配置PA0为模拟输入，初始化ADC1，分频至12MHz，开启内部校准
  */
void MQ138_Init(void)
{
    RCC_APB2PeriphClockCmd(MQ138_GPIO_RCC | MQ138_ADC_RCC, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); 
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; 
    GPIO_InitStructure.GPIO_Pin = MQ138_PIN;
    GPIO_Init(MQ138_PORT, &GPIO_InitStructure);
    
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;                  
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;              
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; 
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;                 
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;                       
    ADC_InitStructure.ADC_NbrOfChannel = 1;                             
    ADC_Init(ADC1, &ADC_InitStructure);
    
    ADC_Cmd(ADC1, ENABLE);
    
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) == SET);
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) == SET);
}

/**
  * 函    数：获取单次原始ADC值
  */
uint16_t MQ138_GetRawAdc(void)
{
    ADC_RegularChannelConfig(ADC1, MQ138_ADC_CH, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE); 
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); 
    return ADC_GetConversionValue(ADC1);
}

/**
  * 函    数：获取滑动平均滤波后的ADC值
  */
uint16_t MQ138_GetFilteredAdc(void)
{
    static uint16_t filter_buf[FILTER_N] = {0};
    static uint8_t filter_ptr = 0;
    static uint8_t is_filled = 0;
    uint32_t sum = 0;
    uint8_t i;

    filter_buf[filter_ptr++] = MQ138_GetRawAdc();
    
    if (filter_ptr >= FILTER_N) 
    {
        filter_ptr = 0;
        is_filled = 1; 
    }

    uint8_t count = is_filled ? FILTER_N : filter_ptr;
    for (i = 0; i < count; i++) 
    {
        sum += filter_buf[i];
    }
    
    return (uint16_t)(sum / count);
}

/**
  * 函    数：将ADC值转换为真实电压
  */
float MQ138_GetVoltage(void)
{
    uint16_t adValue = MQ138_GetFilteredAdc();
    return ((float)adValue / 4095.0f) * 3.3f;
}

/**
  * [新增] 函 数：获取当前传感器实时电阻 (Rs)
  * 返 回 值：实时电阻值 (单位：kΩ)
  */
float MQ138_GetRs(void)
{
    float voltage = MQ138_GetVoltage();
    if(voltage <= 0.01f) voltage = 0.01f; // 防除零异常
    return ((3.3f - voltage) / voltage * RL_VALUE);
}

/**
  * 函    数：开机提取洁净空气基准电阻 (R0)
  */
void MQ138_CalibrateR0(void)
{
    float voltage = 0;
    float sum_Rs = 0;
    uint8_t i;
    
    for(i = 0; i < 50; i++)
    {
        voltage = MQ138_GetVoltage();
        if(voltage <= 0.01f) voltage = 0.01f; 
        sum_Rs += ((3.3f - voltage) / voltage * RL_VALUE);
        Delay_ms(20);
    }
    R0_CleanAir = sum_Rs / 50.0f;
}

/**
  * 函    数：获取高精度甲醛 PPM 浓度
  */
float MQ138_GetPPM(void)
{
    float voltage = MQ138_GetVoltage();
    if(voltage <= 0.01f) voltage = 0.01f; 
    
    float Rs = ((3.3f - voltage) / voltage * RL_VALUE);
    float ratio = Rs / R0_CleanAir;
    
    float ppm = 1.25f * pow(ratio, -0.5f) - 1.25f; 
    
    if(ppm < 0.0f) ppm = 0.0f;
    return ppm;
}
