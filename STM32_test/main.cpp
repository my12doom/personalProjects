#include <stdio.h>

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
#include "common/vector.h"
}

#define PI 3.14159265

typedef struct
{
	short mag[3];			// 
	
	short accel[3];		// roll, pitch, yaw
	
	short temperature1;
	
	short gyro[3];		// roll, pitch, yaw
	
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


float fmax(float a, float b)
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

// a & b : -PI ~ PI
// return a - b
float radian_delta(float a, float b)
{
	float d1 = a-b;
	float d2 = -(a+2*PI-b);

	return abs(d1) > abs(d2) ? d2 : d1;
}



int main(void)
{
	// Basic Initialization
	SysTick_Config(720);
	printf_init();
	SPI_NRF_Init();
	printf("NRF_Check() = %d\r\n", NRF_Check());
	PPM_init(1);
	I2C_init(0x30);
	init_timer();
	init_MPU6050();
	init_HMC5883();	
	init_MS5611();	
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	sensor_data *p = new sensor_data;
	do
	{
		vector estAccGyro = {0};			// for roll & pitch
		vector estMagGyro = {0};			// for yaw
		vector estGyro = {0};				// for gyro only yaw, yaw lock on this
		
		vector mag_avg = {0};
		vector gyro_base = {0};
		vector accel_avg = {0};
		
		
		float accel_max = 0;
		
		
		for(int i=0; i<125; i++)
		{
			printf("\r%d/1000", i);
			read_MPU6050(p->accel);
			read_HMC5883(p->mag);
			
			vector gyro = {-p->gyro[0], -p->gyro[1], -p->gyro[2]};
			vector acc = {-p->accel[1], p->accel[0], p->accel[2]};
			vector mag = {p->mag[1], -p->mag[0], -p->mag[2]};
			vector_add(&gyro_base, &gyro);
			vector_add(&accel_avg, &acc);
			vector_add(&mag_avg, &mag);
			delayms(8);
		}
		

		vector_divide(&gyro_base, 125);
		vector_divide(&accel_avg, 125);
		vector_divide(&mag_avg, 125);

		
		
		vector target = {0};
		vector targetM = {0};
		accel_max = sqrt(accel_avg.V.x*accel_avg.V.x+ accel_avg.V.y*accel_avg.V.y + accel_avg.V.z * accel_avg.V.z);
		target = estAccGyro = estGyro = accel_avg;
		targetM = estMagGyro = mag_avg;


		
		printf("MPU6050 base value measured\r\n");
			
		int i = 0;
		vector GyroI = {0};
		while(1)
		{
			static const float factor = 0.995;
			static const float factor_1 = 1-factor;
			int start_tick = getus();
			
			GPIO_ResetBits(GPIOA, GPIO_Pin_4);
			
			read_MPU6050(&p->accel[0]);
			read_HMC5883(&p->mag[0]);
			
			static float max_gyro = 0;

			static const float GYRO_SCALE = 2000.0 * PI / 180 / 8192 * 0.0002;		// full scale: +/-2000 deg/s  +/-8192, 8ms interval, divided into 10 piece to better use small angle approximation
			vector gyro = {-p->gyro[0], -p->gyro[1], -p->gyro[2]};
			vector acc = {-p->accel[1], p->accel[0], p->accel[2]};
			vector mag = {p->mag[1], -p->mag[0], -p->mag[2]};
			vector_sub(&gyro, &gyro_base);
			vector_multiply(&gyro, GYRO_SCALE);
			vector_add(&GyroI, &gyro);
			max_gyro = fmax(gyro.array[0], max_gyro);
			
			float acc_g = vector_length(&acc)/ vector_length(&accel_avg);

			for(int j=0; j<10; j++)
			{
				vector_rotate(&estGyro, gyro.array);
				vector_rotate(&estAccGyro, gyro.array);
				vector_rotate(&estMagGyro, gyro.array);
			}
			
			// apply CF filter if g force is acceptable
			if (acc_g > 0.85 && acc_g < 1.15)
			{
				vector acc_f = acc;
				vector_multiply(&acc_f, factor_1);
				vector_multiply(&estAccGyro, factor);
				vector_add(&estAccGyro, &acc_f);
			}

			// apply CF filter for Mag
			vector mag_f = mag;
			vector_multiply(&mag_f, factor_1);
			vector_multiply(&estMagGyro, factor);
			vector_add(&estMagGyro, &mag_f);
			
			
			
			GPIO_SetBits(GPIOA, GPIO_Pin_4);
			if(i++ %10== 0)
			{
				float roll = atan2(estAccGyro.V.x, estAccGyro.V.z) * 180 / PI;
				float pitch = atan2(estAccGyro.V.y, sqrt(estAccGyro.V.x*estAccGyro.V.x + estAccGyro.V.z * estAccGyro.V.z)) * 180 / PI;
				
				vector estAccGyro16 = estAccGyro;
				vector_divide(&estAccGyro16, 16);
				float xxzz = (estAccGyro16.V.x*estAccGyro16.V.x + estAccGyro16.V.z * estAccGyro16.V.z);
				float G = sqrt(xxzz+estAccGyro16.V.y*estAccGyro16.V.y);
				float yaw_est = atan2(estMagGyro.V.z * estAccGyro16.V.x - estMagGyro.V.x * estAccGyro16.V.z,
					(estMagGyro.V.y * xxzz - (estMagGyro.V.x * estAccGyro16.V.x + estMagGyro.V.z * estAccGyro16.V.z) *estAccGyro16.V.y )/G) * 180 / PI;
				float yaw_gyro = 0;
				
				printf("\r mag:%f,%f,%f, gyro_est:%f,%f,%f, yaw_est=%f", mag.V.x, mag.V.y, mag.V.z, estMagGyro.V.x, estMagGyro.V.y, estMagGyro.V.z, yaw_est);
				//printf("\r gyro:%f, %f, %f, acc:%f, %f, %f,  max_gyro=%f, roll, pitch,yaw = %f,%f,%f         ", estAccGyro.array[0], estAccGyro.array[1], estAccGyro.array[2], acc.array[0], acc.array[1], acc.array[2], max_gyro, roll, pitch, yaw_est);
				//printf("\rGyroI: %f, %f, %f", GyroI.array[0]* 180 / PI, GyroI.array[1]* 180 / PI, GyroI.array[2]* 180 / PI);
				//printf("xyz=%f,%f,%f, dx,dy,dz = %d,%d,%d, angel(pitch,roll,yaw, pitch_accel)=%f,%f,%f,%f\r\n", pitch, roll, yaw, p->gyro[0], p->gyro[1], p->gyro[2], pitch*9/17500, roll*9/17500 , yaw*9/17500, pitch_accel);
				//printf("accel xyz = %d, %d, %d, pitch_accel = %f\r\n", p->accel[0], p->accel[1], p->accel[2], pitch_accel);
				//printf("pitch, pitchI, pitch_accel, roll, rollI, roll_accel=%f,%f,%f,%f,%f,%f \r\n", pitch*9/17500, pitchI*9/17500, pitch_accel, roll*9/17500, rollI*9/17500, roll_accel);
				
				//printf("\rdelta roll,pitch=%f, %f", (roll - roll_target)*9/17500, (pitch - pitch_target)*9/17500);
				
				//printf("\rmag[2]ero=%d,%d,%d, mag=%d,%d,%d, r=%d                 ", (mag[0]_min+mag[0]_max)/2, (mag[1]_min+mag[1]_max)/2, (mag[2]_min+mag[2]_max)/2, 
				//	dx, dy, dz, (int)sqrt(dx*dx+dy*dy+dz*dz));

				//printf("\rraw data: accel = %d,%d,%d, gyro=%d,%d,%d, mag=%d,%d,%d, i=%d,                ", p->accel[0], p->accel[1], p->accel[2], p->gyro[0], p->gyro[1], p->gyro[2], 
				//			p->mag[0], p->mag[1], p->mag[2], i);
			}

			//printf("%d %d %d\r\n", p->mag[0], p->mag[1], p->mag[2]);
			
			//g_ppm_output[0] = 1500 - (roll - roll_target)*9/17500 * 25;
			//g_ppm_output[1] = 1500 + (pitch - pitch_target)*9/17500 * 12;
			//g_ppm_output[2] = 1500 + (yaw - yaw_target)*9/17500 * 12;
			
			for(int i=0; i<3; i++)
			{
				g_ppm_output[i] = max(1000, g_ppm_output[i]);
				g_ppm_output[i] = min(2000, g_ppm_output[i]);
			}
			
			PPM_update_output_channel(PPM_OUTPUT_CHANNEL0 | PPM_OUTPUT_CHANNEL1 | PPM_OUTPUT_CHANNEL2);
						
			//printf("yaw, yawI, yaw_mag, time = %f, %f, %f, %f\r\n", yaw*9/17500, yawI*9/17500, yaw_mag, (float)GetSysTickCount()/100000.0f);
			//printf("%d\t%d\t%d\r", p->mag[0], p->mag[1], p->mag[2]);
			
			//printf("\r roll, pitch, yaw = %f, %f, %f", roll*9/17500, pitch*9/17500, yaw*9/17500);
			
			while(getus()-start_tick < 8000)
				;
		}
		
		
		
	}while(0);	
}
