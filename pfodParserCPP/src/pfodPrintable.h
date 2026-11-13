/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#ifndef pfodPrintable_h
#define	pfodPrintable_h
/*
 * Plain C++ replacement for the Arduino Printable class.
 * A pfodPrintable subclass implements printTo(pfodPrint&) so it can be passed
 * to pfodPrint::print()/println().
 * In the Arduino pfodParser library, pfodPrintable is a typedef for Printable,
 * so the same code compiles against both libraries.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

class pfodPrint; // forward declaration only, as in Arduino's Printable.h -- pfodPrint.h includes this file


class pfodPrintable {
  public:
    virtual size_t printTo(pfodPrint& p) const = 0;
    virtual ~pfodPrintable() {}
};

#endif //pfodPrintable_h
