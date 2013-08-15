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
#include "math.h"
#include "stm32f10x_usart.h"

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

// Gyro and accelerator registers
#define	SMPLRT_DIV		0x19	//??????,???:0x07(125Hz)
#define	MPU6050_CONFIG			0x1A	//??????,???:0x06(5Hz)
#define	GYRO_CONFIG		0x1B	//??????????,???:0x18(???,2000deg/s)
#define	ACCEL_CONFIG	0x1C	//?????????????????,???:0x01(???,2G,5Hz)
#define	ACCEL_XOUT_H	0x3B
#define	ACCEL_XOUT_L	0x3C
#define	ACCEL_YOUT_H	0x3D
#define	ACCEL_YOUT_L	0x3E
#define	ACCEL_ZOUT_H	0x3F
#define	ACCEL_ZOUT_L	0x40
#define	TEMP_OUT_H		0x41
#define	TEMP_OUT_L		0x42
#define	GYRO_XOUT_H		0x43
#define	GYRO_XOUT_L		0x44	
#define	GYRO_YOUT_H		0x45
#define	GYRO_YOUT_L		0x46
#define	GYRO_ZOUT_H		0x47
#define	GYRO_ZOUT_L		0x48
#define	PWR_MGMT_1		0x6B	//????,???:0x00(????)
#define	WHO_AM_I		0x75	//IIC?????(????0x68,??)

