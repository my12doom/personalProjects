extern "C"
{
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
	int result;
	u8 data[32] = {0};


	// Basic Initialization
	SysTick_Config(720);
	printf_init();
	SPI_NRF_Init();
	i = NRF_Check();
	printf("NRF_Check() = %d", i);
	PPM_init(1);
	I2C_init(0x30);
	init_timer();
	

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
			delayms(1);
		}
		
		bx /= 1000;
		by /= 1000;
		bz /= 1000;
		accel_avg_x /= 1000;
		accel_avg_y /= 1000;
		accel_avg_z /= 1000;
		accel_max /= 1000;
		accel_max = sqrt(accel_max);
		pitch = -atan(-(float)p->accel_x / sqrt((float)p->accel_z*p->accel_z+p->accel_y*p->accel_y)) * 180 / PI *17500/9;
		roll = -atan(-(float)p->accel_y / p->accel_z) * 180 / PI *17500/9;
		yaw = atan2((float)p->mag_x, (float)p->mag_y) * 180.0 / PI*17500/9;
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
			
			pitch_accel = -atan(-(float)p->accel_x / sqrt((float)p->accel_z*p->accel_z+p->accel_y*p->accel_y)) * 180 / PI;			
			roll_accel = -atan(-(float)p->accel_y / p->accel_z) * 180 / PI;
			
			pitch = factor*pitch + factor_1* pitch_accel*17500/9;
			roll = factor*roll + factor_1* roll_accel*17500/9;
			
			yaw_mag = atan2((float)p->mag_x, (float)p->mag_y) * 180.0 / PI;
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
		}
	}
}
