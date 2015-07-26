#!/usr/bin/env python

#---------------------------------------------------------------------------------------
# make_dictionary.py                                                         15.051.91
# Copyright 2015 Michael Nagy
# This code is released under GPLv2 license.
#
# Windows:
#
#    python make_dictionary.py
#
# Linux/Mac:
#
#    chmod +x make_dictionary.py
#    ./make_dictionary.py
#
# This program generates pairs of binary talkie.dat (data) and talkie.ndx (index) files
# containing speech datastreams suitable for rendering using the talkie library.  The
# files are expected to be stored in the root directory of an SD memory card where the
# talkie library can find them.
#
# This python program reads all the word files matching LoadFilePattern.  Each should
# contain one or more named definitions of speech datastreams which can individually
# vary in length from around a dozen to many thousands of bytes in length.
#
# When it reads word files it expects to find precisely-formatted blocks of lines of the
# form:
#
#   const byte spFAILURE [] PROGMEM = {
#     0x0C,...
#   };
#
# Such blocks are output from the companion standard_format.py program.  Those files are
# conventional C language header files, and as such may also be included directly by C
# programs as an alternative to processing them into a binary dictionary.  Refer to the
# comments in the standard_format.py program for additional information.
#---------------------------------------------------------------------------------------

from __future__ import print_function

import os, sys, glob

#---------------------------------------------------------------------------------------
# Globals.
#---------------------------------------------------------------------------------------

LoadFilePattern = 'standard/*.h'

First = '_us_' # or '_uk_'
Final = '_uk_' # or '_us_'

DatFileName = 'talkie.dat'
NdxFileName = 'talkie.ndx'

DatFileSize = 0 # tracks data file size as it is created
NdxFileSize = 0 # tracks index file size as it is created

Histogram = {} # tracks how many datastreams of various sizes are processed

NameList = [] # tracks unique datastream names
WordList = {} # buffers all name,datastream tuples during processing

#---------------------------------------------------------------------------------------
# Add a single word to NameList and the WordList dictionary.
#---------------------------------------------------------------------------------------

def AddWord(Word):
  L1 = Word.split('=')
  L2 = L1[1].strip().split('{')[1].strip()
  Name = L1[0].split()[2]
  Data = L2.split('}')[0].strip().split(',')
  Name = Name[2:].lower()
  if not Name in NameList:
    NameList.append(Name)
    WordList[Name] = Data

#---------------------------------------------------------------------------------------
# Load all words from a single C header-style word file.
#---------------------------------------------------------------------------------------

def LoadWords(WordFileName):
  with open(WordFileName) as f:
    for Chunk in f:
      Chunk = Chunk.strip()
      if len(Chunk):
        if Chunk.startswith('const'):
          Word = Chunk
        elif Chunk.startswith('0x'):
          Word += Chunk
        elif Chunk.startswith('};'):
          Word += Chunk
          AddWord(Word)

#---------------------------------------------------------------------------------------
# Save all words (from all word files) to the binary data and index output files.
#---------------------------------------------------------------------------------------

def SaveWords():
  global DatFileSize, NdxFileSize
  with open(DatFileName, 'wb') as DatFile:
    with open(NdxFileName, 'wb') as NdxFile:
      for Name in sorted(NameList):
        Data = WordList[Name]
        try:
          DatBuffer = bytearray(x.decode('hex') for x in [h[2:] for h in Data])
          NdxBuffer = bytearray()
          NdxBuffer.extend('%s:%d:%d\n' % (Name, len(DatBuffer), DatFileSize))
          NdxFile.write(NdxBuffer)
          DatFile.write(DatBuffer)
          DatLength = len(DatBuffer)
          NdxLength = len(NdxBuffer)
          DatFileSize += DatLength
          NdxFileSize += NdxLength
          if DatLength in Histogram:
            Histogram[DatLength] += 1
          else:
            Histogram[DatLength] = 1
        except:
          print('  *** Name [%s] exception:' % Name)
          print('    %s' % (Data))

#---------------------------------------------------------------------------------------
# Scan the current folder for word files matching LoadFilePattern and load all the
# words in each one, then save them all to the binary output files.
#---------------------------------------------------------------------------------------

print()
List1 = []
List2 = []
List3 = []
print('First: %s' % (First))
print('Final: %s' % (Final))
print()
for WordFileName in glob.glob(LoadFilePattern):
  if First in WordFileName:
    List1.append(WordFileName)
  elif Final in WordFileName:
    List3.append(WordFileName)
  else:
    List2.append(WordFileName)
for WordFileName in List1+List2+List3:
  print('Loading words from: %s' % (WordFileName))
  LoadWords(WordFileName)
SaveWords()
print()
print('Histogram of word bytestream lengths:')
print()
Summary = ''
for Length in sorted(Histogram):
  Summary += '  [%5d: %2d]' % (Length, Histogram[Length])
  if len(Summary) > 100:
    print('%s' % (Summary))
    Summary = ''
if Summary:
  print('%s' % (Summary))
print()
print('Output %d words to %s (%5d bytes).' % (len(NameList), NdxFileName, NdxFileSize))
print('Output %d words to %s (%5d bytes).' % (len(NameList), DatFileName, DatFileSize))
print()

#---------------------------------------------------------------------------------------
# End
#---------------------------------------------------------------------------------------
