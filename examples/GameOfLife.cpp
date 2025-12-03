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

#define PX 16*16

// We add 1 px to buffer so we have a location for dummy writes
// (we perform a dummy write when pixel is off the display)
uint8_t buffer1[PX+1];
uint8_t buffer2[PX+1];

/** HELPER FUNCTIONS **/
uint32_t mapXY(int8_t x, int8_t y) {
  if (x < 0 || y < 0 || x > 15 || y > 15) return 256;

  // Wrap:
  // if (x < 0) x = 15;
  // if (y < 0) y = 15;
  // if (x > 15) x = 0;
  // if (y > 15) y = 0;
  
  uint8_t px = (y*16+x);

  uint8_t board = (px / 64);
  uint8_t boardPx = px % 64;
  uint8_t boardPx8 = px % 8;
  
  // Segment
  // A     B
  // D     C

  // V y, x >
  // 0-7          16-23
  // 15-8         31-24

  // 48-55        32-39
  // 63-56        47-40
  
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

  memset(buffer1, 0, sizeof(buffer1));
  memset(buffer2, 0, sizeof(buffer2));

  // Pulling EN low enables display output
  digitalWrite(PIN_EN, LOW);

  uint8_t x = 3;
  buffer1[mapXY(0,x+4)] = 1;

  buffer1[mapXY(1,x+5)] = 1;
  buffer1[mapXY(1,x+3)] = 1;

  buffer1[mapXY(4,x+3)] = 1;
  buffer1[mapXY(4,x+4)] = 1;
  buffer1[mapXY(4,x+5)] = 1;

  buffer1[mapXY(5,x+3)] = 1;
  buffer1[mapXY(5,x+4)] = 1;
  buffer1[mapXY(5,x+5)] = 1;

  buffer1[mapXY(6,x+4)] = 1;

  buffer1[mapXY(9,x+4)] = 1;

  buffer1[mapXY(10,x+3)] = 1;
  buffer1[mapXY(10,x+4)] = 1;
  buffer1[mapXY(10,x+5)] = 1;

  buffer1[mapXY(11,x+3)] = 1;
  buffer1[mapXY(11,x+4)] = 1;
  buffer1[mapXY(11,x+5)] = 1;

  buffer1[mapXY(14,x+3)] = 1;
  buffer1[mapXY(14,x+5)] = 1;

  buffer1[mapXY(15,x+4)] = 1;
}

/** GAME OF LIFE **/
uint_fast8_t countNeighbors(uint8_t* px, uint_fast8_t x, uint_fast8_t y) {
  uint_fast8_t neighbor = 
    px[mapXY(x-1, y-1)] + 
    px[mapXY(x-1, y)] + 
    px[mapXY(x-1, y+1)] + 
    
    px[mapXY(x, y-1)] + 
    px[mapXY(x, y+1)] + 

    px[mapXY(x+1, y-1)] + 
    px[mapXY(x+1, y)] + 
    px[mapXY(x+1, y+1)];
  return neighbor;
}

void update(uint8_t* from, uint8_t* to) {
  for (uint_fast8_t x = 0; x < 16; x++) {
    for (uint_fast8_t y = 0; y < 16; y++) {
      uint_fast8_t neighbors = countNeighbors(from, x, y);
      uint_fast8_t aliveOld = from[mapXY(x,y)];
      uint_fast8_t aliveNew = 0;
      if (aliveOld && neighbors < 2) aliveNew = 0;
      else if (aliveOld && (neighbors == 2 || neighbors == 3)) aliveNew = aliveOld;
      else if (aliveOld && (neighbors > 3)) aliveNew = 0;
      else if (!aliveOld && (neighbors == 3)) aliveNew = 1;
      to[mapXY(x,y)] = aliveNew;
    }
  }
}

void loop() {
  update(buffer1, buffer2);
  shiftPixels(buffer2);
  delay(500);

  update(buffer2, buffer1);
  shiftPixels(buffer1);
  delay(500);
}