typedef struct
{
	short mag_x;
	short mag_z;
	short mag_y;
	
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
#define	BMP085_SlaveAddress 0xee
#define OSS 0
#define	HMC5883SlaveAddress 0x3C

short I2C_Double_Read(u8 slave, u8 reg)
{
	short o;
	I2C_ReadReg(slave, reg, (u8*)&o, 2);
	swap(&o, 2);
	
	return o;
}

int abs(int i)
{
	if (i>0)
		return i;
	return -i;
}


#define PI 3.14159265

int main(void)
{
	int i=0;
	int result;
	float avg;
	int total_tx = 0;
	u8 data[32] = {0};
	GPIO_InitTypeDef GPIO_InitStructure;

	// Basic Initialization
	USART1_Config();
	SPI_NRF_Init();
	printf("NRF_Check() = %d\r\n", NRF_Check());
	SysTick_Config(720);

	/*
	NRF_TX_Mode();
	while(1)
	{
		u8 result = NRF_Tx_Dat(data);
		
		if (result == TX_OK)
		{
			printf("\rTX_OK count : %d", total_tx ++);			
		}
	}
	*/
	
	printf("I2C init\r\n");
	I2C_init(0x30);
	printf("I2C init OK\r\n");
	
	do
	{
		u8 who_am_i = 0;
		float pitch = 0;
		float pitchI = 0;
		float roll = 0;
		float yaw = 0;
		float yawI = 0;
		float bx = 0;
		float by = 0;
		float bz = 0;
		float accel_max = 0;
		sensor_data *p = (sensor_data*)data;
		printf("start MPU6050\r\n");
		I2C_WriteReg(MPU6050SlaveAddress, PWR_MGMT_1, 0x00);
		I2C_WriteReg(MPU6050SlaveAddress, SMPLRT_DIV, 0x07);
		I2C_WriteReg(MPU6050SlaveAddress, MPU6050_CONFIG, 0x06);
		I2C_WriteReg(MPU6050SlaveAddress, GYRO_CONFIG, 0x18);
		I2C_WriteReg(MPU6050SlaveAddress, ACCEL_CONFIG, 0x08);
		
		I2C_ReadReg(MPU6050SlaveAddress, WHO_AM_I, &who_am_i, 1);
		printf("MPU6050 initialized, WHO_AM_I=%x\r\n", who_am_i);
		
		I2C_WriteReg(HMC5883SlaveAddress, 0x02, 0x00);

		
		for(i=0; i<1000; i++)
		{
			I2C_ReadReg(MPU6050SlaveAddress, ACCEL_XOUT_H, (u8*)&p->accel_x, 14);
			I2C_ReadReg(HMC5883SlaveAddress, 0x03, (u8*)&p->mag_x, 6);
			
			swap(&p->mag_x, 2);
			swap(&p->mag_y, 2);
			swap(&p->mag_z, 2);
			swap(&p->gyro_x, 2);
			swap(&p->gyro_y, 2);
			swap(&p->gyro_z, 2);
			swap(&p->accel_x, 2);
			swap(&p->accel_y, 2);
			swap(&p->accel_z, 2);
			
			bx+= p->gyro_x;
			by+= p->gyro_y;
			bz+= p->gyro_z;
			accel_max += (p->accel_x*p->accel_x+ p->accel_y*p->accel_y + p->accel_z * p->accel_z);
			msdelay(100);
		}
		
		bx /= 1000;
		by /= 1000;
		bz /= 1000;
		accel_max /= 1000;
		accel_max = sqrt(accel_max);
		
		printf("MPU6050 base value measured, b xyz=%f,%f,%f, accel_max=%f \r\n", bx, by, bz, accel_max);
				
		i=0;
		while(1)
		{
			float pitch_accel;
			float pitch_mag;
			static const float factor = 0.995;
			static const float factor_1 = 1-factor;
			int start_tick = GetSysTickCount();
			int centerx = -1;
			int centery = -142;
			int centerz = -42;
			float yaw_mag;
			
			GPIO_ResetBits(GPIOA, GPIO_Pin_4);
			
			I2C_ReadReg(MPU6050SlaveAddress, ACCEL_XOUT_H, (u8*)&p->accel_x, 14);
			I2C_ReadReg(HMC5883SlaveAddress, 0x03, (u8*)&p->mag_x, 6);
			
			swap(&p->mag_x, 2);
			swap(&p->mag_y, 2);
			swap(&p->mag_z, 2);
			swap(&p->gyro_x, 2);
			swap(&p->gyro_y, 2);
			swap(&p->gyro_z, 2);
			swap(&p->accel_x, 2);
			swap(&p->accel_y, 2);
			swap(&p->accel_z, 2);
			
			pitch +=  p->gyro_x - bx ;
			pitchI += p->gyro_x - bx ;
			roll += p->gyro_y - by;
			
			pitch_accel = -atan(-(float)p->accel_y / p->accel_z) * 180 / PI;
			if (pitch_accel > 0)
				pitch_accel = -180 + pitch_accel;
			
			
			pitch = factor*pitch + factor_1* pitch_accel*17500/9;
			
			GPIO_SetBits(GPIOA, GPIO_Pin_4);
//			if(i++ %10== 0)
//			{
				//printf("xyz=%f,%f,%f, dx,dy,dz = %d,%d,%d, angel(pitch,roll,yaw, pitch_accel)=%f,%f,%f,%f\r\n", pitch, roll, yaw, p->gyro_x, p->gyro_y, p->gyro_z, pitch*9/17500, roll*9/17500 , yaw*9/17500, pitch_accel);
				//printf("accel xyz = %d, %d, %d, pitch_accel = %f\r\n", p->accel_x, p->accel_y, p->accel_z, pitch_accel);
//				printf("pitch, pitch_accel, pitchI=%f,%f, %f \r\n", pitch*9/17500, pitch_accel, pitchI*9/17500);
//			}
			
			/*
			for(i=0; i<6; i++)
			{
				USART_SendData(USART1, data[i]);
				while( USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET );
			}
			USART_SendData(USART1, 0xb0);
			while( USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET );
			USART_SendData(USART1, 0x3d);
			while( USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET );
			*/
			
			
			yaw_mag = atan2(p->mag_x - centerx, p->mag_y - centery) * 180.0 / PI;
			yaw += p->gyro_z - bz;
			yawI += p->gyro_z - bz;
			yaw = yaw*factor + factor_1* yaw_mag*17500/9;
			
			printf("yaw, yawI, yaw_mag = %f, %f, %f\r\n", yaw*9/17500, yawI*9/17500, yaw_mag);
			
			
			while(GetSysTickCount()-start_tick < 800)
				;
		}
		
		
		
	}while(0);

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
		
	}
}
/******************* (C) COPYRIGHT 2012 ***************** *****END OF FILE************/
