#include "hal_wcdma.h"

#include "hal_mcu.h"
#include "osal.h"
#include "OSAL_Clock.h"
#include "hal_gps.h"

#include <string.h>

#define UART_BAUD_M					216
#define UART_BAUD_E					11		// set baudrate to 115200bps

#define PERCFG_U0CFG_ALT1				0x00
#define PERCFG_U0CFG					0x01

static uint8 ModemRxBuffer[353];
static uint16 ModemWriteIdx;
static uint16 ModemReadIdx;

static uint8 CheckBuffer[50];
static uint8 CheckIdx;
static uint8 bootflag = 0;
static uint8 initialflag = 0;

uint8 PhoneNumber[12];
uint8 PrivateIP[4];
uint8 CellID[3];

static uint8 CommandEchoOff[6] = {'A','T','E','0','\r'};
static uint8 CommandGetNumber[9] = {'A','T','+','C','N','U','M','\r'};
static uint8 CommandGetCellID[13] = {'A','T','$','$','D','S','C','R','E','E','N','?','\r'};
static uint8 CommandSetAddr[37] = {'A','T','$','$','T','C','P','_','A','D','D','R','=','0',',','2','1','1',',','1','6','9',',','1','4','6',',','1','8','4',',','6','4','0','0','0','\r'};
static uint8 CommandPPPOpen[16] = {'A','T','$','$','T','C','P','_','P','P','P','O','P','\r'};
static uint8 CommandSockOpen[14] = {'A','T','$','$','U','D','P','_','S','C','O','P','\r'};

void HalWCDMAInit(void){
	
	PERCFG = (PERCFG & ~PERCFG_U0CFG) | PERCFG_U0CFG_ALT1;
	P0SEL |= 0x0C;
	P0DIR |= 0x0C;
	PERCFG = 0x00;
	
	/* UART0 peripheral initialize */
	U0CSR = 0xC0;
	U0UCR = 0x02;
	U0GCR = 0x0B;
	U0BAUD = 216;
	URX0IE = 1;
}

void HalWCDMAPowerControl(uint8 ctrl){
	
	if(ctrl == ON){
		
		P1_7 = 1;
	}
	else if(ctrl == OFF){
		
		P1_7 = 0;
	}
	else{
		/* Should get here */
	}
}

void HalSendATComms(uint8* cmd, uint8 length){
	
	uint8 i = 0;
	
	VOID osal_memset(ModemRxBuffer,0x00,353);
	RESET_IDX();
	
	for(i=0;i<length;i++){
		
		U0DBUF = cmd[i];
		while(U0ACTIVE == 1);
		
		if(cmd[i] == '\r'){
			break;
		}
	}
}

void HalResetModemBootFlag(void){
	initialflag = 0;
	bootflag = 0;
}
static uint8 HalGetRxHit(void){
	
	uint8 hit = RESET;
	
	if( ModemWriteIdx != ModemReadIdx)
		hit = SET;
	else
		hit = RESET;
	
	return hit;
}

static uint8 HalGetRxByte(void){
	
	uint8 byte = 0x00;
	
	byte = ModemRxBuffer[ModemReadIdx++];
	
	if(ModemReadIdx == 353)
		ModemReadIdx = 0;
	
	return byte;
}

static uint8 Halctoi(uint8 data){
	
	uint8 retval = 0;
	
	if(data > 0x40){
		
		retval = data - 0x41 + 0x0A;
	}
	else if(data > 0x2F && data < 0x3A){
		
		retval = data - 0x30;
	}
	
	return retval;
}

static uint8 HalGetChecksum(uint8* data, uint8 len){
	
	uint8 i = 0;
	uint8 checksum = 0;
	
	for(i=1;i<len;i++){
		
		checksum ^= data[i];
	}
	
	return checksum;
}

