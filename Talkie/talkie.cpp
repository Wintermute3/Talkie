// ---------------------------------------------------------------------------
// Talkie library
// Copyright 2011 Peter Knight
// This code is released under GPLv2 license.
//
// Modified by Michael Nagy to work as a standard Arduino library
// for boards based on the Atmel ATmega32U4 processor (in particular,
// the Pololu A-Star 32U4 Prime).  This involved adding const qualifiers
// to various pointer variables and parameters, remapping timer 2 (which
// doesn't exist on the 32U4) to timer 3, and remapping output OC2B to
// OC3A (PC6, Arduino digital pin 5).  Also converted variable types
// to standard Arduino types (i.e. uint8_t -> byte).
// ---------------------------------------------------------------------------

#include "Arduino.h"
#include "talkie.h"

// ---------------------------------------------------------------------------
// Revision history:
//
//   27-Apr-2015: M. Nagy - initial modifications.
//   16-May-2015: M. Nagy - add support for reading datastreams from sd card. 
// ---------------------------------------------------------------------------

#define TALKIE_CPP_VERSION "1.505.16" // Y.YMM.DD

// ---------------------------------------------------------------------------
// Globals which define the speech synthesizer parameters for a single 25ms period.
// ---------------------------------------------------------------------------

static byte  synthPeriod;
static word  synthEnergy;
static short synthK1, synthK2;
static char  synthK3, synthK4, synthK5, synthK6, synthK7, synthK8, synthK9, synthK10;

// ---------------------------------------------------------------------------
// Constants which determine the behavior of the speech synthesizer engine.
// ---------------------------------------------------------------------------

#define FS 8000 // speech engine sample rate

static byte  tmsEnergy[0x10] = {
  0x00,0x02,0x03,0x04,0x05,0x07,0x0a,0x0f,0x14,0x20,0x29,0x39,0x51,0x72,0xa1,0xff
};
static byte  tmsPeriod[0x40] = {
  0x00,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,
  0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2D,0x2F,0x31,
  0x33,0x35,0x36,0x39,0x3B,0x3D,0x3F,0x42,0x45,0x47,0x49,0x4D,0x4F,0x51,0x55,0x57,
  0x5C,0x5F,0x63,0x66,0x6A,0x6E,0x73,0x77,0x7B,0x80,0x85,0x8A,0x8F,0x95,0x9A,0xA0
};
static short tmsK1[0x20] = {
  0x82C0,0x8380,0x83C0,0x8440,0x84C0,0x8540,0x8600,0x8780,0x8880,0x8980,0x8AC0,0x8C00,
  0x8D40,0x8F00,0x90C0,0x92C0,0x9900,0xA140,0xAB80,0xB840,0xC740,0xD8C0,0xEBC0,0x0000,
  0x1440,0x2740,0x38C0,0x47C0,0x5480,0x5EC0,0x6700,0x6D40
};
static short tmsK2[0x20] = {
  0xAE00,0xB480,0xBB80,0xC340,0xCB80,0xD440,0xDDC0,0xE780,0xF180,0xFBC0,0x0600,0x1040,
  0x1A40,0x2400,0x2D40,0x3600,0x3E40,0x45C0,0x4CC0,0x5300,0x5880,0x5DC0,0x6240,0x6640,
  0x69C0,0x6CC0,0x6F80,0x71C0,0x73C0,0x7580,0x7700,0x7E80
};
static char  tmsK3[0x10] = {
  0x92,0x9F,0xAD,0xBA,0xC8,0xD5,0xE3,0xF0,0xFE,0x0B,0x19,0x26,0x34,0x41,0x4F,0x5C
};
static char  tmsK4[0x10] = {
  0xAE,0xBC,0xCA,0xD8,0xE6,0xF4,0x01,0x0F,0x1D,0x2B,0x39,0x47,0x55,0x63,0x71,0x7E
};
static char  tmsK5[0x10] = {
  0xAE,0xBA,0xC5,0xD1,0xDD,0xE8,0xF4,0xFF,0x0B,0x17,0x22,0x2E,0x39,0x45,0x51,0x5C
};
static char  tmsK6[0x10] = {
  0xC0,0xCB,0xD6,0xE1,0xEC,0xF7,0x03,0x0E,0x19,0x24,0x2F,0x3A,0x45,0x50,0x5B,0x66
};
static char  tmsK7[0x10] = {
  0xB3,0xBF,0xCB,0xD7,0xE3,0xEF,0xFB,0x07,0x13,0x1F,0x2B,0x37,0x43,0x4F,0x5A,0x66
};
static char  tmsK8[0x08] = {
  0xC0,0xD8,0xF0,0x07,0x1F,0x37,0x4F,0x66
};
static char  tmsK9[0x08] = {
  0xC0,0xD4,0xE8,0xFC,0x10,0x25,0x39,0x4D
};
static char  tmsK10[0x08] = {
  0xCD,0xDF,0xF1,0x04,0x16,0x20,0x3B,0x4D
};

