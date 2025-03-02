/*
 * NAME: VISHNUKUMAR D
 * DATE: 16-12-2024
 * PROJECT - CAR BLACK BOX.
 */

#include <xc.h>
#include "clcd.h"
#include "keypad.h"
#include "adc.h"
#include "eeprom.h"
#include "i2c.h"
#include "ds1307.h"
#include "uart.h"

typedef enum {
    DASH_BOARD,
    MAIN_MENU,
    VIEW_LOG,
    CLEAR_LOG,
    DOWNLOAD_LOG,
    SET_TIME
} Status;

// Function Declarations
void dashboard(void);
void mainmenu(void);
void view_log(void);
void clear_log(void);
void download_log(void);
void set_time(void);
void store_events(void);
static void get_time(void);
void print_clcd(void);

// MAIN_MENU
Status state = DASH_BOARD;
unsigned char menu_cursor = 0;
unsigned char menu_items = 4;
unsigned char key = 0; 
unsigned int scroll; // down = 0, up = 1

// DASH_BOARD
unsigned short adc_reg_val;
unsigned int result = 0;
unsigned char time[9];
char gear[][3] = {"ON", "GN", "GR", "G1", "G2", "G3", "G4"};
int collision_flag = 0;
int gear_count = 0;

// Store_Events
unsigned int address = 0x00;
unsigned int event_count = 0;
unsigned int shift_index = 0;

// VIEW_LOG
char event[10][16];
unsigned int view_addr = 0x00;
unsigned int display_index = 0;     

// GET_TIME
unsigned char clock_reg[3];

// SET_TIME
static unsigned char field_flag = 0;
unsigned char hour, min, sec;
unsigned int blink = 0; 

// Initializes the required peripherals
void init_config(void)
{
    init_clcd();            
    init_mkp();             
    init_adc();   
    init_i2c();
	init_ds1307();
    init_uart();
}

void main(void)
{  
    init_config();
    
    while(1)
    {
        get_time(); // Continuously update the system time using RTC.
        key = read_matrix_keypad(STATE);

        switch (state)  // Finite State Machine to handle transitions.
        {
            case DASH_BOARD:
                dashboard();
                if (key == SW1)
                {
                collision_flag = 1; 
                store_events(); 
                }
                else if (key == SW2)
                {
                    if (collision_flag)
                    {
                        collision_flag = 0; 
                        gear_count = 1;  
                    }
                    else if (gear_count < 6)
                    {
                        gear_count++; 
                    }
                    store_events(); 
                }
                else if (key == SW3)
                {
                    if (collision_flag)
                    {
                        collision_flag = 0; 
                        gear_count = 1;    
                    }
                    else if (gear_count > 1)
                    {
                        gear_count--; 
                    }
                    store_events(); 
                }
                else if (key == SW4) 
                { 
                    CLEAR_DISP_SCREEN;
                    state = MAIN_MENU;
                    menu_cursor = 0;
                }
                break;

            case MAIN_MENU:  
                mainmenu();
                if (key == SW1) 
                {  
                    scroll = 1;
                    if (menu_cursor > 0)
                    {
                        --menu_cursor;
                    }    
                } 
                else if (key == SW2) 
                {  
                    scroll = 0;
                    if (menu_cursor < 3)
                    {
                        ++menu_cursor;
                    }
                } 
                else if (key == SW4) 
                {  
                    switch (menu_cursor) 
                    {
                        case 0: 
                            CLEAR_DISP_SCREEN;
                            state = VIEW_LOG; 
                            break;
                        case 1: 
                            CLEAR_DISP_SCREEN;
                            state = CLEAR_LOG; 
                            break;
                        case 2:
                            CLEAR_DISP_SCREEN;
                            state = DOWNLOAD_LOG;
                            break;
                        case 3: 
                            CLEAR_DISP_SCREEN;
                            state = SET_TIME; 
                            break;
                    }
                } 
                else if (key == SW5) 
                {  
                    CLEAR_DISP_SCREEN;
                    state = DASH_BOARD;
                }
                break;

            case VIEW_LOG:
                view_log();  
                if (key == SW5)
                {    
                    state = MAIN_MENU;
                    CLEAR_DISP_SCREEN;
                }
                break;
         
            case CLEAR_LOG:
                clear_log();
                break;

            case SET_TIME: 
            set_time();
            if (key == SW1)
            {
                if (field_flag == 1) 
                {
                    hour = (hour + 1) % 24; 
                }
                else if (field_flag == 2) 
                {
                    min = (min + 1) % 60; 
                }
                else if (field_flag == 3) 
                {
                    sec = (sec + 1) % 60; 
                }
            }
            else if (key == SW2) 
            {
                if (++field_flag == 4) 
                {
                    field_flag = 1; 
                }
            }
            else if (key == SW4) 
            {
                write_ds1307(HOUR_ADDR, ((hour / 10) << 4) | (hour % 10)); 
                write_ds1307(MIN_ADDR, ((min / 10) << 4) | (min % 10));
                write_ds1307(SEC_ADDR, ((sec / 10) << 4) | (sec % 10));
                get_time(); 
                field_flag = 0;
                state = MAIN_MENU;
                CLEAR_DISP_SCREEN;
            }
            else if (key == SW5) 
            {    
                field_flag = 0;
                state = MAIN_MENU;
                CLEAR_DISP_SCREEN;
            }    
            break;


            case DOWNLOAD_LOG:
                download_log();
                if (key == SW5) 
                {    
                    state = MAIN_MENU;
                    CLEAR_DISP_SCREEN;
                }    
                break;    
        }
    }    
}

