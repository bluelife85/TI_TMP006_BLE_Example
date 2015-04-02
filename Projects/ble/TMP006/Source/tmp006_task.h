#ifndef TI_TMP006_TASK_H_
#define TI_TMP006_TASK_H_

#define TMP006_START_DEVICE_EVT				0x0001
#define TMP006_POWER_ON_EVT				0x0002
#define TMP006_READ_VOBJ_EVT				0x0004
#define TMP006_READ_TAMB_EVT				0x0008
#define TMP006_POWER_OFF_EVT				0x0010

void Temp_Init( uint8 task_id );
uint16 TMP006_ProcessEvent( uint8 task_id, uint16 events );

#endif