#define CHIRP_SIZE 41

static char chirp[CHIRP_SIZE] = {
  0x00,0x2a,0xd4,0x32,0xb2,0x12,0x25,0x14,0x02,0xe1,0xc5,0x02,0x5f,0x5a,0x05,0x0f,
  0x26,0xfc,0xa5,0xa5,0xd6,0xdd,0xdc,0xfc,0x25,0x2b,0x22,0x21,0x0f,0xff,0xf8,0xee,
  0xed,0xef,0xf7,0xf6,0xfa,0x00,0x03,0x02,0x01
};

// ---------------------------------------------------------------------------
// The ROMs used with the TI speech were serial, not byte wide.  Here's a handy
// routine to flip ROM data which is usually reversed.
// ---------------------------------------------------------------------------

byte Talkie::revBits(byte bits) {
  bits = ( bits         >> 4) | ( bits         << 4); // Swap groups of 4: 76543210 -> 32107654
  bits = ((bits & 0xcc) >> 2) | ((bits & 0x33) << 2); // Swap groups of 2: 32107654 -> 10325476
  bits = ((bits & 0xaa) >> 1) | ((bits & 0x55) << 1); // Swap groups of 1: 10325476 -> 01234567
  return bits;
}

// ---------------------------------------------------------------------------
// Read and return from 1 to 6 bits from the speech datastream.  Bits may
// span byte boundardies in the datastream.  The current position in the
// datastream is defined by the globals ptrAddr and ptrBit when reading from
// program memory (sdFile closed), and by sdOffset and ptrBit when reading
// from sd card (sdFile open).
// ---------------------------------------------------------------------------

byte Talkie::getBits(byte bits) {
  word data;
  if (sdFile) {
    data = revBits(sdFile.peek()) << 8;
    if (ptrBit + bits >= 8) {
      sdFile.read();
    }
    if (ptrBit + bits > 8) {
      data |= revBits(sdFile.peek());
    }
  } else {
    data = revBits(pgm_read_byte(ptrAddr)) << 8;
    if (ptrBit + bits > 8) {
      data |= revBits(pgm_read_byte(ptrAddr + 1));
    }
    if (ptrBit + bits >= 8) {
      ptrAddr++;
    }
  }
  data <<= ptrBit;
  byte value = data >> (16-bits);
  ptrBit += bits; // advance by 1..6 bits
  if (ptrBit >= 8) {
    ptrBit -= 8;
  }
  return value;
}

// ---------------------------------------------------------------------------
// Stop timers to silence.
// ---------------------------------------------------------------------------

void Talkie::mute() {
  TCCR3A = TCCR3B = 0;
  TCCR1A = TCCR1B = 0;
  spSetup = false;
}

// ---------------------------------------------------------------------------
// Enable the speech system whenever sayRaw() is called.  Timer 3 is set up
// as a 62500Hz PWM.  The PWM 'buzz' is well above human hearing range
// and is very easy to filter out.  Say a word or phrase, and return
// after it is completely sounded.  This routine is a loop which, every
// 25ms (1/40th sec), processes the next variable-length speech-data
// frame and calculates all necessary synth parameters which will apply
// for the next 25ms.  Set up 16-bit timer 1 to generate interrupts 8000
// times per second.
// ---------------------------------------------------------------------------

// TCCR3A = COM3A1  COM3A0  COM3B1  COM3B0  COM3C1  COM3C0  WGM31  WGM30
//          1       0       0       0       0       0       0      1
// TCCR3B = ICNC3   ICES3   -       WGM33   WGM32   CS32    CS31   CS30
//          0       0               0       1       0       0      1
//
//   WGM = 0101 = Fast PWM  8-bit, 0x00FF, TOP, TOP
//   COM3A = 10 = Clear OC3A on compare match, set OC3A at TOP
//   CS3 = 001 = CLKIO/1 (no prescaling)
//
// 16MHz / 256 (8-bit) = 62.5KHz

