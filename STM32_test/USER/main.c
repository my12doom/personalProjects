/******************** (C) COPYRIGHT 2012 ***************** **************************
 * 文件名  ：main.c
 * 描述    ：串口1(USART1)向电脑的超级终端以1s为间隔打印当前ADC1的转换电压值         
 * 实验平台：STM32开发板
 * 库版本  ：ST3.5.0
 *
**********************************************************************************/
#include "stm32f10x.h"
#include "usart1.h"
#include "adc.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_spi.h"
#include "I2C.h"
#include "stm32f10x_it.h"
#include "NRF24L01.h"

// I2C functions


// ADC1转换的电压值通过MDA方式传到SRAM
extern __IO uint16_t ADC_ConvertedValue;

// 局部变量，用于保存转换计算后的电压值			 

float ADC_ConvertedValueLocal;        

// 软件延时
void Delay(__IO uint32_t nCount)
{
  for(; nCount != 0; nCount--);
} 
#define MPU6050SlaveAddress 0xD0
#define MPU6050_REG_WHO_AM_I 0x75

unsigned char I2c_Buf[10];

/*************************************************** 
**函数名:I2C_ReadTmp 
**功能:读取 tmp101的 2个字节温度 
***************************************************/ 

void I2C_ReadTmp(void) 
{
	/*检测总线是否忙 就是看  SCL 或SDA是否为 低  */ 
	//printf("1");
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)); 
   
	/*允许1字节1 应答模式*/ 
	I2C_AcknowledgeConfig(I2C2, ENABLE); 
 
	/* 发送起始位  */ 
     I2C_GenerateSTART(I2C2, ENABLE); 

	//printf("2");
     while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)); /*EV5,主模式*/ 
 
     /*发送器件地址(写)*/ 
     I2C_Send7bitAddress(I2C2, MPU6050SlaveAddress, I2C_Direction_Transmitter); 
	
	//printf("3");
     while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
	
			/*发送Pointer Register*/ 
     I2C_SendData(I2C2, MPU6050_REG_WHO_AM_I);
	//printf("4");
     while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); /*数据已发送*/
   
			/*起始位*/ 
			I2C_GenerateSTART(I2C2, ENABLE); 
	//printf("5");
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)); 
 
			/*发送器件地址(读)*/ 
			I2C_Send7bitAddress(I2C2, MPU6050SlaveAddress, I2C_Direction_Receiver); 
	//printf("6");
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); 

			/* 读Temperature Register*/ 
			//while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)); /* EV7 */
			//I2c_Buf[0]= I2C_ReceiveData(I2C2); 
 
 
    /*● 为了在收到最后一个字节后产生一个NACK脉冲，在读倒数第二个数据字节之后(在倒数第二个RxNE事件之后)必须清除ACK 位。 
			● 为了产生一个停止/重起始条件，软件必须在读倒数第二个数据字节之后(在倒数第二个RxNE 事件之后)设置 STOP/START 位。 
			● 只接收一个字节时，刚好在EV6 之后(EV6_1时，清除ADDR之后)要关闭应答和停止条件的产生位。*/ 
     I2C_AcknowledgeConfig(I2C2, DISABLE); //最后一位后要关闭应答的
     I2C_GenerateSTOP(I2C2, ENABLE);   //发送停止位
 
	//printf("7");
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)); /* EV7 */ 
				I2c_Buf[1]= I2C_ReceiveData(I2C2); 
            
		/*再次允许应答模式*/ 
	//printf("8,");
		I2C_AcknowledgeConfig(I2C2, ENABLE); 
} 


typedef struct
{
	short mag_x;
	short mag_y;
	short mag_z;
	
	short accel_x;
	short accel_y;
	short accel_z;
	
	short temperature1;
	
	short gyro_x;
	short gyro_y;
	short gyro_z;
	
	long temperature2;
	long pressure;
	
} sensor_data;


