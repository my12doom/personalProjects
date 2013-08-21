#include "MPU6050.h"
#include <stdio.h>
#include "../common/I2C.h"
#include "../common/common.h"

#define MPU6050SlaveAddress 0xD0
#define MPU6050_REG_WHO_AM_I 0x75

// Gyro and accelerator registers
#define	SMPLRT_DIV		0x19	//??????,???:0x07(125Hz)
#define	MPU6050_CONFIG 0x1A	//??????,???:0x06(5Hz)
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
#define EXT_SENS_DATA 0x49
#define	PWR_MGMT_1		0x6B	//????,???:0x00(????)
#define	WHO_AM_I		0x75	//IIC?????(????0x68,??)

// call initI2C before this
int init_MPU6050(void)
{
	u8 who_am_i = 0;

	printf("start MPU6050\r\n");
	delayms(10);
	I2C_WriteReg(MPU6050SlaveAddress, PWR_MGMT_1, 0x00);
	I2C_WriteReg(MPU6050SlaveAddress, SMPLRT_DIV, 0x07);
	I2C_WriteReg(MPU6050SlaveAddress, MPU6050_CONFIG, 0x06);
	I2C_WriteReg(MPU6050SlaveAddress, GYRO_CONFIG, 0x18);
	I2C_WriteReg(MPU6050SlaveAddress, ACCEL_CONFIG, 0x08);
	
	I2C_ReadReg(MPU6050SlaveAddress, WHO_AM_I, &who_am_i, 1);
	printf("MPU6050 initialized, WHO_AM_I=%x\r\n", who_am_i);
	
	// enable I2C bypass for AUX I2C and initialize HMC5883 into continues mode
	I2C_WriteReg(MPU6050SlaveAddress, 0x37, 0x02);
	delayms(10);
	
	return 0;
}

// data[0 ~ 7] :
// accel_x, accel_y, accel_z, raw_temperature, gyro_x, gyro_y, gyro_z
int read_MPU6050(short*data)
{	
	int i;
	int o = I2C_ReadReg(MPU6050SlaveAddress, ACCEL_XOUT_H, (u8*)data, 14);
	for(i=0; i<7; i++)
		swap((u8*)&data[i], 2);
	
	return 0;
}
