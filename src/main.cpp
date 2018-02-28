#include <Arduino.h>

// Control

#include <Encoder.h>
const int ENCODER_PIN_BUTTON = 16;
const int ENCODER_PIN_A = 17;
const int ENCODER_PIN_B = 18;
pinMode(14, OUT);
pinMode(15, OUT);
digitalWrite(14, LOW);
digitalWrite(15, HIGH);
pinMode(ENCODER_PIN_BUTTON, INPUT_PULLUP);
Encoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);

// LED
#include <WS2812Serial.h>
const int numled = 64;
const int pin = 10;
byte drawingMemory[numled * 3];          //  3 bytes per LED
DMAMEM byte displayMemory[numled * 12];  // 12 bytes per LED
WS2812Serial leds(numled, displayMemory, drawingMemory, pin, WS2812_GRB);

#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define YELLOW 0xFFFF00
#define PINK 0xFF1088
#define ORANGE 0xE05800
#define WHITE 0xFFFFFF
int currentColor = BLUE;
bool currentState[numled];

// Setup
void setup() {
  delay(200);
  Serial.begin(115200);

  leds.begin();
  delay(10);
}

void loop() {
  // Read Encoder
  static long encoderPosition = 0;
  long newPosition = encoder.read();
  if (newPosition != encoderPosition) {
    Serial.println(newPostion);
    encoderPosition = newPosition;
  }

  // Update LEDs
  for (int i = 0; i < leds.numPixels(); i++) {
    if (currentState[i]) {
      leds.setPixel(i, currentColor);
    } else {
      leds.setPixel(i, 0);
    }
  }
  leds.show();
}
