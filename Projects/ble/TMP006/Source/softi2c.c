#include "softi2c.h"

void delay_50us(void){
	
	volatile uint16 i = 28;
	
	while(--i);
	
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP");
	asm("NOP"); // checked pulse through oscilloscope
}

void SoftI2C_init(void)
{
	I2C_SDA_OUTPUT;
	I2C_SCL_OUTPUT;
	
	I2C_SDA_HIGH;
	I2C_SCL_HIGH;
	delay_50us();
}

void SoftI2C_start(void)
{
	I2C_SDA_HIGH;
	I2C_SCL_HIGH;
	delay_50us();
	
	I2C_SDA_LOW;
	delay_50us();
	I2C_SCL_LOW;
	delay_50us();
}

void SoftI2C_repstart(void)
{
	I2C_SDA_HIGH;
	I2C_SCL_HIGH;
	delay_50us();
	
	I2C_SCL_LOW;
	delay_50us();
	
	delay_50us();
	
	delay_50us();
	
	I2C_SDA_LOW;
	delay_50us();
}

void SoftI2C_stop(void)
{
	I2C_SCL_HIGH;
	delay_50us();
	
	I2C_SDA_HIGH;
	delay_50us();
}

static void SoftI2C_writebit( uint8 b )
{
	if ( b )
	{
		I2C_SDA_HIGH;
	}
	else
	{
		I2C_SDA_LOW;
	}
	
	I2C_SCL_HIGH;
	delay_50us();
	
	I2C_SCL_LOW;
	delay_50us();
	
	if( b )
	{
		I2C_SDA_LOW;
	}
	
	delay_50us();
}

static uint8 SoftI2C_readbit(void)
{
	uint8 c;
	
	I2C_SDA_HIGH;
	I2C_SCL_HIGH;
	delay_50us();
	
	I2C_SDA_INPUT;
	c = I2C_SDA_READ;
	
	I2C_SCL_LOW;
	delay_50us();
	I2C_SDA_OUTPUT;
	return ( c & I2C_SDA_PIN_VALUE ) ? 1 : 0;
}

uint8 SoftI2C_write( uint8 c )
{
	uint8 i;
	for(i=0;i<8;i++)
	{
		SoftI2C_writebit( c & 0x80 );
		c <<= 1;
	}
	
	return SoftI2C_readbit();
}

uint8 SoftI2C_read( uint8 ack )
{
	uint8 i;
	uint8 res = 0;
	
	for(i=0;i<8;i++)
	{
		res <<= 1;
		res |= SoftI2C_readbit();
	}
	
	if ( ack )
	{
		SoftI2C_writebit(0);
	}
	else
	{
		SoftI2C_writebit(1);
	}
	
	delay_50us();
	
	return res;
}

uint8 SoftI2C_beginTransmission(uint8 addr)
{
	uint8 res;
	
	SoftI2C_start();
	
	res = SoftI2C_write((addr <<1) | 0);
	
	return res;
}

uint8 SoftI2C_requestFrom( uint8 addr )
{
	uint8 res;
	
	SoftI2C_start();
	res = SoftI2C_write((addr <<1) | 1);
	return res;
}

void SoftI2C_endTransmission(void)
{
	SoftI2C_stop();
}