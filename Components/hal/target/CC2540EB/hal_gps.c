#include "hal_mcu.h"
#include "hal_gps.h"

//#define UART_BAUD_M							216	// set baudrate to 115200bps
//#define UART_BAUD_E							11	// set buadrate to 115200bps

#define UART_BAUD_M							59	// set buadrate to 9600bps
#define UART_BAUD_E							8	// set baudrate to 9600bps

#define PERCFG_U1CFG_ALT1						0x00
#define PERCFG_U1CFG							0x01

uint8 GPSRxComplete;
GPS_T GPSData;

static uint8 GPSRingBuffer[120];
static uint8 pGPSIdx;

void HalGPSInit(void){
	
	PERCFG = (PERCFG & ~PERCFG_U1CFG) | PERCFG_U1CFG_ALT1;
	P0SEL |= 0x3C;
	P2_0 = 1;
	U1CSR = 0xC0;								//UART mode, Receiver enable
	U1UCR = 0x02;								//non flow control, parity none, 8bit transfer, low and 1 stopbit, low start bit
	U1GCR = (U1GCR & ~UART_BAUD_E) | UART_BAUD_E;			//baud rate of E
//	U1GCR = 0x08;								//baud rate of E
	U1BAUD = 59;								//baud rate of M - 9600 bps
	
	URX1IE = 1;									//RX interrupt enable
}

static uint8 HalGPSGetDataPosition(uint8 skip){
	
	uint8 position = 0;
	uint8 i = 0;
	
	for(position = 0; i < skip ; position++){
		
		if(GPSRingBuffer[position] == ',')
			i++;
	}
	
	if(GPSRingBuffer[position] == ',')
		position = 0xFF;
	
	return position;
}

static uint32 HalGPSReadFloatData(uint8 data_type,uint8 position){
	
	uint32 sum = 0;
	
	if(data_type == GPS_TYPE_DEFAULT){
		
		for(int i = 0;i < 15; i++){
			
			if((GPSRingBuffer[position] >= 0x30) && (GPSRingBuffer[position] <= 0x39))
			{
				sum = (sum * 10) + ((unsigned int)GPSRingBuffer[position] - 0x30);
			}
			else if(GPSRingBuffer[position] == '.');
			else if(GPSRingBuffer[position] == ',') break;
			position++;
		}
	}
	else if(data_type == GPS_TYPE_HDOP){
		for(int i = 0;i < 15; i++){
			
			if((GPSRingBuffer[position] >= 0x30) && (GPSRingBuffer[position] <= 0x39))
			{
				sum = (sum * 10) + ((unsigned int)GPSRingBuffer[position] - 0x30);
			}
			else if(GPSRingBuffer[position] == '.') break;
			else if(GPSRingBuffer[position] == ',') break;
			position++;
		}
	}
	else{
		uint32 sum = 0;
		uint8 pos = position;
		uint8 length = position + 5;
		
		for(int i = 0;i < 15; i++){
			if((GPSRingBuffer[pos] >= 0x30) && (GPSRingBuffer[pos] <= 0x39))
			{
				sum = (sum * 10) + ((unsigned int)GPSRingBuffer[pos] - 0x30);
			}
			else if(GPSRingBuffer[pos] == '.');
			else if(GPSRingBuffer[pos] == ',') break;
			pos++;
		}
		
		while(pos < length)
		{
			sum *= 10;
			pos++;
		}
	}
	
	return sum;
}

uint8 HalGPSDataParsing(void){
	
	uint8 position = 0;
	
	if(GPSRingBuffer[4] == 'G'){
		
		position = HalGPSGetDataPosition(GPS_UTC);
		
		if(position != 0xFF){
			GPSData.UTC_hh = ((GPSRingBuffer[position] - 0x30) * 10) + (GPSRingBuffer[position+1] - 0x30) + 9;
			GPSData.UTC_mm = ((GPSRingBuffer[position + 2] - 0x30) * 10) + (GPSRingBuffer[position+3] - 0x30);
			GPSData.UTC_ss = ((GPSRingBuffer[position + 4] - 0x30) * 10) + (GPSRingBuffer[position+5] - 0x30);
		}
		else{
			GPSData.UTC_hh = 0;
			GPSData.UTC_mm = 0;
			GPSData.UTC_ss = 0;
		}
		
		//calculate leap year
		if(GPSData.UTC_hh >= 24){
			
			GPSData.UTC_hh -= 24;
		}
		
		position = HalGPSGetDataPosition(GPS_LAT);
		
		if(position != 0xFF){
			GPSData.Latitude = HalGPSReadFloatData(GPS_TYPE_POSITION,position);
		}
		else
			GPSData.Latitude = 0;
		
		position = HalGPSGetDataPosition(GPS_NSI);
		
		if(position != 0xFF){
			if(GPSRingBuffer[position] == 'N')
				GPSData.Latitude &= 0x7FFFFFFF;
			else
				GPSData.Latitude |= 0x10000000;
		}
		
		position = HalGPSGetDataPosition(GPS_LONG);
		
		if(position != 0xFF){
			GPSData.Longtitude = HalGPSReadFloatData(GPS_TYPE_POSITION,position);
		}
		else
			GPSData.Longtitude = 0;
		
		position = HalGPSGetDataPosition(GPS_EWI);
		
		if(position != 0xFF){
			if(GPSRingBuffer[position] == 'E')
				GPSData.Longtitude &= 0x7FFFFFFF;
			else
				GPSData.Longtitude |= 0x10000000;
		}
		
		position = HalGPSGetDataPosition(GPS_SAT);
		
		if(position != 0xFF){
			GPSData.NbrOfSat = HalGPSReadFloatData(GPS_TYPE_DEFAULT,position);
		}
		else
			GPSData.NbrOfSat = 0;
		
		position = HalGPSGetDataPosition(GPS_HDOP);
		
		if(position != 0xFF){
			GPSData.HDOP = (uint8)HalGPSReadFloatData(GPS_TYPE_HDOP,position);
		}
		else
			GPSData.HDOP = 0xEF;
		
		return 1;
	}
	
	return 0;
}

void HalGPSBufferClear(void){
	
	uint8 i = 0;
	
	for(i=0;i<120;i++)
		GPSRingBuffer[i] = 0x00;
	
	pGPSIdx = 0;
}

void HalGPSPowerContrl(uint8 ctrl){
	
	if(ctrl == ON){
		
		P2_0 = 1;
		P1_0 = 1;
	}
	else if(ctrl == OFF){
		
		P2_0 = 0;
		P1_0 = 0;
	}
	else{
		/* shout get here */
	}
}

#pragma vector=URX1_VECTOR
__near_func __interrupt void URX1_ISR(void){
	
	char data;
	
	data = U1DBUF;
	
	U1DBUF = data;
	while(U1ACTIVE == 1);
	
	if(!GPSRxComplete){
		
		GPSRingBuffer[pGPSIdx++] = data;
		
		if(GPSRingBuffer[0] == '$')
			pGPSIdx++;
		else
			pGPSIdx = 0;
		
		if(GPSRingBuffer[pGPSIdx-1] == 0x0A){
			
			if(GPSRingBuffer[4] == 'G'){
				
				GPSRxComplete = SET;
			}
			else
				HalGPSBufferClear();
		}
	}
	else
		pGPSIdx = 0;
}