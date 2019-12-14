// power_moon: Code for running a power moon christimas tree topper
//
// Designed to run on an Adafruit Gemma M0 connected to a single 
// vibration switch and a speaker. Outputs two-square wave tone
// music and lightshow using Gemma's onboar dotstar LED
//

#include <Adafruit_DotStar.h>
#include <SPI.h> 

#include "Tone1.h"
#include "pitches.h"
#include "fossil_falls.h"

#define TUNE_PIN1 0 // Music output A
#define TUNE_PIN2 2 // Music output B
#define VIBSWITCH_PIN 1 // Vibration-sensitive switch pin

// Song Control Data
int songA_index = -1;
int songB_index = -1;
uint8_t songA_stepsleft = 0;
uint8_t songB_stepsleft = 0;

// Setup for dotstar LED
#define DATAPIN 3 // as per Gemma M0
#define CLOCKPIN 4 // as per Gemma M0
Adafruit_DotStar strip = Adafruit_DotStar(1, DATAPIN, CLOCKPIN, DOTSTAR_BGR);
uint16_t hue = 0;

// Variables to track motion
int VibSwitchState = 0;
int event = 0;
int eventcount = 0;

void setup() {
  strip.begin();
  strip.show();
  pinMode(VIBSWITCH_PIN, INPUT_PULLUP);
}
 
void loop() {

  // Look for motion
  VibSwitchState = digitalRead(VIBSWITCH_PIN);
  if (VibSwitchState == LOW) {
    event = 1;
  }
  else {
    event = 0;
  }

  // If motion, cycle through songs/static colours
  if (event == 1) {
    strip.setPixelColor(0, 0, 0, 255);
    strip.show();

    // Play through tunes
    while (songA_index < musicA_count) {
      if (songA_stepsleft == 0) {
        songA_index++;
        hue = hue + 30000;
        if (songA_index < musicA_count) {
          songA_stepsleft = musicA_duration[songA_index];
          if (musicA_pitch[songA_index] > 0) {
            tone1(TUNE_PIN1, pitches[musicA_pitch[songA_index]]);
          }
          else {
            noTone1(TUNE_PIN1);
          }
        }
      }
      if (songB_stepsleft == 0) {
        songB_index++;
        if (songB_index < musicB_count) {
          songB_stepsleft = musicB_duration[songB_index];
          if (musicB_pitch[songB_index] > 0) {
            tone2(TUNE_PIN2, pitches[musicB_pitch[songB_index]]);
          }
          else {
            noTone2(TUNE_PIN2);
          }
        }
      }
      delay(17);
      songA_stepsleft--;
      songB_stepsleft--;
      strip.setPixelColor(0, strip.ColorHSV(hue));
      strip.show();
      hue = hue + 100;
    }

    // Reset for next time
    songA_index = -1;
    songB_index = -1;
    songA_stepsleft = 0;
    songB_stepsleft = 0;
    noTone1(TUNE_PIN1);
    noTone2(TUNE_PIN2);
    hue = 0;
  }
  else { // rainbow mode with no music
    strip.setPixelColor(0, strip.ColorHSV(hue));
    strip.show();
    delay(10);
    hue = hue + 100;
  }
}