// Displays real-time data like time, gear status, and speed.
void dashboard(void) 
{ 
    clcd_print("  TIME     EV SP", LINE1(0));
    clcd_print(time, LINE2(0)); // fetch and print system time.

    // Update gear status based on collision flag and gear count.
    if (collision_flag)
    {
        clcd_putch('-', LINE2(11));
        clcd_putch('C', LINE2(12));
    }
    else
    {
        if (gear_count >= 0 && gear_count < 7)
        {
            clcd_print(gear[gear_count], LINE2(11));
        }
    }
    
    // Calculate speed using ADC input and display it.
    adc_reg_val = read_adc(CHANNEL4);
    result = (adc_reg_val / 10.33); 
    clcd_putch((result / 10) + '0', LINE2(14)); 
    clcd_putch((result % 10) + '0', LINE2(15)); 
}

// Manages menu navigation and item selection, displays menu based on the menu cursor.
void mainmenu(void) 
{
    if ((menu_cursor == 0) || (menu_cursor == 1 && !scroll))    
    {    
        clcd_print((menu_cursor == 0) ? "* VIEW LOG      " : "  VIEW LOG      ", LINE1(0));
        clcd_print((menu_cursor == 1) ? "* CLEAR LOG     " : "  CLEAR LOG     ", LINE2(0));
    }
    else if ((menu_cursor == 2 && !scroll) || (menu_cursor == 1))   
    {
        clcd_print((menu_cursor == 1) ? "* CLEAR LOG     " : "  CLEAR LOG     ", LINE1(0));
        clcd_print((menu_cursor == 2) ? "* DOWNLOAD LOG " : "  DOWNLOAD LOG ", LINE2(0));
    }
    else  
    {    
        clcd_print((menu_cursor == 2) ? "* DOWNLOAD LOG " : "  DOWNLOAD LOG   ", LINE1(0));
        clcd_print((menu_cursor == 3) ? "* SET TIME     " : "  SET TIME      ", LINE2(0));
    }
}

