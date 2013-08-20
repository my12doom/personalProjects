#include "MS5611.h"
#include "../common/I2C.h"
#include "../common/common.h"

// registers of the device
#define MS561101BA_ADDR_CSB_LOW   0xEC
#define MS5611Address (0x77<<1)
#define MS561101BA_D1 0x40
#define MS561101BA_D2 0x50
#define MS561101BA_RESET 0x1E
// OSR (Over Sampling Ratio) constants
#define MS561101BA_OSR_256 0x00
#define MS561101BA_OSR_512 0x02
#define MS561101BA_OSR_1024 0x04
#define MS561101BA_OSR_2048 0x06
#define MS561101BA_OSR_4096 0x08

#define MS561101BA_PROM_BASE_ADDR 0xA2 // by adding ints from 0 to 6 we can read all the prom configuration values. 
// C1 will be at 0xA2 and all the subsequent are multiples of 2
#define MS561101BA_PROM_REG_COUNT 6 // number of registers in the PROM
#define MS561101BA_PROM_REG_SIZE 2 // size in bytes of a prom registry.
#define EXTRA_PRECISION 0 // trick to add more precision to the pressure and temp readings

#define SAMPLEING_TIME 1000 // 10ms

u8 OSR = MS561101BA_OSR_4096;
int temperature = 0;
int pressure = 0;
int new_temperature = 0;
int last_temperature_time = 0;
int last_pressure_time = 0;
int64_t rawTemperature = 0;
int64_t rawPressure = 0;
int64_t DeltaTemp = 0;
int64_t off;//  = (((int64_t)_C[1]) << 16) + ((_C[3] * dT) >> 7);
int64_t sens;// = (((int64_t)_C[0]) << 15) + ((_C[2] * dT) >> 8);
u16 refdata[6];

// call initI2C before this
int init_MS5611(void)
{
	u8 tmp[3];
	int i;
	
	I2C_WriteReg(MS5611Address, MS561101BA_RESET, 0x00);	
	msdelay(1000);
	for(i=0; i<6; i++)
	{
		I2C_ReadReg(MS5611Address, MS561101BA_PROM_BASE_ADDR+i*2, tmp, 2);
		refdata[i] = (tmp[0] << 8) + tmp[1];
	}
	
	// Temperature
	I2C_WriteReg(MS5611Address, MS561101BA_D2 + OSR, 0x00);
	msdelay(10000);
	I2C_ReadReg(MS5611Address, 0x00, tmp, 3);
	
	rawTemperature = ((int)tmp[0] << 16) + ((int)tmp[1] << 8) + (int)tmp[2];
	DeltaTemp = rawTemperature - (((int32_t)refdata[4]) << 8);
	temperature = ((1<<EXTRA_PRECISION)*2000l + ((DeltaTemp * refdata[5]) >> (23-EXTRA_PRECISION))) / ((1<<EXTRA_PRECISION));
		
	// Pressure
	I2C_WriteReg(MS5611Address, MS561101BA_D1 + OSR, 0x00);
	msdelay(10000);
	I2C_ReadReg(MS5611Address, 0x00, tmp, 3);
	
	rawPressure = ((int)tmp[0] << 16) + ((int)tmp[1] << 8) + (int)tmp[2];
	off  = (((int64_t)refdata[1]) << 16) + ((refdata[3] * DeltaTemp) >> 7);
	sens = (((int64_t)refdata[0]) << 15) + ((refdata[2] * DeltaTemp) >> 8);
	pressure = ((((rawPressure * sens) >> 21) - off) >> (15-EXTRA_PRECISION)) / ((1<<EXTRA_PRECISION));
	
	return 0;
}


int read_MS5611(int *data)
{
	int rtn = 1;
	u8 tmp[3];
	if (new_temperature == 0 && last_temperature_time == 0 && last_pressure_time == 0)
	{
		last_temperature_time = GetSysTickCount();
		I2C_WriteReg(MS5611Address, MS561101BA_D2 + OSR, 0x00);
	}
	
	if (GetSysTickCount() - last_temperature_time >  SAMPLEING_TIME && new_temperature == 0)
	{
		I2C_ReadReg(MS5611Address, 0x00, tmp, 3);
	
		rawTemperature = ((int)tmp[0] << 16) + ((int)tmp[1] << 8) + (int)tmp[2];
		DeltaTemp = rawTemperature - (((int32_t)refdata[4]) << 8);
		new_temperature = ((1<<EXTRA_PRECISION)*2000l + ((DeltaTemp * refdata[5]) >> (23-EXTRA_PRECISION))) / ((1<<EXTRA_PRECISION));
		
		last_pressure_time = GetSysTickCount();
		I2C_WriteReg(MS5611Address, MS561101BA_D1 + OSR, 0x00);
	}

	if (GetSysTickCount() - last_pressure_time >  SAMPLEING_TIME && last_pressure_time > 0)
	{
		I2C_ReadReg(MS5611Address, 0x00, tmp, 3);
		rawPressure = ((int)tmp[0] << 16) + ((int)tmp[1] << 8) + (int)tmp[2];
		off  = (((int64_t)refdata[1]) << 16) + ((refdata[3] * DeltaTemp) >> 7);
		sens = (((int64_t)refdata[0]) << 15) + ((refdata[2] * DeltaTemp) >> 8);
		pressure = ((((rawPressure * sens) >> 21) - off) >> (15-EXTRA_PRECISION)) / ((1<<EXTRA_PRECISION));
		temperature = new_temperature;

		new_temperature = 0;
		last_temperature_time = 0;
		last_pressure_time = 0;
		rtn = 0;
	}
	
	data[0] = pressure;
	data[1] = temperature;
	return rtn;
}
