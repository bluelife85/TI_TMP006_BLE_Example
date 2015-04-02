#ifndef CC2541_SOFTI2C_H_
#define CC2541_SOFTI2C_H_

#include <iocc2541.h>
#include "hal_types.h"

#define I2C_SDA_PIN						4
#define I2C_SCL_PIN						5

#define I2C_SDA_PIN_VALUE					0x10

#define I2C_SDA_PIN_DIR						P0DIR_4
#define I2C_SCL_PIN_DIR						P0DIR_5

#define I2C_SDA_PIN_PORT					P0_4
#define I2C_SCL_PIN_PORT					P0_5

#define I2C_SDA_READ						(P0 & I2C_SDA_PIN_VALUE)
#define I2C_SDA_OUTPUT						(I2C_SDA_PIN_DIR = 1)
#define I2C_SCL_OUTPUT						(I2C_SCL_PIN_DIR = 1)

#define I2C_SDA_INPUT						(I2C_SDA_PIN_DIR = 0)
#define I2C_SCL_INPUT						(I2C_SCL_PIN_DIR = 0)

#define I2C_SDA_HIGH						(I2C_SDA_PIN_PORT = 1)
#define I2C_SDA_LOW						(I2C_SDA_PIN_PORT = 0)

#define I2C_SCL_HIGH						(I2C_SCL_PIN_PORT = 1)
#define I2C_SCL_LOW						(I2C_SCL_PIN_PORT = 0)

//#ifndef uint8
//typedef unsigned char uint8;
//#endif
//
//#ifndef int8
//typedef char int8;
//#endif
//
//#ifndef uint16
//typedef unsigned short uint16;
//#endif
//
//#ifndef int16
//typedef short int16;
//#endif
//
//#ifndef uint32
//typedef unsigned long uint32;
//#endif
//
//#ifndef int32
//typedef long int32;
//#endif

void delay_50us(void);
void SoftI2C_init(void);
void SoftI2C_start(void);
void SoftI2C_repstart(void);
void SoftI2C_stop(void);
uint8 SoftI2C_write( uint8 c );
uint8 SoftI2C_read( uint8 ack );
uint8 SoftI2C_beginTransmission(uint8 addr);
uint8 SoftI2C_requestFrom( uint8 addr );
void SoftI2C_endTransmission(void);

#endif
