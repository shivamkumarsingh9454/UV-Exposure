/* This project is for PIC16F676 for the UV-Exposure control panel.
 * Key features of it will be:
 * - a knob for setting delay time [30s - 10m] [ADC, read continiusly with timer]
 * - Timer for making the delay.
 * - Outputs for Relay and indicator LED(to indicate if the time is over)
 */

#pragma config FOSC  = INTRCIO
#pragma config WDTE  = OFF    
#pragma config PWRTE = OFF   
#pragma config MCLRE = OFF   
#pragma config BOREN = ON    
#pragma config CP    = ON       
#pragma config CPD   = ON      

#include <xc.h>
#include <stdint.h>
#define _XTAL_FREQ 4000000UL // 4MHz internal RC oscilator

#define RELAY RC0
#define LED1  RC1
#define LED2  RC2

void ADC_Init(void)
{
    ADCON0 = 0x1D; // ADC Result Left justified, analog channel AN7, ADON = 1
    ADCON1 = 0x10; // ADCS<2:> = 001 => Fosc/8
    ANSEL  = 0x80; // ANS7 = 1
}

void Timer1_Init(void)
{
    INTCON = 0xC0; // Global interrupt = 1, pheripheral interrupt = 1
    PIE1   = 0x01; // Timer1 overflow interrupt enable bit = 1
    T1CON  = 0x35; // TImer1 on bit = 1, timer1 clock source internal, do not sync external clock input, LP OSC off, prescalar 1:8, Timer1 is on
}

void GPIO_Init(void)
{
    CMCON = 0x07; // Turn off comparator
    TRISC = 0x08; // RC3/AN7 input, other output
    TRISA = 0x00; // entire port a output
    PORTA = 0x00; // clear port a
    PORTC = 0x00; // clear port c
}

uint16_t ADC_Read(void)
{
    uint16_t ADC_Result = 0x00; // Temp buffer to store the 10 bit adc value
    ADCON0 |= (1 << 1);         // Set GO/DONE bit to initiate the ADC Conversion
    while(ADCON0 & (1 << 1));   // Wait while the conversion is taking place (wait for GO/DONE bit to clear)
    ADC_Result = ((uint16_t)ADRESH << 2) | ((uint16_t)ADRESL >> 6); // copy the 10 bit result in the 16 bit buff variable
    return ADC_Result;          // Return the ADC Results
}

volatile uint16_t Overflow_Counter = 0x0000;        // Timer1 Overflow Overflow_Counterer
void __interrupt() ISR(void)
{
    if(TMR1IF)
    {
        Overflow_Counter++;    // Increment Overflow Overflow_Counterer every time Timer1 overflow interrrupts
        TMR1IF = 0; // Clear Timer1 Interrupt bit
    }
}

/* Time       = [30000, 600000]
 * ADC Result = [0    , 1023  ]
 * Linear Dependency (directly proportional)
 * 
 * Time = m*ADC_Result + 30000     {y = mx + c}
 * m = (y2 - y1) / (x2 - x1) = 557
 * 
 * Time = 557*ADC_Result + 30000   {y = mx + c}
 * 
 * Time = (557*ADC_Result + 30000) ms & count = Time/525
 * 
 * => count = (ADC_Result*557 + 30000) / 525
 *          =  ADC_Result + 57
 * 
 * ADREAD_TO_COUNT_CONSTANT = 57
 */
#define ADREAD_TO_COUNT_CONSTANT 57

void main(void)
{
    GPIO_Init();
    ADC_Init();
    Timer1_Init();
    __delay_ms(50); 
    
    RELAY = 1;
    LED1  = 1;
    LED2  = 0;
    
    while(1)
    {
        uint16_t Count_Constant = ADC_Read() + ADREAD_TO_COUNT_CONSTANT;
        
        GIE = 0;
        uint16_t Current_OverflowCounter_Val = Overflow_Counter;
        GIE = 1;
        __delay_ms(10); 
        
        if(Current_OverflowCounter_Val >= Count_Constant)
        {
            Overflow_Counter = 0x0000;
            RELAY = 0;
            LED1  = 0;
            LED2  = 1;
            while(1);
        }
    }
}

// Finalized by Shivam on 25-10-2026 1:14 pm
 