// Stores system events in External EEPROM
void store_events(void)
{
    if (address >= 100) 
    {
        address = 0; 
    }
    
    // Writes timestamp
    write_external_eeprom(address++, time[0]); 
    write_external_eeprom(address++, time[1]); 
    write_external_eeprom(address++, time[3]); 
    write_external_eeprom(address++, time[4]);
    write_external_eeprom(address++, time[6]); 
    write_external_eeprom(address++, time[7]); 
    
    // Writes collision or gears
    if (collision_flag) 
    {
        write_external_eeprom(address++, '-');
        write_external_eeprom(address++, 'C'); 
    } 
    else 
    {
        write_external_eeprom(address++, gear[gear_count][0]); 
        write_external_eeprom(address++, gear[gear_count][1]); 
    }
    
    // Writes speed
    write_external_eeprom(address++, (result / 10) + '0'); 
    write_external_eeprom(address++, (result % 10) + '0'); 
    
    
    // Manages event count and shift older events if the buffer is full.
    if (event_count < 10)
    {
        event_count++;
    }
    else
    {    
        shift_index = (shift_index + 1) % 10;
    }    
}

// Retrieves and displays logs from External EEPROM 
void view_log(void)
{
    if (event_count == 0) 
    {
        state = MAIN_MENU;
        clcd_print("EVENTS NOT     ", LINE1(0));
        clcd_print("        STORED ", LINE2(0));
        for(unsigned long wait = 500000; wait--; );
    }
    else
    {
        for (int i = 0; i < event_count; i++)
        {
            int current_event = (shift_index + i) % 10;
            view_addr = current_event * 10;

            event[i][0] = read_external_eeprom(view_addr++);
            event[i][1] = read_external_eeprom(view_addr++);
            event[i][2] = ':';
            event[i][3] = read_external_eeprom(view_addr++);
            event[i][4] = read_external_eeprom(view_addr++);
            event[i][5] = ':';
            event[i][6] = read_external_eeprom(view_addr++);
            event[i][7] = read_external_eeprom(view_addr++);
            event[i][8] = ' ';
            event[i][9] = read_external_eeprom(view_addr++);
            event[i][10] = read_external_eeprom(view_addr++);
            event[i][11] = ' ';
            event[i][12] = read_external_eeprom(view_addr++);
            event[i][13] = read_external_eeprom(view_addr++);
            event[i][14] = '\0'; 
        }

       
        clcd_print("               ", LINE2(0));
        
        // Handle navigations b/w logs using SW1 and SW2
        while (1)
        {
            clcd_print("# time     EV SP", LINE1(0));
            key = read_matrix_keypad(STATE);
            if (key == SW1 && display_index > 0) 
            {
                display_index--;     
            }
        
            else if (key == SW2 && display_index < event_count - 1) 
            {
                display_index++;
            }
        
            else if (key == SW5) 
            {
                display_index = 0;
                state = MAIN_MENU;
                CLEAR_DISP_SCREEN;
                break;
            }
        
            clcd_putch(display_index + '0', LINE2(0)); 
            clcd_print(event[display_index], LINE2(2));
        }
    }
}

// Clear all stored logs, resets counters and addresses for events in EEPROM.
void clear_log(void)
{
    event_count = 0;
    shift_index = 0;
    address = 0;
    view_addr = 0;
    clcd_print("Logs are Cleared", LINE1(0));
    clcd_print("                 ", LINE2(0));
    for (unsigned long wait = 500000; wait--; );
    CLEAR_DISP_SCREEN;
    state = MAIN_MENU;
}