void swap(void *p, int size)
{
	int i;
	u8 *pp = (u8*)p;
	u8 tmp;
	for(i=0; i<size/2; i++)
	{
		tmp = pp[i];
		pp[i] = pp[size-1-i];
		pp[size-1-i] = tmp;
	}
}
/**
  * @brief  Main program.
  * @param  None
  * @retval : None
  */
#define	BMP085_SlaveAddress 0xee
#define OSS 0

short I2C_Double_Read(u8 slave, u8 reg)
{
	short o;
	I2C_ReadReg(slave, reg, (u8*)&o, 2);
	swap(&o, 2);
	
	return o;
}

int main(void)
{
	int i=0;
	int result;
	float avg;
	u8 data[32] = {0};

	// Basic Initialization
	USART1_Config();
	SPI_NRF_Init();
	printf("NRF_Check() = %d\r\n", NRF_Check());
	SysTick_Config(72000);

	// msdelay test
	while(1)
	{
		GPIO_SetBits(GPIOA,GPIO_Pin_1);
		msdelay(5);
		GPIO_ResetBits(GPIOA,GPIO_Pin_1);
		msdelay(5);
		GPIO_SetBits(GPIOA,GPIO_Pin_1);
		msdelay(5);
		GPIO_ResetBits(GPIOA,GPIO_Pin_1);
		msdelay(5);
		GPIO_SetBits(GPIOA,GPIO_Pin_1);
		msdelay(5);
		GPIO_ResetBits(GPIOA,GPIO_Pin_1);
		msdelay(5);
		GPIO_SetBits(GPIOA,GPIO_Pin_1);
		msdelay(5);
		GPIO_ResetBits(GPIOA,GPIO_Pin_1);
		msdelay(5);
		GPIO_SetBits(GPIOA,GPIO_Pin_1);
		msdelay(5);
		GPIO_ResetBits(GPIOA,GPIO_Pin_1);
		msdelay(5);
	}
	

	printf("I2C init\r\n");
	I2C_init(0x30);
	printf("I2C init OK\r\n");

	do
	{
		short ac1;
		short ac2; 
		short ac3; 
		unsigned short ac4;
		unsigned short ac5;
		unsigned short ac6;
		short b1; 
		short b2;
		short mb;
		short mc;
		short md;		

		printf("start BMP085\r\n");
		
		ac1 = I2C_Double_Read(BMP085_SlaveAddress, 0xAA);
		ac2 = I2C_Double_Read(BMP085_SlaveAddress, 0xAC);
		ac3 = I2C_Double_Read(BMP085_SlaveAddress, 0xAE);
		ac4 = I2C_Double_Read(BMP085_SlaveAddress, 0xB0);
		ac5 = I2C_Double_Read(BMP085_SlaveAddress, 0xB2);
		ac6 = I2C_Double_Read(BMP085_SlaveAddress, 0xB4);
		b1 =  I2C_Double_Read(BMP085_SlaveAddress, 0xB6);
		b2 =  I2C_Double_Read(BMP085_SlaveAddress, 0xB8);
		mb =  I2C_Double_Read(BMP085_SlaveAddress, 0xBA);
		mc =  I2C_Double_Read(BMP085_SlaveAddress, 0xBC);
		md =  I2C_Double_Read(BMP085_SlaveAddress, 0xBE);

		printf("BMP085 initialized, ac1=%d,ac2=%d,ac3=%d,ac4=%d,ac5=%d,ac6=%d,b1=%d,b2=%d,mb=%d,mc=%d,md=%d\r\n", ac1, ac2, ac3, ac4, ac5, ac6, b1, b2, mb, mc, md);
		
		while(1)
		{
			long x1, x2, b5, b6, x3, b3, p;
			unsigned long b4, b7;
			long  temperature;
			long  pressure;
					
			I2C_WriteReg(BMP085_SlaveAddress, 0xF4, 0x2E);
			Delay(0xfffff);			
			temperature = I2C_Double_Read(BMP085_SlaveAddress, 0xF6);
			I2C_WriteReg(BMP085_SlaveAddress, 0xF4, 0x34 + (OSS << 6));
			Delay(0xfffff);
			pressure = I2C_Double_Read(BMP085_SlaveAddress, 0xF6);
			pressure &= 0x0000FFFF;
			
			x1 = ((long)temperature - ac6) * ac5 >> 15;
			x2 = ((long) mc << 11) / (x1 + md);
			b5 = x1 + x2;
			temperature = (b5 + 8) >> 4;
			
			b6 = b5 - 4000;
			x1 = (b2 * (b6 * b6 >> 12)) >> 11;
			x2 = ac2 * b6 >> 11;
			x3 = x1 + x2;
			b3 = (((long)ac1 * 4 + x3) + 2)/4;
			x1 = ac3 * b6 >> 13;
			x2 = (b1 * (b6 * b6 >> 12)) >> 16;
			x3 = ((x1 + x2) + 2) >> 2;
			b4 = (ac4 * (unsigned long) (x3 + 32768)) >> 15;
			b7 = ((unsigned long) pressure - b3) * (50000 >> OSS);
			if( b7 < 0x80000000)
				p = (b7 * 2) / b4 ;
			else  
				p = (b7 / b4) * 2;
			x1 = (p >> 8) * (p >> 8);
			x1 = (x1 * 3038) >> 16;
			x2 = (-7357 * p) >> 16;
			pressure = p + ((x1 + x2 + 3791) >> 4);
			
			
			
			printf("I2C data = %d, %d, systick=%d\r\n", temperature, pressure, GetSysTickCount());
		}
	}while(0);
	
	NRF_RX_Mode();
	while(1)
	{
		//i = NRF_Tx_Dat(data);
		//NRF_TX_Mode();
		//printf("NRF_Tx_Dat(%d) = %d\r\n", i++, NRF_Tx_Dat(data));

		result = NRF_Rx_Dat(data);
		//printf("NRF_Rx_Dat(%d) = %d\r\n", i++, result);
		
		if (result== RX_OK)
		{
				sensor_data *p = (sensor_data*)data;
			
			swap(&p->mag_x, 2);
			swap(&p->mag_y, 2);
			swap(&p->mag_z, 2);

			swap(&p->accel_x, 2);
			swap(&p->accel_y, 2);
			swap(&p->accel_z, 2);

			swap(&p->temperature1, 2);

			swap(&p->gyro_x, 2);
			swap(&p->gyro_y, 2);
			swap(&p->gyro_z, 2);

			swap(&p->pressure, 4);
			swap(&p->temperature2, 4);
			
			printf("HMC5883=%d,%d,%d, ACCEL=%d,%d,%d, TEMP1 = %d, GYRO=%d,%d,%d, TEMP2=%d, pressure=%d\r\n",
			p->mag_x, p->mag_y, p->mag_z,
			p->accel_x, p->accel_y, p->accel_z, p->temperature1, p->gyro_x, p->gyro_y, p->gyro_z, p->temperature2, p->pressure);
			
			//printf("data=");
			//for(i=0; i<32; i++)
			//	printf("%02x,", data[i]);
			//printf("\r\n");
		}
	}
	
	/* enable adc1 and config adc1 to dma mode */
	ADC1_Init();
	
	printf("\r\n -------这是一个ADC实验------\r\n");
	
	while (1)
	{
		avg = 0;
		for(i=0;i<200;i++)
		{
			ADC_ConvertedValueLocal =(float) ADC_ConvertedValue/4095*3.374*2.957; // 读取转换的AD值
			avg += ADC_ConvertedValueLocal;
			Delay(0xff);                              // 延时 
		}
		
		avg /= 200.0f;
	
		printf("The current AD value = %f V, The current AD value = 0x%04X \r\n",avg, ADC_ConvertedValue);
		
		
		// I2C test
		I2C_ReadTmp();
		printf("I2C result=%02X", I2c_Buf[1]);
	}
}
/******************* (C) COPYRIGHT 2012 ***************** *****END OF FILE************/
