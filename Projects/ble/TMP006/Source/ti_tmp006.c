#include "ti_tmp006.h"

void TMP006_start(void)
{
	SoftI2C_init();
	P2DIR = 0x1F;
	P2_0 = 1;
}

void TMP006_config(void)
{
	uint16 config = TMP006_CFG_MODEON | TMP006_CFG_DRDYEN | TMP006_CFG_16SAMPLE;
	
	TMP006_write16( TMP006_CONFIG, config );
}

void TMP006_sleep(void)
{
	uint16 config = TMP006_read16(TMP006_CONFIG);
	
	config &= ~(TMP006_CFG_MODEON);
	TMP006_write16( TMP006_CONFIG, config);
}

void TMP006_wake(void)
{
	uint16 config = TMP006_read16(TMP006_CONFIG);
	
	config |= TMP006_CFG_MODEON;
	TMP006_write16( TMP006_CONFIG, config);
}

uint16 TMP006_readTAMBValue(void)
{
	uint16 ret = TMP006_read16(TMP006_READ_TAMB);
	
	return ret;
}

uint16 TMP006_readVOBJValue(void)
{
	uint16 ret = TMP006_read16(TMP006_READ_VOBJ);
	
	return ret;
}

void TMP006_write16(uint8 reg, uint16 val)
{
	uint8 high = (uint8)(val >> 8);
	uint8 low = (uint8)(val & 0xff);
	
	SoftI2C_beginTransmission(TMP006_I2CADDR);
	SoftI2C_write(reg);
	SoftI2C_write(high);
	SoftI2C_write(low);
	SoftI2C_endTransmission();
}

uint16 TMP006_read16(uint8 reg)
{
	uint16 ret = 0;
	uint8 high;
	uint8 low;
	
	SoftI2C_beginTransmission(TMP006_I2CADDR);
	SoftI2C_write(reg); // read die
	SoftI2C_endTransmission();
	SoftI2C_requestFrom(TMP006_I2CADDR);
	high = SoftI2C_read(1);
	low = SoftI2C_read(0);
	SoftI2C_endTransmission();
	
	ret = (uint16)high << 8 | (uint16)low;
	
	return ret;
}