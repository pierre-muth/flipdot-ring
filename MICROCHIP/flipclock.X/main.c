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
© [2026] Microchip Technology Inc. and its subsidiaries.
 
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

uint8_t rxBuffer[XSIZE];
uint8_t displayBuffer[XSIZE];
uint8_t displayState[XSIZE];
uint8_t flipping_time = 2;
uint8_t bufferIndex = 0;
uint8_t refreshDisplayNeeded = 0;
uint8_t newDisplayData = 0;
uint8_t forceFlipping = 0;
uint8_t powerSaveingDrivers = 0;

void delay100us(uint8_t usx100){
    for (uint8_t i = 0; i < usx100; i++) {
        __delay_us(100);
    }

}

void reset(uint8_t x, uint8_t y) {
    PORTB = col_set_addr[x];
    PORTA = row_set_addr[y];
    colE_SetHigh();
    rowE_SetHigh();
    delay100us(flipping_time);
    colE_SetLow();
    rowE_SetLow();
}

void set(uint8_t x, uint8_t y) {
    PORTB = col_reset_addr[x];
    PORTA = row_reset_addr[y];
    colE_SetHigh();
    rowE_SetHigh();
    delay100us(flipping_time);
    colE_SetLow();
    rowE_SetLow();
}
void animate(){
    uint8_t x, y;
    
    FP_POW_SetHigh();
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
        __delay_ms(15);
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
    
    FP_POW_SetLow();
    LED_SetLow();
}
// Copy rxBuffer to displayBuffer, and scan all the dots and change the state accordingly
// and finally copy displayBuffer to displayState
void display(void) {
    // enable drivers
    FP_POW_SetHigh();
    LED_SetHigh();
    
    uint8_t x, y, segmentState, segment, mask;
    uint8_t flipTimeOpt = 0;

    // copy rxBuffer to displayBuffer
    for (x=0; x<XSIZE; x++){
        displayBuffer[XSIZE-x-1] = rxBuffer[x];
    }

    // if there is a new configuration, update the display parameters.
    if (displayBuffer[0] & 0b10000000) {
        // decode the flipping time parameter
        if (displayBuffer[1] & 0b10000000) flipTimeOpt |= 0b00000001;
        if (displayBuffer[2] & 0b10000000) flipTimeOpt |= 0b00000010;
        if (displayBuffer[3] & 0b10000000) flipTimeOpt |= 0b00000100;
        if (displayBuffer[4] & 0b10000000) flipTimeOpt |= 0b00001000;
        flipping_time = flipTimeOpt;

        // decode the 'force or skip' driving parameter
        if (displayBuffer[5] & 0b10000000) forceFlipping = 1;
        else forceFlipping = 0;
        
        if (displayBuffer[6] & 0b10000000) powerSaveingDrivers = 1;
        else powerSaveingDrivers = 0;
    }

    // scan all the dots
    for (y=0; y<YSIZE; y++){
        mask = 0b1 << y;
        for (x=0; x<XSIZE; x++){
            segment = displayBuffer[x];
            segmentState = displayState[x];

            // if we are not forcing the dot flipping.
            if (!forceFlipping) {
                // then if there is no state change then we skip the driving.
                if ((mask & segment) == (mask & segmentState)) {
                    continue;
                }
            }

            // Drive the flipping.
            if ( (mask & segment) > 0){
                set(x,y);
            } else {
                reset(x,y);
            }
            
        }
    }

    // copy displayBuffer to displayState
    for (x=0; x<XSIZE; x++){
        displayState[x] = displayBuffer[x];
    }

    // reset status flags
    newDisplayData = 0;
    refreshDisplayNeeded = 0;
    
    // switch off driver 5V if requested
    if (powerSaveingDrivers != 0) FP_POW_SetLow();
    
    LED_SetLow();
}

// interrupt handler
void __interrupt() ISR (void) {
    
    // We have an SPI interrupt
    if(PIR3bits.SSP1IF == 1) {
        // put the received byte into the panelBuffer at index, it overrides the byte that has already be shifted out
        rxBuffer[bufferIndex] = SSP1BUF;
        // increment the buffer index, modulo panel size 24
        bufferIndex++;
        if (bufferIndex >= XSIZE) bufferIndex = 0;
        // put the byte from the panelBuffer at this new index into SSP1BUF, it will be shifted out at the next byte reception
        SSP1BUF = rxBuffer[bufferIndex];
        // enable and reset the latch timer 
        TMR0_Write(0);
        PIE0bits.TMR0IE = 1;
        // set new data incoming flag
        newDisplayData = 1;
        // reset SPI interrupt flag
        PIR3bits.SSP1IF = 0;
    } 

    // We have a Timer0 interrupt
    if(PIR0bits.TMR0IF == 1) {
        // reset interrupt flag
        PIR0bits.TMR0IF = 0;
        // if data has been received, it is time to latch
        if (newDisplayData) {
            // reset the SPI in case wrong clock/data de-sync
            SSP1CON1bits.SSPEN = 0; 
            SSP1CON1bits.SSPEN = 1;  
            // reset the buffer index
            bufferIndex = 0;       
            // set the display refresh-needed flag. display refresh happens outside interrupt.
            refreshDisplayNeeded = 1;
        }
    } 
    
    // clear IOC pin interrupt flag and disable the IOC interrupt, we stay awake until full display.
    if ( IOCCF == 1) {
        IOCCF = 0;
        PIE0bits.IOCIE = 0;
        IOCCPbits.IOCCP0 = 0;
    }
}

int main(void) {   
    // init 
    SYSTEM_Initialize();
    
    // animation
    animate();
    __delay_ms(500);
            
    // enable SPI1
    SPI1_Open(CLIENT_CONFIG);
    SSP1CON1bits.SSPEN = 1;
    PIE3bits.SSP1IE = 1;
    
    // enable interrupts
    INTERRUPT_GlobalInterruptEnable(); 
    INTERRUPT_PeripheralInterruptEnable(); 
    
    while(1) {
        
        if (refreshDisplayNeeded) {
            // actualize the display
            display();
            // disable timer to sleep longer, 
            // it is re-enable at SPI reception
            PIE0bits.TMR0IE = 0;
            // Enable the interrupt on change on SPI clock pin
            PIE0bits.IOCIE = 1;
            IOCCPbits.IOCCP0 = 1;
            // Go to sleep mode, we will sleep again after a display refresh.
            SLEEP();
        }
        
    }    
}