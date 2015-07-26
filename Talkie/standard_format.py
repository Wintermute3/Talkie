#!/usr/bin/env python

#---------------------------------------------------------------------------------------
# standard_format.py                                                          15.042.81
# Copyright 2015 Michael Nagy
# This code is released under GPLv2 license.
#
# Windows:
#
#    python standard_format.py
#
# Linux/Mac:
#
#    chmod +x standard_format.py talkiedata_file.cpp
#    ./standard_format.py
#
# This program reads the files in the 'original' folder with extensions of .ino that
# were collected from the original Talkie project authored by going-digital.  To see
# that original project navigate to:
#
#     https://github.com/going-digital/Talkie
#
# It does its best to grab the speech datastream definitions from those .ino files,
# even those which are commented out, and outputs a parallel set of .h file into the
# current folder into which it emits precisely-formatted blocks of lines of the form:
#
#   const byte spFAILURE [] PROGMEM = {
#     0x0C,...
#   };
#
# During this process, if duplicate word definitions are encountered, only the first
# will be processed into the associated output file.
#
# One or more of these files may be included directly by C programs as an alternative
# to processing them into a binary dictionary, or such a dictionary may be created
# using the companion program make_dictionary.py program.  See the comments in that
# program for additional information.
#---------------------------------------------------------------------------------------

from __future__ import print_function

import os, sys, glob

#---------------------------------------------------------------------------------------
# Globals.
#---------------------------------------------------------------------------------------

LoadFilePattern = 'original/*.ino'
SaveFilePattern = 'standard/%s.h'

# lines are categorized as follows:
#
# comments starting with //
#   comment includes PROGMEM: process entire
#   else: pass
# blank lines: pass
# line includes PROGMEM:
#   line ends with }: process entire
#   else: process start
# line starts with 0x
#   line ends with }: process final
#   else: process middle

# We expect lines of the forms:
#
#   3:       byte spFAILURE[]  PROGMEM = {0x0C,...}
#   4:       byte spFAILURE [] PROGMEM = {0x0C,...}
#   4: const byte spFAILURE[]  PROGMEM = {0x0C,...}
#   5: const byte spFAILURE [] PROGMEM = {0x0C,...}

ChunkSize = 24 # max number of bytes per line in output

GlobalNameList = []
GlobalNameBytes = 0

NameList = []
NameBytes = 0
NameDups  = 0

#---------------------------------------------------------------------------------------
# And entire word definition has been collected into a single string.  Parse it and
# emit the canonical version to the output file.  Keep a few statistics along the way.
#---------------------------------------------------------------------------------------

def ProcessEntire(HdrFile, DataEntire):
  global GlobalNameList, GlobalNameBytes, NameList, NameDups, NameBytes
  L1 = DataEntire.split('=')
  L2 = L1[0].split()
  if L2[-1] == 'PROGMEM':
    L2 = L2[:-1]
  else:
    print('  *** Error1: %s' % (L2))
    os._exit(1)
  if L2[0] == 'const':
    L2 = L2[1:]
  if L2[0] == 'uint8_t':
    L2 = L2[1:]
  else:
    print('  *** Error2: %s' % (L2))
    os._exit(1)
  Name = L2[0]
  if len(L2) == 2:
    if not L2[1] == '[]':
      print('  *** Error3: %s' % (L2))
      os._exit(1)
  else:
    if len(L2) == 1:
      if Name.endswith('[]'):
        Name = Name[:-2]
      else:
        print('  *** Error1: %s' % (L2))
        os._exit(1)
    else:
      print('  *** Error2: %s' % (L2))
      os._exit(1)
  L3 = L1[1].strip().split('{')[1].strip()
  Data = L3.split('}')[0].strip().split(',')
  if Data[-1] == '':
    Data = Data[:-1]
  if Name in NameList:
    NameDups += 1
  else:
    if not Name in GlobalNameList:
      GlobalNameList.append(Name)
      GlobalNameBytes += len(Data)
    NameList.append(Name)
    NameBytes += len(Data)
    print('const byte %s [] PROGMEM = {' % (Name), file=HdrFile)
    while len(Data) > ChunkSize:
      print('  %s,' % (','.join(Data[:ChunkSize])), file=HdrFile)
      Data = Data[ChunkSize:]
    print('  %s' % (','.join(Data)), file=HdrFile)
    print('};', file=HdrFile)

#---------------------------------------------------------------------------------------
# Spin through an input file assembling word definitions and calling ProcessEntire to
# parse them.  We basically key on PROGMEM lines to find the start of word definitions,
# and on }; to find the ends.  Anything in between is buffered as needed.
#---------------------------------------------------------------------------------------

def ProcessFile(InoFileName, HdrFileName):
  global NameList, NameDups, NameBytes
  with open(InoFileName) as InoFile:
    with open(HdrFileName, 'w') as HdrFile:
      NameList = []
      NameDups = NameBytes = 0
      DataAccumulator = None
      for InoLine in InoFile:
        InoLine = InoLine.strip()
        if len(InoLine):
          if InoLine.startswith('//'):
            if 'PROGMEM' in InoLine:
              ProcessEntire(HdrFile, InoLine[2:].strip())
          elif 'PROGMEM' in InoLine:
            if InoLine.endswith('};'):
              ProcessEntire(HdrFile, InoLine)
            else:
              DataAccumulator = InoLine # first
          elif InoLine.startswith('0x'):
            if InoLine.endswith('};'):
              DataAccumulator = DataAccumulator + InoLine # final
              ProcessEntire(HdrFile, DataAccumulator)
            else:
              DataAccumulator = DataAccumulator + InoLine # middle

#---------------------------------------------------------------------------------------
# Process all files matching LoadFilePattern and load all the words in each one, then
# save each to a corresponding file based on SaveFilePattern.
#---------------------------------------------------------------------------------------

print()
SavePath = SaveFilePattern.split('/')[0]
if not os.path.exists(SavePath):
  os.makedirs(SavePath)
Summary = []
for InoFileName in glob.glob(LoadFilePattern):
  InoFileName = InoFileName.replace('\\','/')
  SaveName = os.path.basename(InoFileName).split('.')[0].lower()
  HdrFileName = SaveFilePattern % (SaveName)
  print('Processing %s...' % (InoFileName))
  ProcessFile(InoFileName, HdrFileName)
  Summary.append([InoFileName, HdrFileName, NameDups, len(NameList), NameBytes])
print()

#---------------------------------------------------------------------------------------
# Display a nice summary table showing the results.
#---------------------------------------------------------------------------------------

print('  %-29s  %-27s  %4s  %5s  %6s' % ('Input File','Output File','Dups','Words','Bytes'))
print('  %-29s  %-27s  %4s  %5s  %6s' % ('-'*29,'-'*27,'-'*4,'-'*5,'-'*6))
TotalDups = TotalWords = TotalBytes = 0
for Line in Summary:
  print('  %-29s  %-27s  %4d  %5d  %6d' % (Line[0],Line[1],Line[2],Line[3],Line[4]))
  TotalDups  += Line[2]
  TotalWords += Line[3]
  TotalBytes += Line[4]
print('  %-29s  %-27s  %4s  %5s  %6s' % (' ',' ','-'*4,'-'*5,'-'*6))
print('  %-29s  %-27s  %4d  %5d  %6d' % (' ',' ',TotalDups,TotalWords,TotalBytes))
print('Processed %d files, %d globally unique words (%d bytes).' % (len(Summary), len(GlobalNameList), GlobalNameBytes))
print()

#---------------------------------------------------------------------------------------
# End
#---------------------------------------------------------------------------------------
