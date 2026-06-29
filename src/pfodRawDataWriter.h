/*
   pfodRawDataWriter.cpp
   (c)2026 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/

#include <Print.h>

class pfodRawDataWriter : public Print {
 public:
    pfodRawDataWriter(Print** outputsPtr, size_t maxOutputs);
    // methods required for Print
	 virtual size_t write(uint8_t c);
   virtual size_t write(const uint8_t *buffer, size_t size);
 private:
   Print** outputsPtr;
   size_t maxOutputs;
};
