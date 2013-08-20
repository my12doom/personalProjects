#ifndef __I2C_H__
#define __I2C_H__

#include "stm32f10x.h"

// return 0 on success, -1 on error

extern int I2C_init(u8 OwnAddress1);
extern int I2C_ReadReg(u8 SlaveAddress, u8 startRegister, u8*out, int count);
extern int I2C_WriteReg(u8 SlaveAddress, u8 Register, u8 data);
extern int I2C_WriteReg2(u8 SlaveAddress, u8 Register, u8 data);

#endif