static void HalWcdmaDelay(void){
	
	volatile uint16 i = 0;
	
	for(i=0;i<3000;i++);
}
uint8 HalModemBootOn(void){
	
	uint8 complete = 0;
	
	while(HalGetRxHit() == 1){
		
		CheckBuffer[CheckIdx] = HalGetRxByte();
		
		U1DBUF = CheckBuffer[CheckIdx];
		while(U1ACTIVE == 1);
		
		if(CheckBuffer[CheckIdx] == '\r'){
			
			if(CheckIdx < 2){
				
				CheckIdx = 0;
				VOID osal_memset(CheckBuffer,0x00,30);
			}
			else{
				CheckIdx = 0;
				
				if(CheckBuffer[9] == '9' && CheckBuffer[10] == '0' && CheckBuffer[11] == '0'){
					bootflag = 1;
					VOID osal_memset(CheckBuffer,0x00,30);
				}
				else if(CheckBuffer[9] == '3' && CheckBuffer[10] == '5' && CheckBuffer[11] == '9' && bootflag == 1){
					initialflag = 1;
					VOID osal_memset(CheckBuffer,0x00,30);
				}
				else if(CheckBuffer[9] == '1' && CheckBuffer[10] == '0' && CheckBuffer[11] == '1' && initialflag == 1){
					complete = 1;
					VOID osal_memset(CheckBuffer,0x00,30);
				}
			}
		}
		else
			CheckIdx++;
	}
	
	return complete;
}

uint8 HalSetEchoMode(void){
	
	uint8 flag = 0;
	
	HalSendATComms(CommandEchoOff,sizeof(CommandEchoOff));
	HalWcdmaDelay();
	while(HalGetRxHit() == 1){
		
		CheckBuffer[CheckIdx] = HalGetRxByte();
		U1DBUF = CheckBuffer[CheckIdx];
		while(U1ACTIVE == 1);
		if(CheckBuffer[CheckIdx] == '\n'){
			
			if(CheckIdx < 2){
				
				CheckIdx = 0;
				VOID osal_memset(CheckBuffer,0x00,30);
			}
			else{
				CheckIdx = 0;
				if(CheckBuffer[0] == 'O' && CheckBuffer[1] == 'K'){
					flag = 1;
				}
				else{
					VOID osal_memset(CheckBuffer,0x00,30);
				}
			}
			
		}
		else
			CheckIdx++;
	}
	
	return flag;
}

uint8 HalGetPhoneNum(void){
	
	uint8 complete = FALSE;
	
	HalSendATComms(CommandGetNumber,sizeof(CommandGetNumber));
	HalWcdmaDelay();
	
	while(HalGetRxHit() == 1){
		
		CheckBuffer[CheckIdx] = HalGetRxByte();
		U1DBUF = CheckBuffer[CheckIdx];
		while(U1ACTIVE == 1);
		if(CheckBuffer[CheckIdx] == '\n'){
			
			if(CheckIdx < 2){
				
				CheckIdx = 0;
				VOID osal_memset(CheckBuffer,0x00,30);
			}
			else{
				CheckIdx = 0;
				if(CheckBuffer[0] == '+' && CheckBuffer[4] == 'M'){
					
					PhoneNumber[0] = '0';
					PhoneNumber[1] = CheckBuffer[25];
					PhoneNumber[2] = CheckBuffer[26];
					PhoneNumber[3] = CheckBuffer[27];
					PhoneNumber[4] = CheckBuffer[28];
					PhoneNumber[5] = CheckBuffer[29];
					PhoneNumber[6] = CheckBuffer[30];
					PhoneNumber[7] = CheckBuffer[31];
					PhoneNumber[8] = CheckBuffer[32];
					PhoneNumber[9] = CheckBuffer[33];
					PhoneNumber[10] = CheckBuffer[34];
					
					VOID osal_memset(CheckBuffer,0x00,30);
					complete = TRUE;
					VOID osal_memset(ModemRxBuffer,0x00,200);
					RESET_IDX();
				}
				else if(CheckBuffer[0] == 'O' && CheckBuffer[1] == 'K'){
					
					break;
				}
			}
			
		}
		else
			CheckIdx++;
	}
	
	return complete;
}

