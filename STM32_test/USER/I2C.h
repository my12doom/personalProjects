#ifndef __I2C_H__
#define __I2C_H__

#include "stm32f10x.h"

// return 0 on success, -1 on error

int I2C_init(u8 OwnAddress1);
int I2C_ReadReg(u8 SlaveAddress, u8 startRegister, u8*out, int count);
int I2C_WriteReg(u8 SlaveAddress, u8 Register, u8 data);

#endif