// Transfers logs via UART.
void download_log(void)
{
    if (event_count == 0) 
    {
        clcd_print("NO LOGS       ", LINE1(0));
        clcd_print("    ARE STORED", LINE2(0));
        for (unsigned long wait = 500000; wait--; );
        state = MAIN_MENU;
        CLEAR_DISP_SCREEN;
        return;
    }

    clcd_print("DOWNLOADING    ", LINE1(0));
    clcd_print("     LOGS....  ", LINE2(0));

    // Read events from EEPROM and sends them to UART.
    for (int i = 0; i < event_count; i++)
    {
        int current_event = (shift_index + i) % 10;
        view_addr = current_event * 10;

        char event_log[16];
        event_log[0] = read_external_eeprom(view_addr++);
        event_log[1] = read_external_eeprom(view_addr++);
        event_log[2] = ':';
        event_log[3] = read_external_eeprom(view_addr++);
        event_log[4] = read_external_eeprom(view_addr++);
        event_log[5] = ':';
        event_log[6] = read_external_eeprom(view_addr++);
        event_log[7] = read_external_eeprom(view_addr++);
        event_log[8] = ' ';
        event_log[9] = read_external_eeprom(view_addr++);
        event_log[10] = read_external_eeprom(view_addr++);
        event_log[11] = ' ';
        event_log[12] = read_external_eeprom(view_addr++);
        event_log[13] = read_external_eeprom(view_addr++);
        event_log[14] = '\r';  
        event_log[15] = '\n';  


        for (int j = 0; j < 16; j++)
        {
            putch(event_log[j]);
        }
    }
    
    for (unsigned long wait = 500000; wait--; );
    state = MAIN_MENU;
    CLEAR_DISP_SCREEN;
}

// Allows manual RTC time setting.
void set_time(void) 
{
    clcd_print("    HH:MM:SS    ", LINE1(0)); 

    clcd_print("    ", LINE2(0));
    clcd_print("    ", LINE2(12));
    if (field_flag == 0) 
    {
        hour = ((time[0] - '0')*10) + (time[1] - '0');
        min = ((time[3] - '0')*10) + (time[4] - '0');
        sec = ((time[6] - '0')*10) + (time[7] - '0');
        field_flag++;
    }
    
    if (field_flag == 1) 
    {
        if (blink++ <= 250) 
        {
            print_clcd();
        } 
        else if (blink++ <= 500) 
        {
            clcd_print("  ", LINE2(4));
        } 
        else 
        {
            blink = 0;
        }
    }
    if (field_flag == 2) 
    {
        if (blink++ <= 250) 
        {
            print_clcd();
        } 
        else if (blink++ <= 500) 
        {
            clcd_print("  ", LINE2(7));
        } 
        else 
        {
            blink = 0;
        }
    }
    if (field_flag == 3) 
    {
        if (blink++ <= 250) 
        {
            print_clcd();
        } 
        else if (blink++ <= 500) 
        {
            clcd_print("  ", LINE2(10));
        } 
        else 
        {
            blink = 0;
        }
    }
}
 
void print_clcd(void) 
{
    clcd_putch(hour / 10 + '0', LINE2(4));
    clcd_putch(hour % 10 + '0', LINE2(5));
    clcd_putch(':', LINE2(6));

    clcd_putch(min / 10 + '0', LINE2(7));
    clcd_putch(min % 10 + '0', LINE2(8));
    clcd_putch(':', LINE2(9));

    clcd_putch(sec / 10 + '0', LINE2(10));
    clcd_putch(sec % 10 + '0', LINE2(11));
}

static void get_time(void)
{
	clock_reg[0] = read_ds1307(HOUR_ADDR);
	clock_reg[1] = read_ds1307(MIN_ADDR);
	clock_reg[2] = read_ds1307(SEC_ADDR);

	if (clock_reg[0] & 0x40)
	{
		time[0] = '0' + ((clock_reg[0] >> 4) & 0x01);
		time[1] = '0' + (clock_reg[0] & 0x0F);
	}
	else
	{
		time[0] = '0' + ((clock_reg[0] >> 4) & 0x03);
		time[1] = '0' + (clock_reg[0] & 0x0F);
	}
	time[2] = ':';
	time[3] = '0' + ((clock_reg[1] >> 4) & 0x0F);
	time[4] = '0' + (clock_reg[1] & 0x0F);
	time[5] = ':';
	time[6] = '0' + ((clock_reg[2] >> 4) & 0x0F);
	time[7] = '0' + (clock_reg[2] & 0x0F);
	time[8] = '\0';
}