uint8 HalGetCellID(void){
	
	uint8 complete = 0;
	
	HalSendATComms(CommandGetCellID,sizeof(CommandGetCellID));
	HalWcdmaDelay();
	
	while(HalGetRxHit() == 1){
		
		CheckBuffer[CheckIdx] = HalGetRxByte();
		U1DBUF = CheckBuffer[CheckIdx];
		while(U1ACTIVE == 1);
		if(CheckBuffer[CheckIdx] == '\n'){
			
			if(CheckIdx < 2){
				
				CheckIdx = 0;
				VOID osal_memset(CheckBuffer,0x00,30);
			}
			else{
				if(CheckBuffer[0] == 'L' && CheckBuffer[2] == 'C'){
					
					CellID[0] += (Halctoi(CheckBuffer[31]) << 4);
					CellID[0] += Halctoi(CheckBuffer[32]);
					CellID[1] += (Halctoi(CheckBuffer[33]) << 4);
					CellID[1] += Halctoi(CheckBuffer[34]);
					CellID[2] += (Halctoi(CheckBuffer[35]) << 4);
					CellID[2] += Halctoi(CheckBuffer[36]);
					VOID osal_memset(CheckBuffer,0x00,30);
					complete = 1;
					break;
				}
				else if(CheckBuffer[0] == 'O' && CheckBuffer[1] == 'K'){
					break;
				}
				
				CheckIdx = 0;
			}
			
		}
		else
			CheckIdx++;
	}
	
	return complete;
}

uint8 HalSetServerAddr(void){
	
	uint8 complete = 0;
	
	HalSendATComms(CommandSetAddr,sizeof(CommandSetAddr));
	HalWcdmaDelay();
	
	while(HalGetRxHit() == 1){
		
		CheckBuffer[CheckIdx] = HalGetRxByte();
		U1DBUF = CheckBuffer[CheckIdx];
		while(U1ACTIVE == 1);
		if(CheckBuffer[CheckIdx] == '\n'){
			
			if(CheckIdx < 2){
				
				CheckIdx = 0;
				VOID osal_memset(CheckBuffer,0x00,30);
			}
			else{
				CheckIdx = 0;
				if(CheckBuffer[0] == 'O' && CheckBuffer[1] == 'K'){
					complete = 1;
					VOID osal_memset(CheckBuffer,0x00,30);
				}
				else{
					VOID osal_memset(CheckBuffer,0x00,30);
				}
			}
			
		}
		else
			CheckIdx++;
	}
	return complete;
}

uint8 HalOpenPPP(void){
	
	uint8 i = 0;
	uint8 j = 0;
	uint8 complete = 0;
	
	HalSendATComms(CommandPPPOpen,sizeof(CommandPPPOpen));
	HalWcdmaDelay();
	
	while(HalGetRxHit() == 1){
		
		CheckBuffer[CheckIdx] = HalGetRxByte();
		U1DBUF = CheckBuffer[CheckIdx];
		while(U1ACTIVE == 1);
		if(CheckBuffer[CheckIdx] == '\n'){
			
			if(CheckIdx < 2){
				
				CheckIdx = 0;
				VOID osal_memset(CheckBuffer,0x00,30);
			}
			else{
				CheckIdx = 0;
				if(CheckBuffer[8] == 'I' && CheckBuffer[9] == 'P'){
					VOID osal_memset(PrivateIP,0x00,4);
					/* get local ip */
					for(i=12;i<29;i++){
						if(CheckBuffer[i] == '.')
							j++;
						else if(CheckBuffer[i] == '\r')
							break;
						else
							PrivateIP[j] = (PrivateIP[j] * 10) + (CheckBuffer[i] - 0x30);
						
					}
					
					complete = 1;
					break;
				}
				else{
					VOID osal_memset(CheckBuffer,0x00,30);
				}
			}
			
		}
		else
			CheckIdx++;
	}
	
	return complete;
}

