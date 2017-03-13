/*
  Family Mart Chime
  Arpeggio
  from http://fonte.me/arduino/#Arpeggio

  Plays arpeggios of chosen chords

  Requires an 8-ohm speaker on digital pin 8

  Copyright (c) 2012 Jeremy Fonte
  This code is released under the MIT license
  http://opensource.org/licenses/MIT
  code addapted to NEO: ChrisMicro
  code adapted to play other tune: dusjagr

  *************************************************************************************

  Hardware Platform: Attiny85
                                      ATTINY85 Pins
                                      =============
                                             _______
                                            |   U   |
  DEBUGLED / SYNC-OUT             reset/PB5-|       |- VCC
  soundprog / buttons ->  D3/A3         PB3-| ATTINY|- PB2    D2/A1  <- POTI_RIGHT / CV-IN
  POTI_LEFT / SYNC-IN ->  D4/A2         PB4-|   85  |- PB1    D1     -> PWM SOUND
                                        GND-|       |- PB0    D0     -> NEOPIXELS
                                            |_______|

*************************************************************************************

*/

#include "pitches.h"

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

// hardware description / pin connections
#define SPEAKERPIN      1
#define NEOPIXELPIN     0

#define POTI_RIGHT     A1
#define POTI_LEFT      A2
#define BUTTONS_ADC    A3

#define NUMPIXELS      8

// hardware calibration
#define Vbutton_left   380
#define Vbutton_right  300
#define Vbutton_both   240
#define Vcc            37 // 3.7 V for LiPo
#define Vdiv           26 // measure max Voltage on Analog In

// variables
uint16_t speedPoti;
uint16_t colorPoti;


Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXELPIN, NEO_GRB + NEO_KHZ800);

// fast pin access
#define AUDIOPIN (1<<SPEAKERPIN)
#define PINLOW (PORTB&=~AUDIOPIN)
#define PINHIGH (PORTB|=AUDIOPIN)

void playSound(long freq_Hz, long duration_ms)
{
  uint16_t n;
  uint32_t delayTime_us;
  uint32_t counts;

  delayTime_us = 1000000UL / (freq_Hz*colorPoti>>6);
  counts = duration_ms * 1000 / delayTime_us;

  for (n = 0; n < counts; n++)
  {
    PINHIGH;
    delayMicroseconds(delayTime_us);
    PINLOW;
    delayMicroseconds(delayTime_us);
  }
}

void setup()
{
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif

  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(40);
  pinMode(SPEAKERPIN, OUTPUT);

}

/*
  uint8_t getButton()

  return value:
  1: left button pressed
  2: right button pressed
  3: both buttons pressed
  ADC values
  no button:512, left:341, right:248, both:200
*/
uint8_t getButton()
{
  uint8_t button = 0;
  if (analogRead(BUTTONS_ADC) < Vbutton_left) button = 1;
  if (analogRead(BUTTONS_ADC) < Vbutton_right) button = 2;
  if (analogRead(BUTTONS_ADC) < Vbutton_both) button = 3;

  return button;
}

void displayBinrayValue(uint16_t value, uint32_t color)
{
  uint8_t n;
  for (n = 0; n < NUMPIXELS; n++)
  {
    if (value & (1 << n)) pixels.setPixelColor(n, color);
    //else pixels.setPixelColor(n,0); // off
  }
}

void setColorAllPixel(uint32_t color)
{
  uint8_t n;
  for (n = 0; n < NUMPIXELS; n++)
  {
    pixels.setPixelColor(n, color);
  }
}

void playMart(int notes[]);

// available chord to arpeggiate
//C major
int c[] = {
  NOTE_B4, NOTE_G4, NOTE_D4, NOTE_G4, NOTE_A4, NOTE_D5, PAUSE, NOTE_A4, NOTE_B4, NOTE_A4, NOTE_D4, NOTE_G4 
};

int pauseBetweenNotes = 80;
int noteDuration_ms = 150;

void playMart(int notes[])
{

  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote <= 11; thisNote++) {
    speedPoti = 1023 - analogReadScaled(POTI_RIGHT)>>2;
    colorPoti = analogReadScaled(POTI_LEFT)>>2;
    pauseBetweenNotes=speedPoti*110/100;
    noteDuration_ms = speedPoti;
    displayBinrayValue(notes[thisNote],Wheel(colorPoti));
    pixels.show(); // This sends the updated pixel color to the hardware.
    
    playSound( notes[thisNote], noteDuration_ms);
    setColorAllPixel(0); // pixels off
    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    delay(pauseBetweenNotes);
        
  }
  delay(pauseBetweenNotes*5);
}

// scaled the adc of the 8BitMixTape voltage divider
// to a return value in range: 0..1023
// Use Vcc / Vdiv as measured on your hardware
uint16_t analogReadScaled(uint8_t channel)
{
  uint16_t value = analogRead(channel);
  value = value * Vcc / Vdiv;
  if (value > 1023) value = 1023;
  return value;
}

void loop()
{
  //uint16_t speedPoti = analogReadScaled(POTI_LEFT);
  speedPoti = analogReadScaled(POTI_RIGHT)>>2;
  pauseBetweenNotes=speedPoti*110/100;
  noteDuration_ms = speedPoti;

  //list any chord sequence here by calling playArp(chord)
  playMart(c);

// wait for button
    while( getButton()==0);

}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

