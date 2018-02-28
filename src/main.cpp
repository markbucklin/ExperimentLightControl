#include <Arduino.h>

// Control
#include <Encoder.h>

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
  // Update LEDs
  for (int i = 0; i < leds.numPixels(); i++) {
    if (currentState[i]) {
      leds.setPixel(i, currentColor);
    } else {
      leds.setPixel(i, 0);
    }
  }
  leds.show();
  delayMicroseconds(1000);
}

// void handler(char **tokens, byte numtokens) {
//   Serial.print(numtokens);
//   Serial.print(" tokens: ");
//   for (int token = 0; token < numtokens; token++) {
//     if (token != 0) Serial.print(", ");
//     Serial.print(tokens[token]);
//   }
//   Serial.println();

//   switch (tokens[0][0]) {
//     case 'h': {
//       dev->println(
//           "led-on <num1> <num2> - turn LEDs <num1> and <num2> on (variable "
//           "argument number)");
//       dev->println("led-on - turn all LEDs on");
//       dev->println("led-off - turn all LEDs off");
//       break;
//     }
//     case 'a': {
//       if (numtokens < 2) return;
//       uint16_t value = atoi(tokens[1]);
//       Serial.print("* Sensor request to: ");
//       Serial.println(value);
//       break;
//       // if (argc == 1) {
//       //   memset(currentState, true, sizeof(bool) * numled);
//       // } else {
//       //   for (int i = 1; i < argc; i++) {
//       //     int ledNum = atoi(argv[i]);
//       //     currentState[ledNum - 1] = true;
//       //   }
//       // }
//     }
//   }
//   //   if (argc == 1) {
//   //   dev->println("must specify color");
//   //   // if (argc == 2) {
//   //   currentColor = atoi(argv[1]);
//   //   // }
//   // }
// }

//  CLI.setDefaultPrompt("> ");
//   CLI.onConnect(connectFunc);

//   CLI.addCommand("led-on", ledOnFunc);
//   CLI.addCommand("led-off", ledOffFunc);
//   CLI.addCommand("led-color", ledColorFunc);
//   CLI.addCommand("help", helpFunc);

//   CLI.addClient(Serial);

// CLI_COMMAND(ledOnFunc) {
//   if (argc == 1) {
//     memset(currentState, true, sizeof(bool) * numled);
//   } else {
//     for (int i = 1; i < argc; i++) {
//       int ledNum = atoi(argv[i]);
//       currentState[ledNum - 1] = true;
//     }
//   }
//   return 0;
// }

// CLI_COMMAND(ledOffFunc) {
//   if (argc == 1) {
//     memset(currentState, false, sizeof(bool) * numled);
//   } else {
//     for (int i = 1; i < argc; i++) {
//       int ledNum = atoi(argv[i]);
//       currentState[ledNum - 1] = false;
//     }
//   }
//   return 0;
// }

// CLI_COMMAND(ledColorFunc) {
//   if (argc == 1) {
//     dev->println("must specify color");
//     return 10;
//   } else {
//     // if (argc == 2) {
//     currentColor = atoi(argv[1]);
//     return 0;
//     // } else {
//     // }
//   }
// }

// CLI_COMMAND(helpFunc) {
//   dev->println(
//       "led-on <num1> <num2> - turn LEDs <num1> and <num2> on (variable "
//       "argument number)");
//   dev->println("led-on - turn all LEDs on");
//   dev->println("led-off - turn all LEDs off");
//   return 0;
// }

// CLI_COMMAND(connectFunc) {
//   dev->println("LED Control");
//   dev->println("Type 'help' to list commands.");
//   dev->println();
//   dev->printPrompt();
// }