uint8 HalSocketOpen(void){
	
	uint8 complete = 0;
	
	HalSendATComms(CommandSockOpen,sizeof(CommandSockOpen));
	HalWcdmaDelay();
	
	while(HalGetRxHit() == 1){
		
		CheckBuffer[CheckIdx] = HalGetRxByte();
		U1DBUF = CheckBuffer[CheckIdx];
		while(U1ACTIVE == 1);
		if(CheckBuffer[CheckIdx] == '\n'){
			
			if(CheckIdx < 2){
				
				CheckIdx = 0;
				VOID osal_memset(CheckBuffer,0x00,30);
			}
			else{
				CheckIdx = 0;
				if(CheckBuffer[8] == '6' && CheckBuffer[9] == '3' && CheckBuffer[10] == '0'){
					complete = 1;
					break;
				}
				else{
					VOID osal_memset(CheckBuffer,0x00,30);
				}
			}
			
		}
		else
			CheckIdx++;
	}
	
	return complete;
}

void HalIPChange(uint8* addr){
	
	uint8 i = 0;
	uint8 j = 0;
	
	for(i=15;i<36;i++){
		
		CommandSetAddr[i] = 0x00;
	}
	
	for(i=15;i<36;i++){
		
		CommandSetAddr[i] = addr[j];
		j++;
	}
}

void makeSearchServerData(void){
	
	uint8 checksum = 0;
	uint8 i = 0;
	uint8 ServerData[28];
	
	ServerData[0] = '$';
	ServerData[1] = 'W';
	ServerData[2] = 'I';
	ServerData[3] = 'M';
	ServerData[4] = 'P';
	ServerData[5] = ',';
	ServerData[6] = (GPSData.Latitude >> 24) && 0xFF;
	ServerData[7] = (GPSData.Latitude >> 16) && 0xFF;
	ServerData[8] = (GPSData.Latitude >> 8) && 0xFF;
	ServerData[9] = (GPSData.Latitude >> 0) && 0xFF;
	ServerData[10] = ',';
	ServerData[11] = (GPSData.Longtitude >> 24) && 0xFF;
	ServerData[12] = (GPSData.Longtitude >> 16) && 0xFF;
	ServerData[13] = (GPSData.Longtitude >> 8) && 0xFF;
	ServerData[14] = (GPSData.Longtitude >> 0) && 0xFF;
	ServerData[15] = ',';
	ServerData[16] = GPSData.UTC_hh;
	ServerData[17] = GPSData.UTC_mm;
	ServerData[18] = GPSData.UTC_ss;
	ServerData[19] = ',';
	ServerData[20] = 0;
	ServerData[21] = ',';
	ServerData[22] = CellID[0];
	ServerData[23] = CellID[1];
	ServerData[24] = CellID[2];
	ServerData[25] = '*';
	
	checksum = HalGetChecksum(ServerData,25);
	
	if((checksum >> 4) > 0x09){
		
		ServerData[26] = (checksum >> 4)- 0x0a + 0x41;
	}
	else{
		ServerData[26] = (checksum >> 4)+ 0x30;
	}
	
	if((checksum & 0x0F) > 0x09){
		
		ServerData[27] = (checksum & 0x0F)- 0x0a + 0x41;
	}
	else{
		ServerData[27] = (checksum & 0x0F) + 0x30;
	}
	
	for(i=0;i<28;i++){
		U0DBUF = ServerData[i];
		while(U0ACTIVE == 1);
	}
}

void makeOKServerData(void){
	
	uint8 i = 0;
	uint8 ServerData[11];
	
	ServerData[0] = '$';
	ServerData[1] = 'W';
	ServerData[2] = 'I';
	ServerData[3] = 'M';
	ServerData[4] = 'P';
	ServerData[5] = ',';
	ServerData[6] = 'O';
	ServerData[7] = 'K';
	ServerData[8] = '*';
	ServerData[9] = '2';
	ServerData[10] = 'B';
	
	for(i=0;i<11;i++){
		U0DBUF = ServerData[i];
		while(U0ACTIVE == 1);
	}
}

#pragma vector=URX0_VECTOR
__near_func __interrupt void URX0_ISR(void){
	
	char data;
	
	data = U0DBUF;
	
	ModemRxBuffer[ModemWriteIdx++] = data;
	
	if(ModemWriteIdx == 353)
		ModemWriteIdx = 0;
}
