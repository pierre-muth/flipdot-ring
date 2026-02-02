
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
    if (y >= HEIGHT) return;
    if (x>=WIDTH) {
        x = x%WIDTH;
    }
    drawingBuffer[(y*WIDTH)+x] = color;

}

void DotFlippersMatrix::setXshift(int shift) {
    if (shift < 0) xShift = WIDTH + (shift%WIDTH);
    else xShift = shift%WIDTH;
}

void DotFlippersMatrix::display() {
    uint8_t mask = 0x00;
    int xOffset = 0;

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