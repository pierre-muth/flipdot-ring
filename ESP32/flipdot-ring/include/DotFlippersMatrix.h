#include <Adafruit_GFX.h>

class DotFlippersMatrix : public Adafruit_GFX {
    public:
        DotFlippersMatrix(int16_t w, int16_t h);
        boolean begin();
        void drawPixel(int16_t x, int16_t y, uint16_t color);
        void clear(uint8_t color);
        void invert(void);
        void display(void);
        void setXshift(int shift);
        void drawCharsUpSideDown(int16_t x, int16_t y, const char *str, uint16_t color);
        void setDotFlipTime(uint8_t time) { customDotFlipTime = time; }
        void setForceFlipping(bool force) { forceFlipping = force; }
        void setCustomConfiguration(bool custom) { customConfiguration = custom; }
        void setDriversPowerSaving(bool powerSaving) { driversPowerSaving = powerSaving; }
        
        ~DotFlippersMatrix(void);
    
    private:
        uint8_t *drawingBuffer;
        uint8_t *flipdotBuffer;
        uint8_t *tempBuffer;
        bool customConfiguration = false;
        bool forceFlipping = false;
        bool driversPowerSaving = false;
        uint8_t customDotFlipTime = 4; // flip time in 100us unit, default to 400us

};