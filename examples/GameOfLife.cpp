#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>

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

  static const uint8_t bits = 7;
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

    bufferA[mapXY(x,y)] = gamma[color];
  }

  // https://www.tme.eu/Document/8876e3da3e0cc25d8b4c7cdeea8b8a88/SCT2024.pdf
  void display() __attribute__((optimize("-O3"))) {
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

  uint32_t getDepth() const {
    return bits_pow2;
  }

  uint32_t getBitDepth() const {
    return bits;
  }

  void show() {
    blank = true;
    for (size_t i = 0; i < sizeof(bufferA)/sizeof(bufferA[0]); i++) {
      bufferB[i] = bufferA[i];
    }
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



/** GAME OF LIFE **/
uint8_t buffer1[257];
uint8_t buffer2[257];
uint16_t getXY(int x, int y) {
  bool clipMode = true;
  if (clipMode) {
    if (x<0 || y<0 || x>15 || y>15) return 256; // out of bounds
    return y*16+x;
  } else {
    if (x < 0) x+=16;
    if (y < 0) y+=16;
    if (x > 15) x-=16;
    if (y > 15) y-=16;
    return y*16+x;
  }
}

uint_fast8_t countNeighbors(uint8_t* px, uint_fast8_t x, uint_fast8_t y) {
  uint_fast8_t neighbor = 
    px[getXY(x-1, y-1)] + 
    px[getXY(x-1, y)] + 
    px[getXY(x-1, y+1)] + 
    
    px[getXY(x, y-1)] + 
    px[getXY(x, y+1)] + 

    px[getXY(x+1, y-1)] + 
    px[getXY(x+1, y)] + 
    px[getXY(x+1, y+1)];
  return neighbor;
}

void update(uint8_t* from, uint8_t* to) {
  for (uint_fast8_t x = 0; x < 16; x++) {
    for (uint_fast8_t y = 0; y < 16; y++) {
      uint_fast8_t neighbors = countNeighbors(from, x, y);
      uint_fast8_t aliveOld = from[getXY(x,y)];
      uint_fast8_t aliveNew = 0;
      if (aliveOld && neighbors < 2) aliveNew = 0;
      else if (aliveOld && (neighbors == 2 || neighbors == 3)) aliveNew = aliveOld;
      else if (aliveOld && (neighbors > 3)) aliveNew = 0;
      else if (!aliveOld && (neighbors == 3)) aliveNew = 1;
      to[getXY(x,y)] = aliveNew;
    }
  }
}


void setup() {
  Serial.begin(115200);
  u8g2_for_adafruit_gfx.begin(myFirstLEDMatrix);

  // Display updater at 8kHz
  // 2**7 (7-bits grayscale) x 60Hz = 7680Hz.
  gpio_set_function(4, GPIO_FUNC_PWM); // Debug
  uint32_t _slice_num = pwm_gpio_to_slice_num(4);
  uint32_t f_sys = 125000000;
  pwm_set_clkdiv(_slice_num, 2); 
  pwm_set_wrap(_slice_num, f_sys / 60 / myFirstLEDMatrix.getDepth() / 2 - 1);
  pwm_set_chan_level(_slice_num, PWM_CHAN_A, 50); 
  pwm_set_enabled(_slice_num, true); // let's go!

  // set up interrupts
  pwm_set_irq_enabled(_slice_num, true);
  irq_add_shared_handler(PWM_IRQ_WRAP, (irq_handler_t) []() -> void{
    myFirstLEDMatrix.display();
    pwm_clear_irq( pwm_gpio_to_slice_num(4));
  }, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(PWM_IRQ_WRAP, true);

  // Setup game-of-life field
  memset(buffer1, 0, sizeof(buffer1));
  memset(buffer2, 0, sizeof(buffer2));

  uint8_t x = 3;
  buffer1[getXY(0,x+4)] = 1;

  buffer1[getXY(1,x+5)] = 1;
  buffer1[getXY(1,x+3)] = 1;

  buffer1[getXY(4,x+3)] = 1;
  buffer1[getXY(4,x+4)] = 1;
  buffer1[getXY(4,x+5)] = 1;

  buffer1[getXY(5,x+3)] = 1;
  buffer1[getXY(5,x+4)] = 1;
  buffer1[getXY(5,x+5)] = 1;

  buffer1[getXY(6,x+4)] = 1;

  buffer1[getXY(9,x+4)] = 1;

  buffer1[getXY(10,x+3)] = 1;
  buffer1[getXY(10,x+4)] = 1;
  buffer1[getXY(10,x+5)] = 1;

  buffer1[getXY(11,x+3)] = 1;
  buffer1[getXY(11,x+4)] = 1;
  buffer1[getXY(11,x+5)] = 1;

  buffer1[getXY(14,x+3)] = 1;
  buffer1[getXY(14,x+5)] = 1;

  buffer1[getXY(15,x+4)] = 1;
}

void loop() {
  update(buffer1, buffer2);
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      myFirstLEDMatrix.writePixel(x,y, buffer2[getXY(x,y)] ? 255 : 0);
    }
  }
  myFirstLEDMatrix.show();
  delay(500);

  update(buffer2, buffer1);
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      myFirstLEDMatrix.writePixel(x,y, buffer1[getXY(x,y)] ? 255 : 0);
    }
  }
  myFirstLEDMatrix.show();
  delay(500);
}