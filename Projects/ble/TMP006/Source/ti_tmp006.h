#ifndef TI_TMP006_H_
#define TI_TMP006_H_

#include "iocc2541.h"
#include "softi2c.h"

#define TMP006_I2CADDR			0x40

#define TMP006_CONFIG			0x02
#define TMP006_READ_VOBJ		0x00
#define TMP006_READ_TAMB		0x01
#define TMP006_READ_MANID		0xFE
#define TMP006_READ_DEVID		0xFF

#define TMP006_CFG_RESET		0x8000
#define TMP006_CFG_MODEON		0x7000
#define TMP006_CFG_1SAMPLE		0x0000
#define TMP006_CFG_2SAMPLE		0x0200
#define TMP006_CFG_4SAMPLE		0x0400
#define TMP006_CFG_8SAMPLE		0x0600
#define TMP006_CFG_16SAMPLE		0x0800
#define TMP006_CFG_DRDYEN		0x0100
#define TMP006_CFG_DRDY			0x0080

#ifndef sei
#define sei()				(EA=1)
#endif

#ifndef cli
#define cli()				(EA=0)
#endif

void TMP006_start(void);
void TMP006_config(void);
void TMP006_write16(uint8 reg, uint16 val);
uint16 TMP006_read16(uint8 reg);
void TMP006_sleep(void);
void TMP006_wake(void);
uint16 TMP006_readTAMBValue(void);
uint16 TMP006_readVOBJValue(void);

#endif
