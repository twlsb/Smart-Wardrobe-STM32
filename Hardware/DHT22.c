#include "stm32f10x.h"
#include "DHT22.h"
#include "Delay.h" // 强依赖精确的微秒级延时函数

/*引脚方向动态配置*********************/

/**
  * 函    数：配置DHT22引脚为推挽输出
  * 说    明：单总线协议下，主机需要发送起始信号时调用
  */
void DHT22_Mode_Out(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = DHT22_PIN;
    GPIO_Init(DHT22_PORT, &GPIO_InitStructure);
}

/**
  * 函    数：配置DHT22引脚为上拉输入
  * 说    明：单总线协议下，主机需要释放总线并读取传感器返回电平时调用
  */
void DHT22_Mode_In(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = DHT22_PIN;
    GPIO_Init(DHT22_PORT, &GPIO_InitStructure);
}

/*硬件配置*********************/

void DHT22_Init(void)
{
    RCC_APB2PeriphClockCmd(DHT22_RCC, ENABLE);
    DHT22_Mode_Out();
    GPIO_SetBits(DHT22_PORT, DHT22_PIN); // 默认将总线拉高，处于空闲状态
    Delay_ms(1000); // 上电后需要等待1秒，让传感器越过不稳定状态
}

/*业务层读写函数*********************/

uint8_t DHT22_ReadData(float *Temperature, float *Humidity)
{
    uint8_t i;
    uint8_t data[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    uint16_t timeout;

    /* 1. 主机发送起始信号 */
    DHT22_Mode_Out();
    GPIO_ResetBits(DHT22_PORT, DHT22_PIN);
    Delay_ms(2); // 主机拉低总线必须大于1ms (DHT22手册规定)
    GPIO_SetBits(DHT22_PORT, DHT22_PIN);
    Delay_us(30); // 主机释放总线，延时20-40us等待传感器响应

    /* 2. 切换为输入，检测传感器响应信号 */
    DHT22_Mode_In();

    timeout = 0;
    while (GPIO_ReadInputDataBit(DHT22_PORT, DHT22_PIN) == 1) // 等待传感器拉低总线(80us)
    {
        timeout++;
        Delay_us(1);
        if (timeout > 100) return 1; // 错误1：未检测到传感器响应
    }

    timeout = 0;
    while (GPIO_ReadInputDataBit(DHT22_PORT, DHT22_PIN) == 0) // 等待传感器拉高总线(80us)
    {
        timeout++;
        Delay_us(1);
        if (timeout > 100) return 2; // 错误2：传感器拉低超时
    }

    timeout = 0;
    while (GPIO_ReadInputDataBit(DHT22_PORT, DHT22_PIN) == 1) // 等待总线再次拉低，准备接收数据
    {
        timeout++;
        Delay_us(1);
        if (timeout > 100) return 3; // 错误3：传感器拉高超时
    }

    /* 3. 接收40位数据 (5个字节) */
    for (i = 0; i < 40; i++)
    {
        timeout = 0;
        while (GPIO_ReadInputDataBit(DHT22_PORT, DHT22_PIN) == 0) // 等待每一位的前置低电平(50us)结束
        {
            timeout++;
            Delay_us(1);
            if (timeout > 100) return 4; // 错误4：数据接收低电平超时
        }

        /* 延时40us。因为'0'的高电平持续26-28us，'1'的高电平持续70us
           所以延时40us后如果还是高电平，说明读到的是'1' */
        Delay_us(40); 

        if (GPIO_ReadInputDataBit(DHT22_PORT, DHT22_PIN) == 1)
        {
            data[i / 8] |= (1 << (7 - (i % 8))); // 按位写入数组
            
            timeout = 0;
            while (GPIO_ReadInputDataBit(DHT22_PORT, DHT22_PIN) == 1) // 等待'1'的剩余高电平结束
            {
                timeout++;
                Delay_us(1);
                if (timeout > 100) return 5; // 错误5：数据接收高电平超时
            }
        }
    }

    /* 4. 数据校验与解析 */
    // DHT22校验规则：Byte4 = (Byte0 + Byte1 + Byte2 + Byte3) 的末8位
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    {
        // 湿度解析: 数据*0.1
        uint16_t humi_raw = (data[0] << 8) | data[1];
        *Humidity = (float)humi_raw / 10.0f;

        // 温度解析: 数据*0.1，最高位为1代表负数
        uint16_t temp_raw = (data[2] << 8) | data[3];
        if (temp_raw & 0x8000)
        {
            *Temperature = (float)(temp_raw & 0x7FFF) / -10.0f;
        }
        else
        {
            *Temperature = (float)temp_raw / 10.0f;
        }
        return 0; // 成功
    }
    else
    {
        return 6; // 错误6：数据校验和不匹配
    }
}
