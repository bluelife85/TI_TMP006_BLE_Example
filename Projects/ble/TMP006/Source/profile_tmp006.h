#ifndef PROFILE_TMP006_H_
#define PROFILE_TMP006_H_

#define TMP006_SERVICE_UUID				0x84A0

#define TMP006_POWER_CTRL_UUID				0x84A1
#define TMP006_VOBJ_UUID				0x84A2
#define TMP006_TAMB_UUID				0x84A3

#define TMP006_SERVICE					0x00000001

#define TMP006_POWER_CTRL				1
#define TMP006_VOBJ					2
#define TMP006_TAMB					3

typedef void (*TMP006_ble_t) (uint8 ctrl);

typedef struct 
{
	TMP006_ble_t pfPower;
} TMP006CB_t;

bStatus_t TMP006_AddService( uint32 services );
bStatus_t TMP006_SetParameter( uint8 param, uint8 len, void* pValue );
bStatus_t TMP006_RegistAppCBs( TMP006CB_t* cb );

#endif
