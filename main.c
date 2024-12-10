/*==============================================================================
 *Group 17 - Project 1
 * Author: Tien Nguyen - Justin Diaz
 * CPEG222
 * Fall 2024
 * University of Delaware
 * Summary of Project: This project utilizes 1 button and 2 switches. If the button R is pressed, the board switches from one mode to the other(total of 4 modes).
 * switch 7 control's the direction (if high the leds shift left, if low the leds shift right). switch 6 control's the amount of led's being ' '
 * shifted (if high 2 led's shift at a time, if low 1 led shifts at a time).
==============================================================================*/
/*---- Board system settings. PLEASE DO NOT MODIFY THIS PART FOR PROJECT 1 ---*/
#ifndef _SUPPRESS_PLIB_WARNING //suppress the plib warning during compiling
#define _SUPPRESS_PLIB_WARNING
#endif
#pragma config FPLLIDIV = DIV_2 // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20 // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_1 // System PLL Output Clock Divider
//(PLL Divide by 1)
#pragma config FNOSC = PRIPLL // Oscillator Selection Bits (Primary Osc w/PLL
//(XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF // Secondary Oscillator Enable (Disabled)
#pragma config POSCMOD = XT // Primary Oscillator Configuration (XT osc mode)
#pragma config JTAGEN = OFF

/*----------------------------------------------------------------------------*/
#include <xc.h> //Microchip XC processor header which links to the
//PIC32MX370512L header
#include <sys/attribs.h>
#include "config.h" // Basys MX3 configuration header
#include "lcd.h" // NOTE: utils.c and utils.h must also be in your project
//to use lcd.c
#include"ssd.h"
#include"mic.h"


/* ----------------------- Custom types ------------------------------------- */
enum mode{MODE1, MODE2, MODE3, MODE4};
/* ----------------------- Prototypes ----------------------------------------*/

void initialize_ports();
void initialize_output_states();
void handle_button_presses();
void delay_ms(int milliseconds);
void logic_mode_one();
void logic_mode_two();
void logic_mode_three();
void logic_mode_four();
void timer2_init();
void SWT_Init();

void logic_button_presses(enum mode *modePtr);
/* ------------------------ Constant Definitions ---------------------------- */
#define CLK (10000000)
#define SYS_FREQ (80000000L) // 80MHz system clock
#define _80Mhz_ (80000000L)
#define LOOPS_NEEDED_TO_DELAY_ONE_MS_AT_80MHz 1426
#define LOOPS_NEEDED_TO_DELAY_ONE_MS (LOOPS_NEEDED_TO_DELAY_ONE_MS_AT_80MHz * (SYS_FREQ / _80Mhz_))
/* The Basys reference manual shows to which pin of the processor every IO 
connects. 
 BtnC connects to Port F pin 0. PORTF reads output values from Port F pins.
 LATF would be used to read input values on Port F pins and TRISF would be used to
 set tristate values of Port F pins. We will see LAT and TRIS later. */
#define TRUE 1
#define FALSE 0
#define BUTTON_DEBOUNCE_DELAY_MS 20
#define BUTTON_RIGHT PORTBbits.RB8
#define sw7 PORTBbits.RB9
#define sw6 PORTBbits.RB10
#define MAX_BITS 8


