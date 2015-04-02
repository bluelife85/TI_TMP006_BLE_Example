#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include "profile_tmp006.h"

#define TMP006_ATTRIBUTE_SUPPORT					9

CONST uint8 tmp006ServUUID[ATT_BT_UUID_SIZE] = 
{
	LO_UINT16(TMP006_SERVICE_UUID), HI_UINT16(TMP006_SERVICE_UUID)
};

CONST uint8 tmp006PwrCtrlUUID[ATT_BT_UUID_SIZE] = 
{
	LO_UINT16(TMP006_POWER_CTRL_UUID), HI_UINT16(TMP006_POWER_CTRL_UUID)
};

CONST uint8 tmp006VOBJ_UUID[ATT_BT_UUID_SIZE] = 
{
	LO_UINT16(TMP006_VOBJ_UUID), HI_UINT16(TMP006_VOBJ_UUID)
};

CONST uint8 tmp006TAMB_UUID[ATT_BT_UUID_SIZE] = 
{
	LO_UINT16(TMP006_TAMB_UUID), HI_UINT16(TMP006_TAMB_UUID)
};

static CONST gattAttrType_t tmp006Service = { ATT_BT_UUID_SIZE, tmp006ServUUID };

static uint8 tmp006PwrCtrlProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 tmp006PwrCtrl = FALSE;

static uint8 tmp006VOBJProps = GATT_PROP_NOTIFY;
static uint8 tmp006VOBJ[2];

static uint8 tmp006TAMBProps = GATT_PROP_NOTIFY;
static uint8 tmp006TAMB[2];

static gattCharCfg_t tmp006VOBJConfig[GATT_MAX_NUM_CONN];
static gattCharCfg_t tmp006TAMBConfig[GATT_MAX_NUM_CONN];

