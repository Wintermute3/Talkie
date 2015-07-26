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

#pragma once

// ---------------------------------------------------------------------------
// Revision history:
//
//   27-Apr-2015: M. Nagy - initial modifications.
//   16-May-2015: M. Nagy - add support for reading datastreams from sd card. 
// ---------------------------------------------------------------------------

#define TALKIE_H_VERSION "1.505.16" // Y.YMM.DD

// ---------------------------------------------------------------------------
// Filenames on the sd card.
// ---------------------------------------------------------------------------

#define TALKIE_NDX "talkie.ndx"
#define TALKIE_DAT "talkie.dat"

// ---------------------------------------------------------------------------
// We depend on the SD library to read datastreams from sd cards.  Any sketch
// which uses this library will also need to first include the <SD.h> header
// file.
// ---------------------------------------------------------------------------

#include <SD.h>

// ---------------------------------------------------------------------------
// Class definition.
// ---------------------------------------------------------------------------

class Talkie {
  public:
         Talkie (                     ); // class initializer
    void mute   (                     ); // silence synth
    void say    (const byte * Address ); // speech data from buffer
    byte sayCard(const char * WordName); // speech data from ndx/dat files on sd card
  private:
    const byte *  ptrAddr  ; // byte cursor into speech datastream
          byte    ptrBit   ; // bit  cursor into speech datastream
          boolean spSetup  ; // flag to indicate timer registers initialized
          File    sdFile   ; // defined while sayCard() active
    unsigned long sdOffset ; // offset into dat file on sd card
    byte revBits(byte bits); // reverse bits in a byte
    byte getBits(byte bits); // get the next 1..8 bits from the datastream
    void sayRaw (void     ); // low-level speech routine
};

// ---------------------------------------------------------------------------
// Possible return codes from sayCard():
// ---------------------------------------------------------------------------

#define TALKIE_OK           0
#define TALKIE_ERR_NDX_OPEN 1 // ndx file not found
#define TALKIE_ERR_DAT_OPEN 2 // dat file not found
#define TALKIE_ERR_NDX_WORD 3 // word not found in ndx file
#define TALKIE_ERR_DAT_SEEK 4 // bad offset into dat file

// ---------------------------------------------------------------------------
// End.
// ---------------------------------------------------------------------------
