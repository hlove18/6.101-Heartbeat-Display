/************************************************************************
 * 6.101 Heartbeat Display
 * Professor: Gim Hom
 * TA: Henry Love
 *
 * Written: Jan. 9, 2019
 * Author: Henry Love (hlove@mit.edu)
 * 
 * Comments:
 * Code adapted from: https://learn.adafruit.com/tap-tempo-trinket/code
 ************************************************************************/

// Libraries
#include <ST7735_t3.h>      // Hardware-specific library

// this Teensy3 native optimized version requires specific pins
#define TFT_SCLK 13  // SCLK can also use pin 14
#define TFT_MOSI 11  // MOSI can also use pin 7
#define TFT_CS   10  // CS & DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23
#define TFT_DC    9  // but certain pairs must NOT be used: 2+10, 6+9, 20+23, 21+22
#define TFT_RST   8  // RST can use any pin
#define SD_CS     4  // CS for SD card, can use any pin

// Instantiate tft
ST7735_t3 tft = ST7735_t3(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

int heartBeat = 14;           // Digital pin 14 has a pushbutton (or your heartbeat) attached to it
unsigned long bpm;            // Instantaneous bpm
unsigned long averageBpm;     // Average bpm
int prevState;                // Value of last digitalRead()
unsigned long prevBeat = 0L;  // Time of last button tap/heartbeat
int windowSize = 5;           // Moving average window size; max is 20
int pastBpm[20] = {};         // Array of past beat rates (for moving average with max window size of 20); behaves like a queue
int arrayIndex = 0;           // Array index
int arraySum = 0;             // Array sum
bool displayFlag = true;      // To prevent display strobing
bool debugging = false;       // To print debugging information to serial monitor

// The setup routine runs once when you press reset:
void setup() {
  Serial.begin(9600);                 // Initialize serial communication at 9600 bits per second
  pinMode(SD_CS, INPUT_PULLUP);       // Don't touch the SD card
  pinMode(heartBeat, INPUT);          // Make the button/heartbeat pin an input
  prevState = digitalRead(heartBeat); // Initialize state
  tft.initR(INITR_144GREENTAB);       // Use this initializer for green tab 1.44" TFT
  tft.fillScreen(ST7735_BLACK);       // Clear TFT display
  delay(500);
  // Draw heart
  tft.fillScreen(ST7735_BLACK);
  tft.fillCircle(54, 30, 10, ST7735_RED);
  tft.fillCircle(74, 30, 10, ST7735_RED);
  tft.fillTriangle(44, 34, 84, 34, 64, 54, ST7735_RED);
  // Write welcome text
  tft.setCursor(20, 80);
  tft.setTextColor(ST7735_RED);
  tft.setTextSize(2);
  tft.println("Welcome!");
  tft.setCursor(20, 100);
  tft.setTextSize(1);
  tft.println("6.101 heartbeat");
  tft.setCursor(45, 110);
  tft.println("display");
  delay(2000);
}

// Waits for change in button/heartbeat state and returns time of change
static unsigned long debounce() {
  uint8_t b;
  unsigned long start, last;
  long d;
 
  start = micros();
 
  for (;;) {
    last = micros();
    while ((b = digitalRead(heartBeat)) != prevState) { // State changed?
      if ((micros() - last) > 25000L) {                 // Sustained > 25 mS?
        prevState = b;                                  // Save new state
        displayFlag = true;                             // Toggle display flag
        return last;                                    // Return time of change
      }
    }                                                   // Else button/heartbeat unchanged...do other things...
    d = (last - start) - 4000000L;                      // Function start time minus 4 sec
    if (d > 0) {                                        // > 4 sec with no button/heartbeat change?
      pastBpm[0] = {};                                  // Reset bpm array
      prevBeat = 0L;                                    // Reset prevBeat
      arrayIndex = 0;                                   // Reset arrayIndex
      arraySum = 0;                                     // Reset arraySum
    }
    // If no prior tap has been registered, program is waiting
    // for initial tap.  If human is connected, call 911.
    if (!prevBeat) {
      if (debugging) {                                  // Print to serial monitor if in debugging mode
        Serial.println("Waiting for heartbeat...");
      }
      if (displayFlag) {                                // Check if display needs to be refreshed
        displayFlag = false;
        // Clear text
        tft.fillRoundRect(0, 75, 128, 50, 0, ST7735_BLACK);
        // Hollow heart
        tft.fillCircle(54, 30, 8, ST7735_BLACK);
        tft.fillCircle(74, 30, 8, ST7735_BLACK);
        tft.fillTriangle(47, 34, 81, 34, 64, 51, ST7735_BLACK);
        tft.fillRoundRect(46, 30, 37, 4, 0, ST7735_BLACK);
        // Text
        tft.setCursor(42, 80);
        tft.setTextColor(ST7735_RED);
        tft.setTextSize(2);
        tft.println("Dead");
        tft.setCursor(10, 100);
        tft.setTextSize(1);
        tft.println("or not connected...");
      }
    }
  }
}

void loop() {
  unsigned long t;
  t = debounce();                                 // Wait for new button/heartbeat state
  if (prevState == HIGH) {                        // Low-to-high (button tap/heartbeat)?
    if (prevBeat) {                               // Button tapped/heart pulsed before?
      bpm = 60000000L / (t - prevBeat);           // bpm
      if (arrayIndex < windowSize) {              // Append to pastBpm if number of samples is less than windowSize
        pastBpm[arrayIndex] = bpm;
        arrayIndex++;                             // Increment arrayIndex
        arraySum += bpm;                          // Increase arraySum
        }
      else {                                      // Otherwise, array is at capacity...
        arraySum -= pastBpm[0];                   // Subtract oldest sample from arraySum
        arraySum += bpm;                          // Add new sample to arraySum
        for (int i=0; i < windowSize - 1; i++) {  // Shift pastBpm values
          pastBpm[i] = pastBpm[i+1];
        }
        pastBpm[windowSize - 1] = bpm;            // Add new sample to pastBpm array
      }
      averageBpm = arraySum/arrayIndex;           // Calculate moving average
      if (debugging) {                            // Print to serial monitor if in debugging mode
        Serial.println("Array: ");
        for (int i=0; i < arrayIndex; i++) {
            Serial.println(pastBpm[i]);
        }
        Serial.print("Array Sum: ");
        Serial.println(arraySum);
        Serial.print("Number of Samples: ");
        Serial.println(arrayIndex);
        Serial.print("Avg BPM: ");
        Serial.println(averageBpm);
        Serial.println("#####################");
      }
      // Clear text
      tft.fillRoundRect(0, 75, 128, 50, 0, ST7735_BLACK);
      // Write text and center it
      if (averageBpm > 99) {
        tft.setCursor(48, 80);
      }
      else {
        tft.setCursor(54, 80);
      }
      tft.setTextColor(ST7735_RED);
      tft.setTextSize(2);
      tft.println(averageBpm);
    }
    else {                                        // First tap
      if (debugging) {                            // Print to serial monitor if in debugging mode
        Serial.println("Recognized initial tap...");
      }
      // Fill heart
      tft.fillCircle(54, 30, 8, ST7735_RED);
      tft.fillCircle(74, 30, 8, ST7735_RED);
      tft.fillTriangle(47, 34, 81, 34, 64, 51, ST7735_RED);
      tft.fillRoundRect(46, 30, 37, 4, 0, ST7735_RED);
      // Clear text
      tft.fillRoundRect(0, 75, 128, 50, 0, ST7735_BLACK);
      // Write text
      tft.setCursor(54, 80);
      tft.setTextColor(ST7735_RED);
      tft.setTextSize(2);
      tft.println("--");
    }
    prevBeat = t;                                 // Record time of last tap
  }
}