/* -------------------- Global Variable Declarations ------------------------ */
char buttonsLocked = FALSE;
char pressedUnlockedBtnR = FALSE;
/* ----------------------------- Main --------------------------------------- */
int main(void)
{
    /*-------------------- Port and State Initialization ---------------------*/
    initialize_ports();
    initialize_output_states();
    timer2_init();
    enum mode current_mode = MODE1;


    while (TRUE)
    {
        /*-------------------- Main logic and actions start ------------------*/
        handle_button_presses();
        if (pressedUnlockedBtnR || current_mode == MODE2 || current_mode == MODE4) // Actions when BTNR is pressed
        {
            logic_button_presses(&current_mode);
        }
        switch (current_mode){
                case MODE1:
                    logic_mode_one();
                    break;
                case MODE2:
                    logic_mode_two();
                    break;
                case MODE3:
                    logic_mode_three();
                    break;
                case MODE4:
                    logic_mode_four();

                    break;        
        }
        /*--------------------------------------------------------------------*/
    }
}
/* ---------------------- Function Definitions ------------------------------ */
void timer2_init() {
    T2CONbits.ON = 0;       // Turn off Timer 2 during configuration
    T2CONbits.TCKPS = 0b111; // Prescaler 1:256
    T2CONbits.TCS = 0;      // Use internal PBCLK
    TMR2 = 0;               // Clear Timer 2 register
    PR2 = (1 * CLK) / (256 * 2); // Adjusted for 2 Hz
    IPC2bits.T2IP = 4;      // Interrupt priority
    IPC2bits.T2IS = 0;      // Interrupt subpriority
    IFS0bits.T2IF = 0;      // Clear interrupt flag
    IEC0bits.T2IE = 1;      // Enable Timer 2 interrupt
    T2CONbits.ON = 1;       // Turn on Timer 2
}
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) Timer2ISR(void) {
    
    IFS0bits.T2IF = 0; // Clear Timer 2 interrupt flag
    if (sw6) {
        // If SW6 = 1, increase the frequency (e.g., set PR2 for 4 Hz)
        PR2 = (1 * CLK) / (256 * 4);  // Adjust for higher frequency
    } else {
        // Default frequency (e.g., 1 Hz)
        PR2 = (1 * CLK) / (256 * 1);
    }
}

void initialize_ports()
{
    // Required to use Pin RA0 (connected to LED 0) as IO
    DDPCONbits.JTAGEN = 0; 
    
    /* 
    The following line sets the tristate of Port A bits 0-7 to 0. The LEDs are 
    connected to those pins. When the tristate of a pin is set low, the pin is 
    configured as a digital output. Notice an &= is used in conjunction with
    leading 1s (the FF) in order to keep the other bits of Port A (8-15) in
    their current state. 
    */
    TRISA &= 0xFF00;
    
    // Configure BTNR
    TRISBbits.TRISB8 = 1; // RB8 (BTNR) configured as input
    ANSELBbits.ANSB8 = 0; // RB8 (BTNR) disabled analog
    
// Initialize SW6
TRISBbits.TRISB10 = 1; // RB8 (BTNR) configured as input
ANSELBbits.ANSB10 = 0; // RB8 (BTNR) disabled analog

// Initialize SW7
TRISBbits.TRISB9 = 1; // RB8 (BTNR) configured as input
ANSELBbits.ANSB9 = 0; // RB8 (BTNR) disabled analog

    SWT_Init();
    LCD_Init(); // A library function provided by Digilent
}
void initialize_output_states()
{
    /* The following line sets the latch values of Port A bits 0-7 to 0.
     *  The LEDs are connected to those pins. When the latch of an LED output
     *  pin is set low, the LED is turned off. Notice we again used an &= in
     *  conjunction with leading 1s in order to keep the other latch values of
     *  Port A (bits 8-15) in their current state. */
    LATA &= 0xFF00;    
    
    /* Display "Group #1" at line 0 position 0, using spaces to center it and
     * clear any previously displayed letters*/
    LCD_WriteStringAtPos("    Group #3    ", 0, 0);
    
    LCD_WriteStringAtPos("     Mode 1     ", 1, 0); // line 1, position 0
    
}
/* The below function only handles BtnR presses. Think about how it could be
 expanded to handle all button presses. You will do this in the future */
void handle_button_presses()
{
    pressedUnlockedBtnR = FALSE;
    if (BUTTON_RIGHT && !buttonsLocked)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLocked = TRUE;
        pressedUnlockedBtnR = TRUE;
    }
    else if (!BUTTON_RIGHT && buttonsLocked)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLocked = FALSE;
    }
}
void delay_ms(int milliseconds)
{
    int i;
    for (i = 0; i < milliseconds * LOOPS_NEEDED_TO_DELAY_ONE_MS; i++) 
    {}
}


