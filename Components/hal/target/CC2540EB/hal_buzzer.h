#ifndef HAL_BUZZER_PERIPHERAL_H_
#define HAL_BUZZER_PERIPHERAL_H_

#include "hal_types.h"

#define BEEP_C6		1047
//#define BEEP_CS6		1109
#define BEEP_D6		1175
//#define BEEP_DS6		1245
#define BEEP_E6		1319
#define BEEP_F6		1397
//#define BEEP_FS6		1475
#define BEEP_G6		1568
//#define BEEP_GS6		1661
#define BEEP_A6		1760
//#define BEEP_AS6		1865
#define BEEP_B6		1976

#define BEEP_C7		2093
//#define BEEP_CS7		2218
#define BEEP_D7		2349
//#define BEEP_DS7		2489
#define BEEP_E7		2637
#define BEEP_F7		2794
#define BEEP_FS7		2960
#define BEEP_G7		3136
//#define BEEP_GS7		3322
#define BEEP_A7		3520
//#define BEEP_AS7		3729
#define BEEP_B7		3951

#define BUZZER_ALERT_HIGH_FREQ BEEP_A7

// Function prototypes
extern void buzzerInit(void);
extern uint8 buzzerStart(uint16 frequency);
extern void buzzerStop(void);
extern void Beep(uint32 msec_500,uint32 afterTerm,uint16 freq);
#endif