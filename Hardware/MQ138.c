#include "MQ138.h"

/**
  * 函    数：MQ138 ADC初始化
  * 说    明：配置PA0为模拟输入，初始化ADC1，分频至12MHz，开启内部校准
  */
void MQ138_Init(void)
{
    RCC_APB2PeriphClockCmd(MQ138_GPIO_RCC | MQ138_ADC_RCC, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 72MHz / 6 = 12MHz (ADC时钟最大不得超过14MHz)
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; // 模拟输入模式
    GPIO_InitStructure.GPIO_Pin = MQ138_PIN;
    GPIO_Init(MQ138_PORT, &GPIO_InitStructure);
    
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;                  // 独立模式
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;              // 数据右对齐
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 软件触发
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;                 // 单次转换
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;                       // 非扫描模式
    ADC_InitStructure.ADC_NbrOfChannel = 1;                             // 通道数1
    ADC_Init(ADC1, &ADC_InitStructure);
    
    ADC_Cmd(ADC1, ENABLE);
    
    /* ADC硬件校准 (必须执行，否则存在固定偏差) */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) == SET);
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) == SET);
}

/**
  * 函    数：获取单次原始ADC值
  * 返 回 值：12位ADC值 (范围：0~4095)
  */
uint16_t MQ138_GetRawAdc(void)
{
    ADC_RegularChannelConfig(ADC1, MQ138_ADC_CH, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE); // 软件触发一次转换
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); // 等待转换结束标志位
    return ADC_GetConversionValue(ADC1);
}

/**
  * 函    数：获取滑动平均滤波后的ADC值
  * 返 回 值：滤波后的ADC平滑值
  * 算法解析：维护一个长度为 FILTER_N 的 FIFO 队列，每次剔除最旧数据，压入最新数据，取算术平均。
  */
uint16_t MQ138_GetFilteredAdc(void)
{
    static uint16_t filter_buf[FILTER_N] = {0};
    static uint8_t filter_ptr = 0;
    static uint8_t is_filled = 0;
    uint32_t sum = 0;
    uint8_t i;

    // 压入新数据
    filter_buf[filter_ptr++] = MQ138_GetRawAdc();
    
    // 队列指针循环
    if (filter_ptr >= FILTER_N) 
    {
        filter_ptr = 0;
        is_filled = 1; // 标记队列已被填满过至少一次
    }

    // 计算当前有效数据均值 (防止开机初期数据被0拉低)
    uint8_t count = is_filled ? FILTER_N : filter_ptr;
    for (i = 0; i < count; i++) 
    {
        sum += filter_buf[i];
    }
    
    return (uint16_t)(sum / count);
}

/**
  * 函    数：将ADC值转换为真实电压
  * 返 回 值：0.00 ~ 3.30 (单位：V)
  */
float MQ138_GetVoltage(void)
{
    uint16_t adValue = MQ138_GetFilteredAdc();
    return ((float)adValue / 4095.0f) * 3.3f;
}

/**
  * 函    数：转换甲醛浓度 (经验公式估算模型)
  * 说    明：MQ系列模块无法直接输出精确PPM。此为根据电压拟合的演示用线性转换函数。
  * 返 回 值：浓度百分比或伪PPM值
  */
float MQ138_GetPPM(void)
{
    float voltage = MQ138_GetVoltage();
    // 基础拟合：假设空气中基准电压为0.5V，满载为3.0V。此参数需根据你手头模块背面的电位器微调。
    float ppm = (voltage - 0.5f) * 40.0f; 
    if(ppm < 0) ppm = 0;
    return ppm;
}