void logic_mode_one(){
    
    LCD_WriteStringAtPos("    Group#17    ", 0, 0); // line 0, position 0
    LCD_WriteStringAtPos("     Mode 1     ", 1, 0); // line 1, position 0
    
    // Turn all the LEDs on
     LATA |= 0x00FF;

    
    return;
}
void logic_mode_two() {
    // Display mode information on the LCD
    LCD_WriteStringAtPos("    Group#17    ", 0, 0); // line 0, position 0
    LCD_WriteStringAtPos("     Mode 2     ", 1, 0); // line 1, position 0


    #define INITIAL_PATTERN 0x00  // Example: 00000000
    

    switch (sw7) {
        case 0:  // SW7 is pressed
            switch (sw6) {
                case 0:  // SW6 is pressed
                    for (int i = 0; i < 8; i++) {
                        delay_ms(500);
                        LATA >>= 1;
                        if (LATA == 0) {
                            LATA = INITIAL_PATTERN;
                        }
                    }
                    break;
                    
                case 1:  // SW6 is not pressed
                    for (int i = 0; i < 8; i++) {
                        delay_ms(500);
                        LATA >>= 1;
                        if (LATA == 0) {
                            LATA = INITIAL_PATTERN;
                        }
                    }
                    break;
            }
            break;
            
        case 1:  // SW7 is not pressed
            switch (sw6) {
                case 0:  // SW6 is pressed
                    for (int i = 0; i < 8; i++) {
                        delay_ms(500);
                        LATA <<= 1;
                        if (LATA == 0) {
                            LATA = INITIAL_PATTERN;
                        }
                    }
                    break;
                    
                case 1:  // SW6 is not pressed
                    for (int i = 0; i < 8; i++) {
                        delay_ms(500);
                        LATA <<= 1;
                        if (LATA == 0) {
                            LATA = INITIAL_PATTERN;
                        }
                    }
                    break;
            }
            break;
    }

    return;
}

void logic_mode_three(){
    
    LCD_WriteStringAtPos("    Group#17    ", 0, 0); // line 0, position 0
    LCD_WriteStringAtPos("     Mode 3     ", 1, 0); // line 1, position 0
    
      // Turn all the LEDs off
    LATA = 0xFF00;
    return;
}
void logic_mode_four() {
    LCD_WriteStringAtPos("    Group#17    ", 0, 0); // line 0, position 0
    LCD_WriteStringAtPos("     Mode 4     ", 1, 0); // line 1,x position 0

   //ATA = 0x00;  

    if (sw7 && sw6) {
        uint8_t accumulated_pattern = 0x00;  
    for (int i = 0; i < 4; i++) {  
        delay_ms(500);
        accumulated_pattern |= (0x03 << (1 * i));  
        LATA = accumulated_pattern;
        if (LATA > 0xFF) {
            LATA = 0xFF;  
        }
    }
}
    if (sw7 && sw6 == 0) {
        uint8_t accumulated_pattern = 0x00;  
        for (int i = 0; i < 8; i++) {  
          delay_ms(500);
        accumulated_pattern |= (0x01 << i);  
            LATA = accumulated_pattern;
            if (LATA > 0xFF) {
              LATA &= 0xFF;  
        }
    }
}
    if (sw7 == 0 && sw6 == 0) {
    //nt8_t accumulated_pattern = 0x00;  
    for (int i = 0; i < 8; i++) {  
        delay_ms(500);
        //TA = accumulated_pattern;
        LATA = LATA>>1 |0x80;
//        if (i < 8) {  
//            accumulated_pattern |= 0x80;  
//        }   
    }
}
    if (sw7 == 0 && sw6 ) {
    uint8_t accumulated_pattern = 0x00;  
    for (int i = 0; i < 4; i++) {  
        delay_ms(500);
        LATA= LATA >>1|0xC0;
        //TA = accumulated_pattern;
        //cumulated_pattern >>= 2;
        //TA |= 1;
//        if (i < 3) { 
//            accumulated_pattern |= 0x03 << 2;  
//        }
    }
}
    }

int read_switch(void) {
    // Read the state of the switch (assuming connected to RB0)
    return PORTBbits.RB0;  // 1 if not pressed, 0 if pressed
}  
void logic_button_presses(enum mode *modePtr){
    if (*modePtr == MODE1){
        *modePtr = MODE2;
    }
    else if (*modePtr == MODE2){
        *modePtr = MODE3;
    }
    else if (*modePtr == MODE3){
        *modePtr = MODE4;
    }
    else if (*modePtr == MODE4){
        *modePtr = MODE1;
    }
}






