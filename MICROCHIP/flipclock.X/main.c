/*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.2
 *
 * @version Package Version: 3.1.2
 */

/*
 © [2025] Microchip Technology Inc. and its subsidiaries.
 
 Subject to your compliance with these terms, you may use Microchip 
 software and any derivatives exclusively with Microchip products. 
 You are responsible for complying with 3rd party license terms  
 applicable to your use of 3rd party software (including open source  
 software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
 NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
 SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
 MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
 WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
 KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
 MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
 FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
 TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
 EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
 THIS SOFTWARE.
 */
#include "mcc_generated_files/system/system.h"

/*
 Main application
 */

#define XSIZE 24
#define YSIZE 7

const uint8_t col_set_addr[XSIZE] = {
    // column at +24v (FP Data pin = b5 = 1) PORTB
    0b111001,    // column 01 = 3A
    0b111010,    // column 02 = 3B
    0b111011,    // column 03 = 3C
    0b111100,    // column 04 = 3D
    0b111110,    // column 05 = 3F
    0b100001,    // column 06 = 0A
    0b100010,    // column 07 = 0B
    0b100011,    // column 08 = 0C
    0b100100,    // column 09 = 0D
    0b100110,    // column 10 = 0F
    0b100101,    // column 11 = 0E
    0b100111,    // column 12 = 0G
    0b101111,    // column 13 = 1G
    0b101101,    // column 14 = 1E
    0b101001,    // column 15 = 1A
    0b101010,    // column 16 = 1B
    0b101011,    // column 17 = 1C
    0b101110,    // column 18 = 1F
    0b101100,    // column 19 = 1D
    0b110001,    // column 20 = 2A
    0b110010,    // column 21 = 2B
    0b110011,    // column 22 = 2C
    0b110110,    // column 23 = 2F
    0b110100,    // column 24 = 2D
};
const uint8_t row_set_addr[YSIZE] = {
    // row at GND (FP Data pin = a5 = 0) PORTA
    0b001111,    // row 01L = 1G
    0b001101,    // row 02L = 1E
    0b000101,    // row 03L = 0E
    0b001010,    // row 04L = 1B
    0b000100,    // row 05L = 0D
    0b001110,    // row 06L = 1F
    0b000010,    // row 07L = 0B
};
const uint8_t col_reset_addr[XSIZE] = {
    // column at GND (FP Data pin = b5 = 0) PORTB
    0b011001,    // column 01 = 3A
    0b011010,    // column 02 = 3B
    0b011011,    // column 03 = 3C
    0b011100,    // column 04 = 3D
    0b011110,    // column 05 = 3F
    0b000001,    // column 06 = 0A
    0b000010,    // column 07 = 0B
    0b000011,    // column 08 = 0C
    0b000100,    // column 09 = 0D
    0b000110,    // column 10 = 0F
    0b000101,    // column 11 = 0E
    0b000111,    // column 12 = 0G
    0b001111,    // column 13 = 1G
    0b001101,    // column 14 = 1E
    0b001001,    // column 15 = 1A
    0b001010,    // column 16 = 1B
    0b001011,    // column 17 = 1C
    0b001110,    // column 18 = 1F
    0b001100,    // column 19 = 1D
    0b010001,    // column 20 = 2A
    0b010010,    // column 21 = 2B
    0b010011,    // column 22 = 2C
    0b010110,    // column 23 = 2F
    0b010100,    // column 24 = 2D
};
const uint8_t row_reset_addr[YSIZE] = {
    // row at +24v (FP Data pin = a5 = 1) PORTA
    0b100111,    // row 01H = 0G
    0b101001,    // row 02H = 1A
    0b100110,    // row 03H = 0F
    0b101011,    // row 04H = 1C
    0b100011,    // row 05H = 0C
    0b101100,    // row 06H = 1D
    0b100001,    // row 07H = 0A
};

const uint16_t flipping_time = 250;

uint8_t rxBuffer[XSIZE];
uint8_t displayBuffer[XSIZE];
uint8_t displayState[XSIZE];
uint8_t bufferIndex = 0;
uint8_t refreshDisplay = 0;

void reset(uint8_t x, uint8_t y) {
    PORTB = col_set_addr[x];
    PORTA = row_set_addr[y];
    colE_SetHigh();
    rowE_SetHigh();
    __delay_us(flipping_time);
    colE_SetLow();
    rowE_SetLow();
}

