#include "hal_mcu.h"
#include "hal_buzzer.h"

void buzzerInit(void)
{
    // Buzzer connected at P1_6
    // We will use Timer 3 Channel 0 at alternate location 2
    // Channel 0 will toggle on compare with 0 and counter will
    // count in up/down mode to T3CC0.

    PERCFG |= 0x20;             // Timer 3 Alternate location 2
    P1DIR |= 0x40;              // P1_6 = output
    P1SEL |= 0x40;              // Peripheral function on P1_6

    T3CTL &= ~0x10;             // Stop timer 3 (if it was running)
    T3CTL |= 0x04;              // Clear timer 3
    T3CTL &= ~0x08;             // Disable Timer 3 overflow interrupts
    T3CTL |= 0x03;              // Timer 3 mode = 3 - Up/Down

    T3CCTL0 &= ~0x40;           // Disable channel 0 interrupts
    T3CCTL0 |= 0x04;            // Ch0 mode = compare
    T3CCTL0 |= 0x10;            // Ch0 output compare mode = toggle on compare
}

uint8 buzzerStart(uint16 frequency)
{
    buzzerInit();
  
    uint8 prescaler = 0;

    // Get current Timer tick divisor setting
    uint8 tickSpdDiv = (CLKCONSTA & 0x38)>>3;

    // Check if frequency too low
    if (frequency < (244 >> tickSpdDiv)){   // 244 Hz = 32MHz / 256 (8bit counter) / 4 (up/down counter and toggle on compare) / 128 (max timer prescaler)
        buzzerStop();                       // A lower tick speed will lower this number accordingly.
        return 0;
    }

    // Calculate nr of ticks required to achieve target frequency
    uint32 ticks = (8000000/frequency) >> tickSpdDiv;      // 8000000 = 32M / 4;

    // Fit this into an 8bit counter using the timer prescaler
    while ((ticks & 0xFFFFFF00) != 0)
    {
        ticks >>= 1;
        prescaler += 32;
    }

    // Update registers
    T3CTL &= ~0xE0;
    T3CTL |= prescaler;
    T3CC0 = (uint8)ticks;

    // Start timer
    T3CTL |= 0x10;
    
    return 1;
}

void buzzerStop(void)
{
    T3CTL &= ~0x10;             // Stop timer 3
    P1SEL &= ~0x40;
    P1_6 = 0;
}

void Beep(uint32 msec_500,uint32 afterTerm,uint16 freq){
	
	uint32 time = 7500 * msec_500;
	uint32 aftertime = 7500 * afterTerm;
		
	buzzerStart(freq);
	
	for(volatile uint32 i = 0;i<time;i++);
	
	buzzerStop();
    
    	for(volatile uint32 i = 0;i<aftertime;i++);
}
