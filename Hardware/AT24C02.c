// AT24C02.c
#include "stm32f10x.h"
#include "AT24C02.h"
#include "Delay.h"

#define AT24C02_ADDRESS 0xA0

/*底层引脚操作*********************/

void AT24C02_W_SCL(uint8_t BitValue)
{
    GPIO_WriteBit(AT24C02_SCL_PORT, AT24C02_SCL_PIN, (BitAction)BitValue);
    Delay_us(10);
}

void AT24C02_W_SDA(uint8_t BitValue)
{
    GPIO_WriteBit(AT24C02_SDA_PORT, AT24C02_SDA_PIN, (BitAction)BitValue);
    Delay_us(10);
}

uint8_t AT24C02_R_SDA(void)
{
    uint8_t BitValue;
    BitValue = GPIO_ReadInputDataBit(AT24C02_SDA_PORT, AT24C02_SDA_PIN);
    Delay_us(10);
    return BitValue;
}

/*内部I2C协议时序*********************/

void AT24C02_I2C_Start(void)
{
    AT24C02_W_SDA(1);
    AT24C02_W_SCL(1);
    AT24C02_W_SDA(0);
    AT24C02_W_SCL(0);
}

void AT24C02_I2C_Stop(void)
{
    AT24C02_W_SDA(0);
    AT24C02_W_SCL(1);
    AT24C02_W_SDA(1);
}

void AT24C02_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        AT24C02_W_SDA(!!(Byte & (0x80 >> i)));
        AT24C02_W_SCL(1);
        AT24C02_W_SCL(0);
    }
}

uint8_t AT24C02_I2C_ReceiveByte(void)
{
    uint8_t i, Byte = 0x00;
    AT24C02_W_SDA(1); 
    for (i = 0; i < 8; i++)
    {
        AT24C02_W_SCL(1);
        if (AT24C02_R_SDA() == 1) { Byte |= (0x80 >> i); }
        AT24C02_W_SCL(0);
    }
    return Byte;
}

void AT24C02_I2C_SendAck(uint8_t AckBit)
{
    AT24C02_W_SDA(AckBit);
    AT24C02_W_SCL(1);
    AT24C02_W_SCL(0);
}

uint8_t AT24C02_I2C_ReceiveAck(void)
{
    uint8_t AckBit;
    AT24C02_W_SDA(1); 
    AT24C02_W_SCL(1);
    AckBit = AT24C02_R_SDA();
    AT24C02_W_SCL(0);
    return AckBit;
}

/*硬件配置*********************/

void AT24C02_Init(void)
{
    RCC_APB2PeriphClockCmd(AT24C02_I2C_RCC, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = AT24C02_SCL_PIN | AT24C02_SDA_PIN;
    GPIO_Init(AT24C02_SCL_PORT, &GPIO_InitStructure);
    
    AT24C02_W_SCL(1);
    AT24C02_W_SDA(1);
}

/*业务层读写函数*********************/

void AT24C02_WriteByte(uint8_t WordAddress, uint8_t Data)
{
    AT24C02_I2C_Start();
    AT24C02_I2C_SendByte(AT24C02_ADDRESS); 
    AT24C02_I2C_ReceiveAck();
    AT24C02_I2C_SendByte(WordAddress);
    AT24C02_I2C_ReceiveAck();
    AT24C02_I2C_SendByte(Data);
    AT24C02_I2C_ReceiveAck();
    AT24C02_I2C_Stop();
    
    Delay_ms(5); 
}

uint8_t AT24C02_ReadByte(uint8_t WordAddress)
{
    uint8_t Data;
    AT24C02_I2C_Start();
    AT24C02_I2C_SendByte(AT24C02_ADDRESS);
    AT24C02_I2C_ReceiveAck();
    AT24C02_I2C_SendByte(WordAddress);
    AT24C02_I2C_ReceiveAck();
    
    AT24C02_I2C_Start();
    AT24C02_I2C_SendByte(AT24C02_ADDRESS | 0x01); 
    AT24C02_I2C_ReceiveAck();
    Data = AT24C02_I2C_ReceiveByte();
    AT24C02_I2C_SendAck(1); 
    AT24C02_I2C_Stop();
    
    return Data;
}