void set(uint8_t x, uint8_t y) {
    PORTB = col_reset_addr[x];
    PORTA = row_reset_addr[y];
    colE_SetHigh();
    rowE_SetHigh();
    __delay_us(flipping_time);
    colE_SetLow();
    rowE_SetLow();
}
void animate(){
    uint8_t x, y;

    LED_SetHigh();
    
    for (y=0; y<YSIZE; y++){
        for (x=0; x<XSIZE; x++){
            reset(x,y);
        }
    }
    __delay_ms(100);
    
    for (x=0; x<XSIZE+4; x++){
        if (x < 24) {
            for (y=0; y<YSIZE; y++){
                set(x,y);
            } 
        }
        if (x-4 >= 0) {
            for (y=0; y<YSIZE; y++){
                reset(x-4,y);
            }
        }
        __delay_ms(20);
    }
    for (x=0; x<XSIZE+5; x++){
        if (x < 24) {
            for (y=0; y<YSIZE; y++){
                set(23-x,y);
            } 
        }
        if ((XSIZE+4)-x < 24) {
            for (y=0; y<YSIZE; y++){
                reset((XSIZE+4)-x,y);
            }
        }
        __delay_ms(15);
    }
    
    LED_SetLow();
}
void display(void) {
    LED_SetHigh();
    uint8_t x, y, segmentState, segment, mask;
    
    for (x=0; x<XSIZE; x++){
        displayBuffer[XSIZE-x-1] = rxBuffer[x];
    }
    
    for (y=0; y<YSIZE; y++){
        mask = 0b1 << y;
        for (x=0; x<XSIZE; x++){
            segment = displayBuffer[x];
            segmentState = displayState[x];
            
            if ((mask & segment) == (mask & segmentState)) {
                continue;
            }
            
            if ( (mask & segment) > 0){
                set(x,y);
            } else {
                reset(x,y);
            }
        }
    }
    
    for (x=0; x<XSIZE; x++){
        displayState[x] = displayBuffer[x];
    }
    
    refreshDisplay = 0;
    LED_SetLow();
}

void spi1InterruptHandler(void) {
    // put the received byte into the panelBuffer at index, it override the byte that has already be shifted out
    // increment the buffer index, modulo panel size 24
    // put the byte from the panelBuffer at this new index into SSP1BUF, it will be shifted out at the next byte reception
    // reset the timer 
    // and maybe the SPI interrupt flag
    
    rxBuffer[bufferIndex] = SSP1BUF;
    bufferIndex++;
    if (bufferIndex >= XSIZE) bufferIndex = 0;
    SSP1BUF = rxBuffer[bufferIndex];
    TMR0_Write(0);
    refreshDisplay = 1;
}

void tmr0InterruptHandler(void) {
    // if no new data has been received ?
    if (refreshDisplay) {
        SSP1CON1bits.SSPEN = 0; // reset the SPI
        SSP1CON1bits.SSPEN = 1; // in case wrong clock/data de-sync
        bufferIndex = 0;        // and the buffer index
        display();
    }
}
void __interrupt() INTERRUPT_InterruptManager (void) {

 /* TODO
 - directly process SPI after the flag check to avoid a jump
 - use the refresh-needed flag in the main loop not in the interrupt 
   reduce the main loop delay (1ms ? null ?)
 */
 
    // interrupt handler
    if(PIR3bits.SSP1IF == 1) {
        PIR3bits.SSP1IF = 0;
        spi1InterruptHandler();
    } 
    if(PIR0bits.TMR0IF == 1) {
        PIR0bits.TMR0IF = 0;
        tmr0InterruptHandler();
    } 
}
int main(void) {   
    // init 
    SYSTEM_Initialize();
    
    // animation
    animate();
    __delay_ms(1000);
            
    // enable SPI1
    SPI1_Open(CLIENT_CONFIG);
    SSP1CON1bits.SSPEN = 1;
    PIE3bits.SSP1IE = 1;
    
    // enable timer0 interrupt
    PIE0bits.TMR0IE = 1;
    
    // enable interrupts
    INTERRUPT_GlobalInterruptEnable(); 
    INTERRUPT_PeripheralInterruptEnable(); 
    
    while(1) {
        __delay_ms(250);
    }    

}
