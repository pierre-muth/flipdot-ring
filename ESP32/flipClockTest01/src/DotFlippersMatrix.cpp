
#include <Adafruit_GFX.h>
#include "DotFlippersMatrix.h"
#include <SPI.h>

#define DATAPIN    13
#define CLOCKPIN   14

static const int spiClk = 80000; 

SPIClass * hspi = NULL; //uninitalised pointers to SPI object
 

DotFlippersMatrix::DotFlippersMatrix(int16_t w, int16_t h) : 
    Adafruit_GFX(w, h) {

    // SPI init
    hspi = new SPIClass(HSPI);
    hspi->begin(CLOCKPIN, 2, DATAPIN, 4); //SCLK, MISO, MOSI, SS

}

boolean DotFlippersMatrix::begin() {
    if((!drawingBuffer) && !(drawingBuffer = (uint8_t *)malloc(WIDTH * HEIGHT))) {
        return false;
    }

    if((!flipdotBuffer) && !(flipdotBuffer = (uint8_t *)malloc(WIDTH))) {
        return false;
    }

    clear(0x00);
    display();

    return true;
}

void DotFlippersMatrix::invert() {
    for (int i=0; i<(WIDTH * HEIGHT); i++) {
        drawingBuffer[i] = drawingBuffer[i] > 0 ? 0 : 1;
    }
}

void DotFlippersMatrix::clear(uint8_t color) {
    for (int i=0; i<(WIDTH * HEIGHT); i++) {
        drawingBuffer[i] = color;
    }
}

void DotFlippersMatrix::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Serial.printf("x=%d, y=%d, c=%d\n", x, y, color);
    if (x<0 || y<0) return;
    if (x>=WIDTH || y>=HEIGHT) {
        x = x%WIDTH;
        y = y%HEIGHT;
    }
    drawingBuffer[(y*WIDTH)+x] = color;

}

void DotFlippersMatrix::display() {
    uint8_t mask = 0x00;

    for(int x=0; x<WIDTH; x++) {
        for(int y=0; y<HEIGHT; y++){
            mask = 0x01 << y;
            if (drawingBuffer[(y*WIDTH) + x] != 0) {
                flipdotBuffer[ x ] |= mask;
            } else {
                flipdotBuffer[ x ] &= ~mask;
            }
        }
    }

    hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    hspi->transferBytes(flipdotBuffer , NULL, WIDTH);
    hspi->endTransaction();

    Serial.print(".");
    // for (int i=0; i<WIDTH; i++) {
    //     Serial.print(flipdotBuffer[i], HEX);
    //     Serial.print(" ");
    // }
}

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