void Talkie::sayRaw() {
  byte energy;
  if (spSetup == false) {
    TCCR3A = _BV(COM3A1) | _BV(WGM30 ); // T3: Fast PWM, 8-bit, clear OC3A on compare match (set output to low level)
    TCCR3B = _BV(CS30  )              ; // T3: Fast PWM, 8-bit, CLKIO/1 (no prescaling)
    TIMSK3 = 0                        ; // T3: all interrupts disabled
    TCCR1A = 0                        ; // T1: CTC mode, no physical output pin enabled
    TCCR1B = _BV(WGM12) | _BV(CS10)   ; // T1: no clock source (timer/counter stopped)
    TCNT1  = 0                        ; // T1: initial count 0
    OCR1A  = F_CPU / FS               ; // T1: 16MHz / 8000 = 2000
    TIMSK1 = _BV(OCIE1A)              ; // T1: enable TIMER1_COMPA_vect interrupt on OCR1
    sei();
    spSetup = true;
  }
  ptrBit = 0;
  do {
    byte repeat;
    energy = getBits(4);
    if (energy == 0) { // energy = 0: rest frame
      synthEnergy = 0;
    } else if (energy == 0xf) { // energy = 15: stop frame, silence the synthesizer.
      synthEnergy = 0;
      synthK1  = 0;
      synthK2  = 0;
      synthK3  = 0;
      synthK4  = 0;
      synthK5  = 0;
      synthK6  = 0;
      synthK7  = 0;
      synthK8  = 0;
      synthK9  = 0;
      synthK10 = 0;
    } else {
      synthEnergy = tmsEnergy[energy];
      repeat = getBits(1);
      synthPeriod = tmsPeriod[getBits(6)];
      // A repeat frame uses the last coefficients
      if (!repeat) { // All frames use the first 4 coefficients
        synthK1 = tmsK1[getBits(5)];
        synthK2 = tmsK2[getBits(5)];
        synthK3 = tmsK3[getBits(4)];
        synthK4 = tmsK4[getBits(4)];
        if (synthPeriod) { // Voiced frames use 6 extra coefficients.
          synthK5  = tmsK5 [getBits(4)];
          synthK6  = tmsK6 [getBits(4)];
          synthK7  = tmsK7 [getBits(4)];
          synthK8  = tmsK8 [getBits(3)];
          synthK9  = tmsK9 [getBits(3)];
          synthK10 = tmsK10[getBits(3)];
        }
      }
    }
    delay(25);
  } while (energy != 0xf);
}

// ---------------------------------------------------------------------------
// Initialize class instance.
// ---------------------------------------------------------------------------

Talkie::Talkie() { 
  mute();
  pinMode(5, OUTPUT); // OC3A == PC6 == Arduino digital pin 5
}

// ---------------------------------------------------------------------------
// Say a word or phrase from program memory.
// ---------------------------------------------------------------------------

void Talkie::say(const byte * Addr) {
  ptrAddr = Addr;
  sayRaw();
}

// ---------------------------------------------------------------------------
// Say a word or phrase from the sd card.  This involves scanning the index
// file for the target word, setting sdOffset to the offset value for the
// word, and calling sayRaw() with the data file open.
// ---------------------------------------------------------------------------

#define STATE_MATCHING 0 // matching WordName against start of line
#define STATE_NOMATCH  1 // failed match, seeking end of line
#define STATE_MATCHED  2 // matched WordName, seeking offset
#define STATE_OFFSET   3 // reading offset
#define STATE_SUCCESS  4 // offset determined, ready to say word

