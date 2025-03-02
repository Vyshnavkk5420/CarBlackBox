/*
#include <xc.h>
#define PERIOD 1023

extern unsigned int dc; // Duty cycle
extern unsigned int tc; // Timer cycle (5 seconds)

void __interrupt() isr() 
{
    static unsigned int pc = 0;
    static unsigned int count = 0;
    
    if (TMR0IF) 
    { 
        TMR0 = TMR0 + 8; 

        if (pc < dc) 
        {
            PORTB = 0xFF;
        } 
        else 
        {
            PORTB = 0x00;
        }
        
        if (pc++ >= PERIOD) 
        {
            pc = 0;
        }

        if (count++ >= 20000) 
        {
            count = 0;
            if (tc > 0) 
            {
                tc--;
            }
        }
        TMR0IF = 0; 
    }
    
} 
*/

