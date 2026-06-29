/*
   pfodRawDataWriter.cpp
   (c)2026 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/

#include "pfodRawDataWriter.h"
extern void writeHttpRawData(uint8_t c); // this method is defined as __week in ESP_PicoW_pfodAppServer.cpp

pfodRawDataWriter::pfodRawDataWriter(Print** _outputsPtr, size_t _maxOutputs) {
     outputsPtr = _outputsPtr;
      maxOutputs = _maxOutputs;
}  

// methods required for Print
size_t pfodRawDataWriter::write(uint8_t c) {
  if (outputsPtr) {
    for (size_t i=0; i<maxOutputs; i++) {
      if (outputsPtr[i]) {
        outputsPtr[i]->write(c);
      }
    }
  }
  writeHttpRawData(c); // this is a week method
  return 1;
}

size_t pfodRawDataWriter::write(const uint8_t *buffer, size_t size) {
  size_t n = 0;
  while (size--) {
    if (write(*buffer++)) n++;
    else break;
  }
  return n;
}


