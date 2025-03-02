#include <xc.h>

void init_timer0(void)
{
    T0CS = 0;
    PSA = 1;
    TMR0 = 6;
    TMR0IE = 1;
    TMR0IF = 0;
}