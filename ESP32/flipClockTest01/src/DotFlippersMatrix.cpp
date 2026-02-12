
#include <Adafruit_GFX.h>
#include "DotFlippersMatrix.h"
#include <SPI.h>

#define DATAPIN    13
#define CLOCKPIN   14

static const int spiClk = 100000; 

SPIClass * hspi = NULL; //uninitalised pointers to SPI object
int xShift=0;

DotFlippersMatrix::DotFlippersMatrix(int16_t w, int16_t h) : 
    Adafruit_GFX(w, h) {

    // SPI init
    hspi = new SPIClass(HSPI);
    hspi->begin(CLOCKPIN, 2, DATAPIN, 4); //SCLK, MISO, MOSI, SS

}

boolean DotFlippersMatrix::begin() {
    // init pixels array
    if((!drawingBuffer) && !(drawingBuffer = (uint8_t *)malloc(WIDTH * HEIGHT))) {
        return false;
    }

    // init data to be transmitted 
    if((!flipdotBuffer) && !(flipdotBuffer = (uint8_t *)malloc(WIDTH))) {
        return false;
    }

    // clear the arrays
    clear(0x00);
    display();

    return true;
}

// invert all pixels
void DotFlippersMatrix::invert() {
    for (int i=0; i<(WIDTH * HEIGHT); i++) {
        drawingBuffer[i] = drawingBuffer[i] > 0 ? 0 : 1;
    }
}

// set all pixels to a color
void DotFlippersMatrix::clear(uint8_t color) {
    for (int i=0; i<(WIDTH * HEIGHT); i++) {
        drawingBuffer[i] = color;
    }
}

// set a specific pixel color, no transmission
void DotFlippersMatrix::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Serial.printf("x=%d, y=%d, c=%d\n", x, y, color);
    if (x<0 || y<0) return;
    if (y >= HEIGHT) return;
    if (x>=WIDTH) {
        x = x%WIDTH;
    }
    drawingBuffer[(y*WIDTH)+x] = color;

}

// shit the matrix 0 coordinate horizontally
void DotFlippersMatrix::setXshift(int shift) {
    if (shift < 0) xShift = WIDTH + (shift%WIDTH);
    else xShift = shift%WIDTH;
}

// compute the transmission buffer and send it on SPI
void DotFlippersMatrix::display() {
    uint8_t mask = 0x00;
    int xOffset = 0;

    memset(flipdotBuffer, 0, WIDTH);  

    // scan over the drawing buffer and build the transmission buffer. 
    // First column transmitted will be pushed to the last position of the display 
    for(int x=0; x<WIDTH; x++) {
        xOffset = (x+xShift)%WIDTH;
        for(int y=0; y<HEIGHT; y++){
            mask = 0x01 << y;
            if (drawingBuffer[(y*WIDTH) + xOffset] != 0) {
                flipdotBuffer[ WIDTH-1-x ] |= mask;
            } else {
                flipdotBuffer[ WIDTH-1-x ] &= ~mask;
            }
        }
    }

    // set the options bits for each of the 12 panels
    for (int i=0; i<12; i++) {
        if (customConfiguration) {
            flipdotBuffer[(i*24)+23] |= 0x80; // set the custom configuration bit for i panel
        }
        if (customFlipTime & 0b00000001) {
            flipdotBuffer[(i*24)+22] |= 0x80; // set the custom flip time bit for i panel
        }
        if (customFlipTime & 0b00000010) {
            flipdotBuffer[(i*24)+21] |= 0x80; // set the custom flip time bit for i panel
        }
        if (customFlipTime & 0b00000100) {
            flipdotBuffer[(i*24)+20] |= 0x80; // set the custom flip time bit for i panel
        }
        if (customFlipTime & 0b00001000) {
            flipdotBuffer[(i*24)+19] |= 0x80; // set the custom flip time bit for i panel
        }
        if (forceFlipping) {
            flipdotBuffer[(i*24)+18] |= 0x80; // set the force flipping bit for i panel
        }
    }
    

    // transmit
    hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    hspi->transferBytes(flipdotBuffer , NULL, WIDTH);
    hspi->endTransaction();

    // debug: print the transmission buffer vertically
    for (int i=0; i<WIDTH; i++) {
        for(int j=0; j<8; j++) {
            if(j%8 == 7) {
                if ((flipdotBuffer[i] >> j) & 0x01) Serial.print(" 1");
                else Serial.print(" 0");
            } else {
                if ((flipdotBuffer[i] >> j) & 0x01) Serial.print("#");
                else Serial.print(" ");
            }
        }
        if(i%24 == 23) Serial.println(" < custom config bit ");
        else if(i%24 == 22) Serial.println(" < flip time bit 0");
        else if(i%24 == 21) Serial.println(" < flip time bit 1");
        else if(i%24 == 20) Serial.println(" < flip time bit 2");
        else if(i%24 == 19) Serial.println(" < flip time bit 3");
        else if(i%24 == 18) Serial.println(" < force flipping bit");
        else Serial.println("");
    }
    Serial.println("--------");
}

// 180° - Text upside down
void DotFlippersMatrix::drawStringRotated180(int16_t x, int16_t y, const char *str, uint16_t color) {
    // Step 1: Create temp buffer
    uint8_t tempBuf[WIDTH * HEIGHT];
    memset(tempBuf, 0, WIDTH * HEIGHT);
    
    uint8_t *savedBuf = drawingBuffer;
    drawingBuffer = tempBuf;
    
    // Step 2: Draw text at origin
    setCursor(0, 0);
    setTextColor(color);
    print(str);
    
    // Step 3: Rotate 180°: flip both horizontally and vertically
    for(int row = 0; row < HEIGHT; row++) {
        for(int col = 0; col < WIDTH; col++) {
            int srcIdx = row * WIDTH + col;
            int newCol = WIDTH - 1 - col;
            int newRow = HEIGHT - 1 - row;
            int dstIdx = newRow * WIDTH + newCol;
            
            drawingBuffer[dstIdx] = tempBuf[srcIdx];
        }
    }
    
    // Step 4: Apply offset
    drawingBuffer = savedBuf;
    for(int row = 0; row < HEIGHT; row++) {
        for(int col = 0; col < WIDTH; col++) {
            int newCol = col + x;
            int newRow = row + y;
            
            if(newCol < WIDTH && newRow < HEIGHT && tempBuf[row * WIDTH + col]) {
                drawingBuffer[newRow * WIDTH + newCol] = color;
            }
        }
    }
}


// object deletion
DotFlippersMatrix::~DotFlippersMatrix(void) {
    if(drawingBuffer) {
        free(drawingBuffer);
        drawingBuffer = NULL;
    }
    if (flipdotBuffer) {
        free(flipdotBuffer);
        flipdotBuffer = NULL;
    }

}

