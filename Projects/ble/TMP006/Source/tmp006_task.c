#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#include "OnBoard.h"
#include "hal_adc.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_lcd.h"

#include "gatt.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"

#include "peripheral.h"

#include "gapbondmgr.h"

#include "tmp006_task.h"
#include "profile_tmp006.h"
#include "ti_tmp006.h"

#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     80
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     800
#define DEFAULT_DESIRED_SLAVE_LATENCY         0
#define DEFAULT_DESIRED_CONN_TIMEOUT          1000
#define DEFAULT_ENABLE_UPDATE_REQUEST         TRUE
#define DEFAULT_CONN_PAUSE_PERIPHERAL         5

#define INVALID_CONNHANDLE                    0xFFFF

static uint8 tmp006_TaskID;

static gaprole_States_t gapProfileState = GAPROLE_INIT;

static uint8 deviceName[] = 
{
	0x08,
	0x09,
	'T',
	'E',
	'M',
	'P',
	'0',
	'0',
	'6'
};

static uint8 advertData[] =
{
	0x02,   // length of first data structure (2 bytes excluding length byte)
	GAP_ADTYPE_FLAGS,   // AD Type = Flags
	DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
	
	0x07,   // length of second data structure (7 bytes excluding length byte)
	GAP_ADTYPE_16BIT_MORE,   // list of 16-bit UUID's available, but not complete list
	LO_UINT16( TMP006_SERVICE_UUID ),        // Link Loss Service (Proximity Profile)
	HI_UINT16( TMP006_SERVICE_UUID )
};

static uint8 attDeviceName[11] = "BLE TMP006";

static void peripheralStateNotificationCB( gaprole_States_t newState );
static void TMP006PowerChangeCB(uint8 ctrl);
static void TMP006_ProcessOSALMsg( osal_event_hdr_t *pMsg );
static void TMP006_HandleKeys( uint8 shift, uint8 keys );

static gapRolesCBs_t temp_PeripheralCBs =
{
	peripheralStateNotificationCB,  // Profile State Change Callbacks
	NULL                // When a valid RSSI is read from controller
};

static gapBondCBs_t temp_BondMgrCBs =
{
	NULL,                     // Passcode callback (not used by application)
	NULL                      // Pairing / Bonding state Callback (not used by application)
};

static TMP006CB_t TMP006_PowerCBs = 
{
	TMP006PowerChangeCB
};

void Temp_Init( uint8 task_id )
{
	tmp006_TaskID = task_id;
	
	// Setup the GAP
	VOID GAP_SetParamValue( TGAP_CONN_PAUSE_PERIPHERAL, DEFAULT_CONN_PAUSE_PERIPHERAL );
	
	// Setup the GAP Peripheral Role Profile
	{
		// For the CC2540DK-MINI keyfob, device doesn't start advertising until button is pressed
		uint8 initial_advertising_enable = FALSE;
		
		// By setting this to zero, the device will go into the waiting state after
		// being discoverable for 30.72 second, and will not being advertising again
		// until the enabler is set back to TRUE
		uint16 gapRole_AdvertOffTime = 0;
		
		uint8 enable_update_request = DEFAULT_ENABLE_UPDATE_REQUEST;
		uint16 desired_min_interval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
		uint16 desired_max_interval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
		uint16 desired_slave_latency = DEFAULT_DESIRED_SLAVE_LATENCY;
		uint16 desired_conn_timeout = DEFAULT_DESIRED_CONN_TIMEOUT;
		
		// Set the GAP Role Parameters
		GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable );
		GAPRole_SetParameter( GAPROLE_ADVERT_OFF_TIME, sizeof( uint16 ), &gapRole_AdvertOffTime );
		
		GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( deviceName ), deviceName );
		GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );
		
		GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE, sizeof( uint8 ), &enable_update_request );
		GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof( uint16 ), &desired_min_interval );
		GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof( uint16 ), &desired_max_interval );
		GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY, sizeof( uint16 ), &desired_slave_latency );
		GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &desired_conn_timeout );
	}
	
	// Set the GAP Attributes
	GGS_SetParameter( GGS_DEVICE_NAME_ATT, 11, attDeviceName );
	
	// Setup the GAP Bond Manager
	{
		uint8 pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
		uint8 mitm = TRUE;
		uint8 ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
		uint8 bonding = TRUE;
		
		GAPBondMgr_SetParameter( GAPBOND_PAIRING_MODE, sizeof ( uint8 ), &pairMode );
		GAPBondMgr_SetParameter( GAPBOND_MITM_PROTECTION, sizeof ( uint8 ), &mitm );
		GAPBondMgr_SetParameter( GAPBOND_IO_CAPABILITIES, sizeof ( uint8 ), &ioCap );
		GAPBondMgr_SetParameter( GAPBOND_BONDING_ENABLED, sizeof ( uint8 ), &bonding );
	}
	
	GGS_AddService( GATT_ALL_SERVICES );         // GAP
	GATTServApp_AddService( GATT_ALL_SERVICES ); // GATT attributes
	TMP006_RegistAppCBs( &TMP006_PowerCBs );
	TMP006_AddService( GATT_ALL_SERVICES );
	
	RegisterForKeys( tmp006_TaskID );
	
	osal_set_event( tmp006_TaskID, TMP006_START_DEVICE_EVT );
}

