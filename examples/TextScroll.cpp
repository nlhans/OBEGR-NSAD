#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>

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
class OBERGRANSAD : public Adafruit_GFX {
protected:
  static const uint32_t PX = 16 * 16;

  const uint8_t Pin_CLK;
  const uint8_t Pin_DI;
  const uint8_t Pin_CLA;
  const uint8_t Pin_EN;

  // The last buffer position is for dummy writes when a pixel is out of bounds.
  uint8_t buffer[PX+1];

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
    buffer[mapXY(15-x,y)] = color != 0;
  
  }
public:

// https://www.tme.eu/Document/8876e3da3e0cc25d8b4c7cdeea8b8a88/SCT2024.pdf
void display() {
  digitalWrite(Pin_CLA, LOW);
  delayMicroseconds(1);
  for (uint_fast8_t i = 0; i < 16; i++) {
    for (uint_fast8_t j = 0; j < 16; j++) {
      digitalWrite(Pin_DI, buffer[i*16+j]>0?HIGH:LOW);
      delayMicroseconds(1);
      digitalWrite(Pin_CLK, HIGH);
      delayMicroseconds(1);
      digitalWrite(Pin_CLK, LOW);
      delayMicroseconds(1);
    }
    delayMicroseconds(1);
  }
  digitalWrite(Pin_CLA, HIGH);
  delayMicroseconds(1);
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

    display();
  }
};



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
  
  u8g2_for_adafruit_gfx.setForegroundColor(1); // on
  u8g2_for_adafruit_gfx.setBackgroundColor(0); // off
  u8g2_for_adafruit_gfx.setFont(u8g2_font_5x7_mf);
  u8g2_for_adafruit_gfx.setFontDirection(3); //left-to-right
  u8g2_for_adafruit_gfx.setFontMode(0); // non-transparant 
  u8g2_for_adafruit_gfx.setCursor(15, scrollX);
  u8g2_for_adafruit_gfx.print("Fablab Oldenzaal");
  u8g2_for_adafruit_gfx.setCursor(7, 80-scrollX);
  u8g2_for_adafruit_gfx.print("ASSORTIMENS");

  scrollX ++;
  if (scrollX > 100) {
    scrollX = 0;
  }
  myFirstLEDMatrix.display();
  
  delay(50);
}