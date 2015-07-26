// ---------------------------------------------------------------------------
// Test_flash.ino
// Copyright 2011 Peter Knight
// This code is released under GPLv2 license.
//
// Modified by Michael Nagy to work as a standard Arduino library
// for boards based on the Atmel ATmega32U4 processor (in particular,
// the Pololu A-Star 32U4 Prime).
//
// Connect an audio amplifier between Arduino pin 5 and gnd.
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

#include <standard/vocab_us_large.h>

void sayNumberString(char* buf) {
  char digits[] = {'0','1','2','3','4','5','6','7','8','9','.','-'};
  char i, j, k;
  i = 0;
  do {
    k = buf[i];
    for (j = 0; j < 12; j++) { // find this input character in "digits"
      if (k == digits[j]) break;
    }
    switch (j) {
      case  0: talkie.say(spZERO ); break;
      case  1: talkie.say(spONE  ); break;
      case  2: talkie.say(spTWO  ); break;
      case  3: talkie.say(spTHREE); break;
      case  4: talkie.say(spFOUR ); break;
      case  5: talkie.say(spFIVE ); break;
      case  6: talkie.say(spSIX  ); break;
      case  7: talkie.say(spSEVEN); break;
      case  8: talkie.say(spEIGHT); break;
      case  9: talkie.say(spNINE ); break;
      case 10: talkie.say(spPOINT); break;
      case 11: talkie.say(spMINUS); break;
      default: ; // unrecognized character in string
    }
    i++;
  } while (buf[i]);
}

void test() {
  char v1[] =  "19.086";
  char v2[] = "-273.45";

  lcd.clear();
  lcd.print(F("The"   )); talkie.say(spTHE  );
  lcd.print(F(" wind" )); talkie.say(spWIND );
  lcd.gotoXY(0, 1);
  lcd.print(F("speed" )); talkie.say(spSPEED);
  lcd.print(F(" is"   )); talkie.say(spIS   );
  lcd.clear();
  lcd.print(v1); sayNumberString(v1);
  lcd.gotoXY(0, 1);
  lcd.print(F("knots.")); talkie.say(spKNOTS);
  delay(1000);

  lcd.clear();
  lcd.print(F("The"     )); talkie.say(spTHE        );
  lcd.print(F(" temp"   )); talkie.say(spTEMPERATURE);
  lcd.gotoXY(0, 1);
  lcd.print(F("is"      )); talkie.say(spIS         );
  lcd.print(F(" near"   )); talkie.say(spNEAR       );
  lcd.clear();
  lcd.print(v2); sayNumberString(v2);
  lcd.gotoXY(0, 1);
  lcd.print(F("degrees" )); talkie.say(spDEGREES    );
  lcd.gotoXY(0, 1);
  lcd.print(F("celsius.")); talkie.say(spCELCIUS    );
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
