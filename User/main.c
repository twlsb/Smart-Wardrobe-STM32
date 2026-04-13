#include "stm32f10x.h"
#include "Delay.h"
#include "DHT22.h"
#include "Control.h"

float Temperature, Humidity;

int main(void)
{
    Control_Init();
//	Delay_ms(2000); 
//	Relay1_Set(OFF);
    
    while (1)
    {
		
    }
}
