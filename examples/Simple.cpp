#include <Arduino.h>

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
#define PIN_CLA   15
#define PIN_CLK   14
#define PIN_DI    16
#define PIN_EN    17

/** HELPER FUNCTIONS **/
uint8_t mapXY(int8_t x, int8_t y) {
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

// https://www.tme.eu/Document/8876e3da3e0cc25d8b4c7cdeea8b8a88/SCT2024.pdf
void shiftPixels(uint8_t* buffer) {
  digitalWrite(PIN_CLA, LOW);
  delayMicroseconds(1);
  for (uint_fast8_t i = 0; i < 16; i++) {
    for (uint_fast8_t j = 0; j < 16; j++) {
      digitalWrite(PIN_DI, buffer[i*16+j]>0?HIGH:LOW);
      delayMicroseconds(1);
      digitalWrite(PIN_CLK, HIGH);
      delayMicroseconds(1);
      digitalWrite(PIN_CLK, LOW);
      delayMicroseconds(1);
    }
    delayMicroseconds(1);
  }
  digitalWrite(PIN_CLA, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_CLA, LOW);
}

void setup() {
  pinMode(PIN_CLA, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_DI, OUTPUT);
  pinMode(PIN_EN, OUTPUT);

  // Pulling EN low enables display output
  digitalWrite(PIN_EN, LOW);
}

#define PX 16*16
void loop() {
  uint8_t buffer[PX];
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      buffer[mapXY(x,y)] = x%2;
    }
  }
  shiftPixels(buffer);
  delay(1000);
}