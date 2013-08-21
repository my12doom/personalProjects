/******************** (C) COPYRIGHT 2012 ***************** **************************
 * 文件名  ：main.c
 * 描述    ：串口1(USART1)向电脑的超级终端以1s为间隔打印当前ADC1的转换电压值         
 * 实验平台：STM32开发板
 * 库版本  ：ST3.5.0
 *
**********************************************************************************/
#include "stm32f10x.h"
#include "common/printf.h"
#include "common/adc.h"
#include "common/I2C.h"
#include "common/NRF24L01.h"
#include "math.h"
#include "common/PPM.h"
#include "common/sdio_sdcard.h"
#include "common/common.h"
#include "sensors/HMC5883.h"
#include "sensors/MPU6050.h"
#include "sensors/MS5611.h"
#include <stdio.h>


// ADC1转换的电压值通过MDA方式传到SRAM
extern __IO uint16_t ADC_ConvertedValue;

// 局部变量，用于保存转换计算后的电压值			 

float ADC_ConvertedValueLocal;        

// 软件延时
void Delay(__IO uint32_t nCount)
{
  for(; nCount != 0; nCount--);
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



int abs(int i)
{
	if (i>0)
		return i;
	return -i;
}

int max(int a, int b)
{
	if (a>b)
		return a;
	return b;
}

int min(int a, int b)
{
	if (a<b)
		return a;
	return b;
}

#define PI 3.14159265



int main(void)
{
	int i=0;
	int j=0;
	int result;
	float avg;
	int total_tx = 0;
	u8 data[32] = {0};
	u8 rawdata[3] = {0};
	GPIO_InitTypeDef GPIO_InitStructure;

	u32 Status;
	SD_Error err;
	int block_to_test;
	extern SD_CardInfo SDCardInfo;
	SysTick_Config(720);

	// Basic Initialization
	printf_init();
	SPI_NRF_Init();
	i = NRF_Check();
	printf("NRF_Check() = %d", i);
	PPM_init(1);
	I2C_init(0x30);
	init_timer();
	
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	while(0)
	{
		int64_t t = getus();
		int64_t t2 = getus();
		
		if (t2<t)
			printf("!!!! %d=%d-%d\r\n", (int)(t-t2), (int)(t%60000), (int)(t2%60000));
		if (t2-t>100)
			printf("!! %d=%d-%d\r\n", (int)(t2-t), (int)(t2%60000), (int)(t%60000));
	}
	
	{
		int t = TIM2->CNT - t;
		int t2;
		int delta[10] = {0};
		while(0)
		{
			GPIO_SetBits(GPIOA, GPIO_Pin_5);
			
			GPIO_SetBits(GPIOA, GPIO_Pin_4);
			delayus(10);
			GPIO_ResetBits(GPIOA, GPIO_Pin_4);
			delayus(10);
			GPIO_SetBits(GPIOA, GPIO_Pin_4);
			delayus(10);
			GPIO_ResetBits(GPIOA, GPIO_Pin_4);
			delayus(10);
			GPIO_SetBits(GPIOA, GPIO_Pin_4);
			delayus(10);
			GPIO_ResetBits(GPIOA, GPIO_Pin_4);
			delayus(10);
			
			GPIO_ResetBits(GPIOA, GPIO_Pin_5);
		}
	
	}
// SD Card test
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
	
	
	/*
	while(1)
	{
		i++;
		for(j=0; j<8; j++)
			g_ppm_output[j] = 1000+ abs(1000 - ((i/100) % 2000));
		PPM_update_output_channel(PPM_OUTPUT_CHANNEL2);
		if (i % 2000 == 0)
			printf("\r PPM input = %d, %d, %d, %d", g_ppm_input[0], g_ppm_input[1], g_ppm_input[2], g_ppm_input[3]);
		;
	}
	*/



	

	do
	{
		float pitch = 0;
		float pitchI = 0;
		float roll = 0;
		float rollI = 0;
		float yaw = 0;
		float yawI = 0;
		float bx = 0;
		float by = 0;
		float bz = 0;
		float accel_max = 0;
		float accel_avg_x = 0;
		float accel_avg_y = 0;
		float accel_avg_z = 0;
		float roll_target = 0;
		float pitch_target = 0;
		float yaw_target = 0;
		sensor_data *p = (sensor_data*)data;
		int z0=0, z1=0;
		
		init_MPU6050();
		
		// HMC5883 self test
		init_HMC5883();
		
		init_MS5611();
		
		for(i=0; i<1000; i++)
		{
			read_MPU6050(&p->accel_x);
			read_HMC5883(&p->mag_x);
									
			bx+= p->gyro_x;
			by+= p->gyro_y;
			bz+= p->gyro_z;
			accel_max += (p->accel_x*p->accel_x+ p->accel_y*p->accel_y + p->accel_z * p->accel_z);
			accel_avg_x += p->accel_x;
			accel_avg_y += p->accel_y;
			accel_avg_z += p->accel_z;
			msdelay(100);
		}
		
		bx /= 1000;
		by /= 1000;
		bz /= 1000;
		accel_avg_x /= 1000;
		accel_avg_y /= 1000;
		accel_avg_z /= 1000;
		accel_max /= 1000;
		accel_max = sqrt(accel_max);
		pitch = -atan(-(float)p->accel_x / sqrt(p->accel_z*p->accel_z+p->accel_y*p->accel_y)) * 180 / PI *17500/9;
		roll = -atan(-(float)p->accel_y / p->accel_z) * 180 / PI *17500/9;
		yaw = atan2(p->mag_x, p->mag_y) * 180.0 / PI*17500/9;
		roll_target = roll;
		pitch_target = pitch;
		yaw_target = yaw;
		
		
		printf("MPU6050 base value measured, b xyz=%f,%f,%f, accel_max=%f, roll,pitch=%f,%f \r\n", bx, by, bz, accel_max, roll, pitch);
				
		i=0;
		while(1)
		{
			float roll_accel;
			float pitch_accel;
			float yaw_mag;
			float pitch_mag;
			static const float factor = 0.995;
			static const float factor_1 = 1-factor;
			int start_tick = getus();
			
			GPIO_ResetBits(GPIOA, GPIO_Pin_4);
			
			read_MPU6050(&p->accel_x);
			read_HMC5883(&p->mag_x);			
			
			pitch +=  p->gyro_y - by ;
			pitchI += p->gyro_y - by ;
			roll += p->gyro_x - bx;
			rollI += p->gyro_x - bx;
			
			pitch_accel = -atan(-(float)p->accel_x / sqrt(p->accel_z*p->accel_z+p->accel_y*p->accel_y)) * 180 / PI;			
			roll_accel = -atan(-(float)p->accel_y / p->accel_z) * 180 / PI;
			
			pitch = factor*pitch + factor_1* pitch_accel*17500/9;
			roll = factor*roll + factor_1* roll_accel*17500/9;
			
			yaw_mag = atan2(p->mag_x, p->mag_y) * 180.0 / PI;
			yaw += p->gyro_z - bz;
			yawI += p->gyro_z - bz;
			yaw = yaw*factor + factor_1* yaw_mag*17500/9;
			
			GPIO_SetBits(GPIOA, GPIO_Pin_4);
			if(i++ %10== 0)
			{
				//printf("xyz=%f,%f,%f, dx,dy,dz = %d,%d,%d, angel(pitch,roll,yaw, pitch_accel)=%f,%f,%f,%f\r\n", pitch, roll, yaw, p->gyro_x, p->gyro_y, p->gyro_z, pitch*9/17500, roll*9/17500 , yaw*9/17500, pitch_accel);
				//printf("accel xyz = %d, %d, %d, pitch_accel = %f\r\n", p->accel_x, p->accel_y, p->accel_z, pitch_accel);
				//printf("pitch, pitchI, pitch_accel, roll, rollI, roll_accel=%f,%f,%f,%f,%f,%f \r\n", pitch*9/17500, pitchI*9/17500, pitch_accel, roll*9/17500, rollI*9/17500, roll_accel);
				
				//printf("\rdelta roll,pitch=%f, %f", (roll - roll_target)*9/17500, (pitch - pitch_target)*9/17500);
				
				//printf("\rmag_zero=%d,%d,%d, mag=%d,%d,%d, r=%d                 ", (mag_x_min+mag_x_max)/2, (mag_y_min+mag_y_max)/2, (mag_z_min+mag_z_max)/2, 
				//	dx, dy, dz, (int)sqrt(dx*dx+dy*dy+dz*dz));
			}
			
			g_ppm_output[0] = 1500 - (roll - roll_target)*9/17500 * 25;
			g_ppm_output[1] = 1500 + (pitch - pitch_target)*9/17500 * 12;
			g_ppm_output[2] = 1500 + (yaw - yaw_target)*9/17500 * 12;
			
			for(i=0; i<3; i++)
			{
				g_ppm_output[i] = max(1000, g_ppm_output[i]);
				g_ppm_output[i] = min(2000, g_ppm_output[i]);
			}
			
			PPM_update_output_channel(PPM_OUTPUT_CHANNEL0 | PPM_OUTPUT_CHANNEL1 | PPM_OUTPUT_CHANNEL2);
			
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
			
			
			
			//printf("yaw, yawI, yaw_mag, time = %f, %f, %f, %f\r\n", yaw*9/17500, yawI*9/17500, yaw_mag, (float)GetSysTickCount()/100000.0f);
			//printf("%d\t%d\t%d\r", p->mag_x, p->mag_y, p->mag_z);
			
			printf("\r roll, pitch, yaw = %f, %f, %f", roll*9/17500, pitch*9/17500, yaw*9/17500);
			
			while(getus()-start_tick < 8000)
				;
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