uint16 TMP006_ProcessEvent( uint8 task_id, uint16 events )
{
	if ( events & SYS_EVENT_MSG )
	{
		uint8 *pMsg;
		
		if ( (pMsg = osal_msg_receive( tmp006_TaskID )) != NULL )
		{
			TMP006_ProcessOSALMsg( (osal_event_hdr_t *)pMsg );
			
			// Release the OSAL message
			VOID osal_msg_deallocate( pMsg );
		}
		
		// return unprocessed events
		return (events ^ SYS_EVENT_MSG);
	}
	
	if ( events & TMP006_START_DEVICE_EVT )
	{
		VOID GAPRole_StartDevice( &temp_PeripheralCBs );
		VOID GAPBondMgr_Register( &temp_BondMgrCBs );
		
		return ( events ^ TMP006_START_DEVICE_EVT );
	}
	
	if ( events & TMP006_POWER_ON_EVT )
	{
		uint8 power_on = TRUE;
		TMP006_SetParameter( TMP006_POWER_CTRL, sizeof( uint8 ) , &power_on );
		
		TMP006_start();
		TMP006_config();
		
		VOID osal_start_reload_timer( tmp006_TaskID, TMP006_READ_VOBJ_EVT, 1000 );
		VOID osal_start_reload_timer( tmp006_TaskID, TMP006_READ_TAMB_EVT, 1000 );
		
		return ( events ^ TMP006_POWER_ON_EVT );
	}
	
	if ( events & TMP006_READ_VOBJ_EVT )
	{
		uint16 vobj = 0;
		cli();
		vobj = TMP006_readVOBJValue();
		sei();
		
		TMP006_SetParameter( TMP006_VOBJ, sizeof( uint16 ) , &vobj );
		VOID osal_start_reload_timer( tmp006_TaskID, TMP006_READ_VOBJ_EVT, 1000 );
		return ( events ^ TMP006_READ_VOBJ_EVT );
	}
	
	if ( events & TMP006_READ_TAMB_EVT )
	{
		uint16 tamb = 0;
		cli();
		tamb = TMP006_readTAMBValue();
		sei();
		
		TMP006_SetParameter( TMP006_TAMB, sizeof( uint16 ) , &tamb );
		VOID osal_start_reload_timer( tmp006_TaskID, TMP006_READ_TAMB_EVT, 1000 );
		
		return ( events ^ TMP006_READ_TAMB_EVT );
	}
	
	if ( events & TMP006_POWER_OFF_EVT )
	{
		uint8 power_off = FALSE;
		
		TMP006_SetParameter( TMP006_POWER_CTRL, sizeof( uint8 ) , &power_off );
		
		P2_0 = 0;
		
		return ( events ^ TMP006_POWER_OFF_EVT );
	}
	
	//discard unknown events
	return 0;
}

static void TMP006_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
	switch ( pMsg->event )
	{
	    case KEY_CHANGE:
		TMP006_HandleKeys( ((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys );
		break;
	}
}

static void TMP006_HandleKeys( uint8 shift, uint8 keys )
{
	(void)shift;  // Intentionally unreferenced parameter
	
	if ( keys & HAL_KEY_SW_2 )
	{
		// if device is not in a connection, pressing the right key should toggle
		// advertising on and off
		if( gapProfileState != GAPROLE_CONNECTED )
		{
			uint8 current_adv_enabled_status;
			uint8 new_adv_enabled_status;
			
			//Find the current GAP advertisement status
			GAPRole_GetParameter( GAPROLE_ADVERT_ENABLED, &current_adv_enabled_status );
			
			if( current_adv_enabled_status == FALSE )
			{
				new_adv_enabled_status = TRUE;
			}
			else
			{
				new_adv_enabled_status = FALSE;
			}
			
			//change the GAP advertisement status to opposite of current status
			GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &new_adv_enabled_status );
		}
		
	}
}

static void peripheralStateNotificationCB( gaprole_States_t newState )
{
	if ( gapProfileState != newState )
	{
		switch( newState )
		{
		    case GAPROLE_STARTED:
			{
			}
			break;
			
			//if the state changed to connected, initially assume that keyfob is in range
		    case GAPROLE_ADVERTISING:
			{
			}
			break;
			
			//if the state changed to connected, initially assume that keyfob is in range      
		    case GAPROLE_CONNECTED:
			{
			}
			break;
			
		    case GAPROLE_WAITING:
			{
			}
			break;
			
		    case GAPROLE_WAITING_AFTER_TIMEOUT:
			{
			}
			break;
			
		    default:
			// do nothing
			break;
		}
	}
	
	gapProfileState = newState;
}

static void TMP006PowerChangeCB(uint8 ctrl)
{
	if ( ctrl == 0x30 )
	{
		VOID osal_stop_timerEx( tmp006_TaskID, TMP006_READ_VOBJ_EVT );
		VOID osal_clear_event( tmp006_TaskID, TMP006_READ_VOBJ_EVT );
		VOID osal_stop_timerEx( tmp006_TaskID, TMP006_READ_TAMB_EVT );
		VOID osal_clear_event( tmp006_TaskID, TMP006_READ_TAMB_EVT );
		
		VOID osal_set_event( tmp006_TaskID, TMP006_POWER_OFF_EVT );
	}
	else
	{
		VOID osal_set_event( tmp006_TaskID, TMP006_POWER_ON_EVT );
	}
}