static gattAttribute_t tmp006AttrTbl[TMP006_ATTRIBUTE_SUPPORT] = 
{
	{
		{ ATT_BT_UUID_SIZE, primaryServiceUUID },
		GATT_PERMIT_READ,
		0,
		(uint8*)&tmp006Service
	},
	
	{
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ,
		0,
		&tmp006PwrCtrlProps
	},
	
	{
		{ ATT_BT_UUID_SIZE, tmp006PwrCtrlUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE,
		0,
		&tmp006PwrCtrl
	},
	
	{
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ,
		0,
		&tmp006VOBJProps
	},
	
	{
		{ ATT_BT_UUID_SIZE, tmp006VOBJ_UUID },
		0,
		0,
		tmp006VOBJ
	},
	
	{
		{ ATT_BT_UUID_SIZE, clientCharCfgUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE,
		0,
		(uint8*)tmp006VOBJConfig
	},
	
	{
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ,
		0,
		&tmp006TAMBProps
	},
	
	{
		{ ATT_BT_UUID_SIZE, tmp006TAMB_UUID },
		0,
		0,
		tmp006TAMB
	},
	
	{
		{ ATT_BT_UUID_SIZE, clientCharCfgUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE,
		0,
		(uint8*)tmp006TAMBConfig
	},
};

static uint8 tmp006_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
			       uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static uint8 tmp006_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
				uint8 *pValue, uint8 len, uint16 offset );
static void tmp006_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

CONST gattServiceCBs_t tmp006CBs = 
{
	tmp006_ReadAttrCB,
	tmp006_WriteAttrCB,
	NULL
};

static CONST TMP006CB_t* appCB;

bStatus_t TMP006_AddService( uint32 services )
{
	bStatus_t ret = SUCCESS;
	GATTServApp_InitCharCfg( INVALID_CONNHANDLE, tmp006VOBJConfig );
	GATTServApp_InitCharCfg( INVALID_CONNHANDLE, tmp006TAMBConfig );
	
	VOID linkDB_Register( tmp006_HandleConnStatusCB );
	
	if ( services & TMP006_SERVICE )
	{
		ret = GATTServApp_RegisterService( tmp006AttrTbl, 
						  GATT_NUM_ATTRS( tmp006AttrTbl ),
						  &tmp006CBs );
	}
	
	return ret;
}

bStatus_t TMP006_RegistAppCBs( TMP006CB_t* cb )
{
	if ( cb )
	{
		appCB = cb;
		return ( SUCCESS );
	}
	else
	{
		return ( bleAlreadyInRequestedMode );
	}
}

bStatus_t TMP006_SetParameter( uint8 param, uint8 len, void* pValue )
{
	bStatus_t ret = SUCCESS;
	
	switch(param)
	{
	    case TMP006_POWER_CTRL:
		if( len == sizeof (uint8) )
		{
			tmp006PwrCtrl = *((uint8*)pValue);
		}
		else
		{
			ret = bleInvalidRange;
		}
		break;
	    case TMP006_VOBJ:
		if( len == sizeof (uint16) )
		{
			VOID osal_memcpy( tmp006VOBJ, pValue, 2 );
			GATTServApp_ProcessCharCfg( tmp006VOBJConfig, tmp006VOBJ, FALSE, 
						   tmp006AttrTbl, GATT_NUM_ATTRS( tmp006AttrTbl ),
						   INVALID_TASK_ID );
		}
		else
		{
			ret = bleInvalidRange;
		}
		break;
	    case TMP006_TAMB:
		if( len == sizeof (uint16) )
		{
			VOID osal_memcpy( tmp006TAMB, pValue, 2 );
			GATTServApp_ProcessCharCfg( tmp006TAMBConfig, tmp006TAMB, FALSE, 
						   tmp006AttrTbl, GATT_NUM_ATTRS( tmp006AttrTbl ),
						   INVALID_TASK_ID );
		}
		else
		{
			ret = bleInvalidRange;
		}
		break;
	    default:
		ret = INVALIDPARAMETER;
		break;
	}
	
	return ( ret );
}

static uint8 tmp006_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
			       uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
	uint8 status = SUCCESS;
	
	if ( offset > 0 )
	{
		return ( ATT_ERR_ATTR_NOT_LONG );
	}
	
	if ( pAttr->type.len == ATT_BT_UUID_SIZE )
	{
		uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1] );
		
		switch(uuid)
		{
		    case TMP006_POWER_CTRL_UUID:
			*pLen = 1;
			pValue[0] = *pAttr->pValue;
			break;
		    default:
			*pLen = 0;
			status = ATT_ERR_ATTR_NOT_FOUND;
			break;
		}
	}
	else
	{
		*pLen = 0;
		status = ATT_ERR_INVALID_HANDLE;
	}
	
	return ( status );
}

static uint8 tmp006_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
				uint8 *pValue, uint8 len, uint16 offset )
{
	uint8 status = SUCCESS;
	uint8 notify = 0xff;
	
	if ( pAttr->type.len == ATT_BT_UUID_SIZE )
	{
		uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1] );
		
		switch(uuid)
		{
		    case TMP006_POWER_CTRL_UUID:
			*pValue = *pAttr->pValue;
			notify = 1;
			break;
		    case GATT_CLIENT_CHAR_CFG_UUID:
			status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
								offset, GATT_CLIENT_CFG_NOTIFY );
			break;
		    default:
			status = ATT_ERR_ATTR_NOT_FOUND;
			break;
		}
	}
	else
	{
		status = ATT_ERR_INVALID_HANDLE;
	}
	
	if ( (notify != 0xFF) && appCB && appCB->pfPower )
	{
		appCB->pfPower(pValue[0]);
	}
	
	return ( status );
}

static void tmp006_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{
	if ( connHandle != LOOPBACK_CONNHANDLE )
	{
		// Reset Client Char Config if connection has dropped
		if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
		    ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
		     ( !linkDB_Up( connHandle ) ) ) )
		{ 
			GATTServApp_InitCharCfg( connHandle, tmp006VOBJConfig );
			GATTServApp_InitCharCfg( connHandle, tmp006TAMBConfig );
		}
	}
}
