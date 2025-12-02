#include <Adafruit_GFX.h>


class DotFlippersMatrix : public Adafruit_GFX {
    public:
        DotFlippersMatrix(int16_t w, int16_t h);
        boolean begin();
        void drawPixel(int16_t x, int16_t y, uint16_t color);
        void clear(uint8_t color);
        void invert(void);
        void display(void);

        ~DotFlippersMatrix(void);
    
    private:
        uint8_t *drawingBuffer;
        uint8_t *flipdotBuffer;
        

};