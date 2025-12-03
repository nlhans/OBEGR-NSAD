#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "pico/time.h"

/*
 * Pinout:
*  Cable (Colour) |  Net  | Pico Connection
 * Pin 1 (Bruin)  |  VCC  |      N/C  
 * Pin 2 (Rood)   |  CLA  | Pin 20 (GP15)
 * Pin 3 (Oranje) |  CLK  | Pin 19 (GP14)
 * Pin 4 (Geel)   |   DI  | Pin 21 (GP16)
 * Pin 5 (Groen)  |   EN  | Pin 22 (GP17)
 * Pin 6 (Blauw)  |  GND  |  Pin 23 (GND)
*/
bool flushDisplay(repeating_timer_t* tmr);

class OBERGRANSAD : public Adafruit_GFX {
protected:
  static const uint32_t PX = 16 * 16;

  static const uint8_t bits = 4;
  static const uint8_t bits_pow2 = 1<<bits;
  
  alarm_pool_t* alarm_pool = nullptr;
  repeating_timer_t alarm_timer;

  const uint8_t Pin_CLK;
  const uint8_t Pin_DI;
  const uint8_t Pin_CLA;
  const uint8_t Pin_EN;

  // The last buffer position is for dummy writes when a pixel is out of bounds.
  uint16_t buffer[PX+1];

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

    return 0;
  }

  virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    buffer[mapXY(15-x,y)] = color;
  }

  uint8_t cycle = 0;
public:

// https://www.tme.eu/Document/8876e3da3e0cc25d8b4c7cdeea8b8a88/SCT2024.pdf
void display() {
  cycle++;
  if (cycle >= bits_pow2) cycle = 0;

  digitalWrite(Pin_CLA, LOW);
  delayMicroseconds(1);
  for (uint_fast8_t i = 0; i < 16; i++) {
    for (uint_fast8_t j = 0; j < 16; j++) {
      digitalWrite(Pin_DI, buffer[i*16+j]>cycle?HIGH:LOW);
      
      // gpio_put(14, true);
      // gpio_put(14, false);
      asm volatile("nop\nnop\n");
      // digitalWrite(Pin_CLK, HIGH);
      gpio_put(14, 1);
      asm volatile("nop\nnop\n");
      digitalWrite(Pin_CLK, LOW);
    }
  }
  delayMicroseconds(5);
  digitalWrite(Pin_CLA, HIGH);
  delayMicroseconds(5);
  digitalWrite(Pin_CLA, LOW);
}

OBERGRANSAD(uint8_t pin_CLK, uint8_t pin_DI, uint8_t pin_CLA, uint8_t pin_EN) : Adafruit_GFX(16,16), Pin_CLK(pin_CLK), Pin_DI{pin_DI}, Pin_CLA{pin_CLA}, Pin_EN{pin_EN} {
    // Setup GPIOs
    pinMode(Pin_CLK, OUTPUT);
    pinMode(Pin_DI, OUTPUT);
    pinMode(Pin_CLA, OUTPUT);
    pinMode(Pin_EN, OUTPUT);

    // Clear pixel buffer
    memset(buffer, 0, sizeof(buffer));

    // Pulling EN low enables display output
    digitalWrite(Pin_EN, LOW);


    alarm_pool = alarm_pool_create(3, 1);
    // alarm_pool_add_repeating_timer_us(alarm_pool, -1000000/30/bits_pow2, flushDisplay, this, &alarm_timer);
  }
};

bool flushDisplay(repeating_timer_t* tmr) {
  OBERGRANSAD* display = reinterpret_cast<OBERGRANSAD*>(tmr->user_data);
  display->display();
  return true;
}

OBERGRANSAD myFirstLEDMatrix(
    14, 16, 15, 17 // Pins: CLK, DI, CLA, EN
);
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

void setup() {
  u8g2_for_adafruit_gfx.begin(myFirstLEDMatrix);

  myFirstLEDMatrix.fillScreen(0);
  myFirstLEDMatrix.display();

  Serial.begin(115200);
}

int16_t scrollX = 0;
void loop() {
  myFirstLEDMatrix.fillScreen(0);
  
  for(uint8_t i = 0; i < 16; i++) {
    myFirstLEDMatrix.drawFastHLine(0, (i+scrollX)%16, 16, i);
  }
  uint32_t m = millis()+500;
  while (m>millis()) {
  myFirstLEDMatrix.display();
  }
  scrollX++;
  // delay(500);
}