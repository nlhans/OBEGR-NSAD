#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <multicore.h>
#include <U8g2_for_Adafruit_GFX.h>

#define RAMFUNC __attribute__((optimize("-O3"), section(".ramfunc"), noinline))
void core1_main() RAMFUNC;

/*
 * Pinout:
*  Cable (Colour) |  Net  | Pico Connection
 * Pin 1 (Bruin)  |  VCC  |      N/C  
 * Pin 2 (Rood)   |  CLA  | Pin 2 (GP1)
 * Pin 3 (Oranje) |  CLK  | Pin 4 (GP2)
 * Pin 4 (Geel)   |   DI  | Pin 5 (GP3)
 * Pin 5 (Groen)  |   EN  | Pin 1 (GP0)
 * Pin 6 (Blauw)  |  GND  | Pin 3 (GND)
*/
template<int Pin_CLK, int Pin_DI, int Pin_CLA, int Pin_EN>
class OBERGRANSAD : public Adafruit_GFX {
protected:
  static const uint32_t PX = 16 * 16;

  static const uint8_t bits = 6;
  static const uint32_t bits_pow2 = 1<<bits;

  // The last buffer position is for dummy writes when a pixel is out of bounds.
  volatile uint16_t bufferA[PX+1];
  volatile uint32_t bufferB[PX+1];

  std::array<uint16_t, 256> gamma; // 8-bit grayscale to the colorspace of display

  constexpr std::array<uint16_t, 256> getGammaArray()
  {
      std::array<uint16_t, 256> arr{0};
      for (int i=0; i < 256; ++i) {
        if (i == 0) {
          arr[i] = 0;
        } else {
          arr[i] = 1 + (bits_pow2 - 1) * powf(i * 1.0f / 256, 2.7f);
        }
      }
      return arr;
  }

  uint8_t mapXY(int16_t x, int16_t y) {
    if (x<0 || y<0 || x>15 || y>15) return PX;

    uint8_t px = (y*16+x);

    uint8_t board = (px / 64);
    uint8_t boardPx = px % 64;
    uint8_t boardPx8 = px % 8;

    if (boardPx >= 0 && boardPx < 8)   return board*64 + 0  + boardPx8;
    if (boardPx >= 8 && boardPx < 16)  return board*64 + 16 + boardPx8;
    if (boardPx >= 16 && boardPx < 24)  return board*64 + 15 - boardPx8;
    if (boardPx >= 24 && boardPx < 32)  return board*64 + 31 - boardPx8;

    if (boardPx >= 32 && boardPx < 40)  return board*64 + 48  + boardPx8;
    if (boardPx >= 40 && boardPx < 48)  return board*64 + 32 + boardPx8;
    if (boardPx >= 48 && boardPx < 56)  return board*64 + 63 - boardPx8;
    if (boardPx >= 56 && boardPx < 64)  return board*64 + 47 - boardPx8;

    return PX;
  }

  volatile bool blank = true;
public:
  virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (color > 255) color = 255;

    bufferA[mapXY(15-x,y)] = gamma[color];
  }

  // https://www.tme.eu/Document/8876e3da3e0cc25d8b4c7cdeea8b8a88/SCT2024.pdf
  void display() RAMFUNC {
    if (blank) return;
    
    static uint32_t cycle = 0;
    cycle++;
    if (cycle >= bits_pow2) cycle = 0;

    auto px = bufferB;
    for (uint32_t i = 0; i < 256; i++) {
      gpio_put(Pin_DI, *px>cycle);
      px++;
      asm volatile("nop\nnop\nnop\nnop\nnop");
      gpio_put(Pin_CLK, 1);
      asm volatile("nop\nnop\nnop\nnop\nnop");
      gpio_put(Pin_CLK, 0);
      asm volatile("nop\nnop\nnop");
    }
    delayMicroseconds(1);
    gpio_put(Pin_CLA, HIGH);
    delayMicroseconds(1);
    gpio_put(Pin_CLA, LOW);
  }

  void show() {
    gpio_put(Pin_EN, HIGH);
    blank = true;
    for (size_t i = 0; i < sizeof(bufferA)/sizeof(bufferA[0]); i++) {
      bufferB[i] = bufferA[i];
    }
    gpio_put(Pin_EN, LOW);
    blank = false;
  }

  OBERGRANSAD() : Adafruit_GFX(16,16), gamma{getGammaArray()} {
    // Setup GPIOs
    pinMode(Pin_CLK, OUTPUT);
    pinMode(Pin_DI, OUTPUT);
    pinMode(Pin_CLA, OUTPUT);
    pinMode(Pin_EN, OUTPUT);

    gpio_set_drive_strength(Pin_CLK, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(Pin_DI, GPIO_DRIVE_STRENGTH_12MA);

    // Clear pixel buffer
    for (size_t i = 0; i < sizeof(bufferA)/sizeof(bufferA[0]); i++) {
      bufferA[i] = 0;
      bufferB[i] = 0;
    }

    // Pulling EN low enables display output
    gpio_put(Pin_EN, LOW);
  }
};
using MyDisplay = OBERGRANSAD<2, 3, 1, 0>; // Pins: CLK, DI, CLA, EN

MyDisplay myFirstLEDMatrix{}; 
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;
void setup() {
  myFirstLEDMatrix.fillScreen(0);
  myFirstLEDMatrix.display();
  u8g2_for_adafruit_gfx.begin(myFirstLEDMatrix);

  Serial.begin(115200);
  
  multicore_reset_core1();
  multicore_launch_core1(core1_main);
}


int16_t scrollX = 0;
void loop() {
  myFirstLEDMatrix.fillScreen(0);
  
  u8g2_for_adafruit_gfx.setForegroundColor(255); // on
  u8g2_for_adafruit_gfx.setBackgroundColor(0); // off
  u8g2_for_adafruit_gfx.setFont(u8g2_font_5x7_mf);
  u8g2_for_adafruit_gfx.setFontDirection(3);
  u8g2_for_adafruit_gfx.setFontMode(0); // non-transparant 
  u8g2_for_adafruit_gfx.setCursor(15, scrollX-20);
  u8g2_for_adafruit_gfx.print("Fablab");
  u8g2_for_adafruit_gfx.setCursor(7, scrollX);
  u8g2_for_adafruit_gfx.print("Oldenzaal");

  scrollX ++;
  if (scrollX > 120) {
    scrollX = 0;
  }
  myFirstLEDMatrix.show();
  
  uint32_t e = millis() + 50;
  while (e > millis());
}

void core1_main() {
  while(1) {
    myFirstLEDMatrix.display(); //51-52us per update
  }
}