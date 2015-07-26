// ---------------------------------------------------------------------------
// Test_sdCard.ino
// Copyright 2015 Michael Nagy
// This code is released under GPLv2 license.
//
// A a standard Arduino sketch for boards based on the Atmel ATmega32U4
// processor (in particular, the Pololu A-Star 32U4 Prime).
//
//  - if installed, remove the jumper shorting /cs to gnd
//  - install a connection from /cs to Arduino pin 4
//  - connect an audio amplifier or headphones between gnd and Arduino pin 5
//  - copy the talkie.ndx and talkie.dat file to the root of an sd card
//  - insert the sd card in the sd card socket on the A-Star board
//
// This demo assumes you have the A-Star LCD display installed.
// ---------------------------------------------------------------------------

#include "Arduino.h"

#include <AStar32U4Prime.h>
#include <AStar32U4PrimeLCD.h>

#include <SD.h> // required by talkie library
#include <talkie.h>

AStar32U4PrimeLCD lcd   ;
Talkie            talkie;

void test() {
  lcd.clear();
  lcd.print(F("diner"));
  byte x = talkie.sayCard("diner");
  lcd.gotoXY(0, 1);
  lcd.print(x);
  delay(1000);
}

void setup() {
  lcd.clear();
  lcd.print(F("Talkie"));
  lcd.gotoXY(0, 1);
  lcd.print(F(TALKIE_H_VERSION));
  delay(2000);
}

void loop() {
  while(1) {
    test();
  }
}

// ---------------------------------------------------------------------------
// End.
// ---------------------------------------------------------------------------