byte Talkie::sayCard(char const * WordName) {
  pinMode(17, OUTPUT);
  SD.begin(4);
  if (sdFile = SD.open(TALKIE_NDX)) {
    byte WordOffset = 0;
    byte State = STATE_MATCHING;
    while (sdFile.available() && (State != STATE_SUCCESS)) {
      char x = sdFile.read();
      switch (State) {
        case STATE_MATCHING:
          if (WordName[WordOffset]) {
            if (WordName[WordOffset++] != x) {
              State = STATE_NOMATCH;
            }
          } else {
            if (':' == x) {
              State = STATE_MATCHED;
            } else {
              State = STATE_NOMATCH;
            }
          }
          break;
        case STATE_MATCHED:
          if (':' == x) {
            State = STATE_OFFSET;
            sdOffset = 0;
          }
          break;
        case STATE_OFFSET:
          if ('\n' == x) {
            State = STATE_SUCCESS;
          } else {
            sdOffset *= 10;
            sdOffset += x - '0';
          }
          break;
        case STATE_NOMATCH:
          if ('\n' == x) {
            WordOffset = 0;
            State = STATE_MATCHING;
          }
          break;
      }
    }
    sdFile.close();
    if (State == STATE_SUCCESS) {
      if (sdFile = SD.open(TALKIE_DAT)) {
        if (sdFile.seek(sdOffset)) {
          sayRaw();
          sdFile.close();
          return TALKIE_OK;
        }
        sdFile.close();
        return TALKIE_ERR_DAT_SEEK;
      }
      return TALKIE_ERR_DAT_OPEN;
    }
    return TALKIE_ERR_NDX_WORD;
  }
  return TALKIE_ERR_NDX_OPEN;
}

// ---------------------------------------------------------------------------
// Interrupt Service Routine for Timer 1 (8000 Hz)
//
// Note: This interrupt service routine has access only to global
// values declared in this file, not to class member values.
// 
// When enabled, we expect to get periodic interrupts at a rate
// of 8000 per second.  Immediately output the value pre-calculated
// by the previous call, then calculate the value we will need on
// the next call using the global sync parameters.
// ---------------------------------------------------------------------------

ISR(TIMER1_COMPA_vect) {
  static byte nextPwm, periodCounter;
  static short x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10;
  short u0, u1, u2, u3, u4, u5, u6, u7, u8, u9, u10;

  OCR3A = (byte) nextPwm; // set PWM register

  sei();
  if (synthPeriod) { // Voiced source
    if (periodCounter < synthPeriod) {
      periodCounter++;
    } else {
      periodCounter = 0;
    }
    if (periodCounter < CHIRP_SIZE) {
      u10 = ((chirp[periodCounter]) * (unsigned long) synthEnergy) >> 8;
    } else {
      u10 = 0;
    }
  } else { // Unvoiced source
    static word synthRand = 1;
    synthRand = (synthRand >> 1) ^ ((synthRand & 1) ? 0xB800 : 0);
    u10 = (synthRand & 1) ? synthEnergy : -synthEnergy;
  }
  // Lattice filter forward path
  u9 = u10 - (((short) synthK10 * x9) >>  7);
  u8 = u9  - (((short) synthK9  * x8) >>  7);
  u7 = u8  - (((short) synthK8  * x7) >>  7);
  u6 = u7  - (((short) synthK7  * x6) >>  7);
  u5 = u6  - (((short) synthK6  * x5) >>  7);
  u4 = u5  - (((short) synthK5  * x4) >>  7);
  u3 = u4  - (((short) synthK4  * x3) >>  7);
  u2 = u3  - (((short) synthK3  * x2) >>  7);
  u1 = u2  - (((long ) synthK2  * x1) >> 15);
  u0 = u1  - (((long ) synthK1  * x0) >> 15);

  // Output clamp
  if (u0 >  511) u0 =  511;
  if (u0 < -512) u0 = -512;
  
  // Lattice filter reverse path
  x9 = x8 + (((short) synthK9 * u8) >>  7);
  x8 = x7 + (((short) synthK8 * u7) >>  7);
  x7 = x6 + (((short) synthK7 * u6) >>  7);
  x6 = x5 + (((short) synthK6 * u5) >>  7);
  x5 = x4 + (((short) synthK5 * u4) >>  7);
  x4 = x3 + (((short) synthK4 * u3) >>  7);
  x3 = x2 + (((short) synthK3 * u2) >>  7);
  x2 = x1 + (((long ) synthK2 * u1) >> 15);
  x1 = x0 + (((long ) synthK1 * u0) >> 15);
  x0 = u0;

  nextPwm = (u0 >> 2) + 0x80;
}

// ---------------------------------------------------------------------------
// End.
// ---------------------------------------------------------------------------
