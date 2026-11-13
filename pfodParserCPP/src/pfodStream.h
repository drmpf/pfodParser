/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#ifndef pfodStream_h
#define	pfodStream_h
/*
 * Plain C++ replacement for the Arduino pfodStream class.
 * Provides just the subset of that API actually used by pfodParser so
 * existing call sites (pfodPrint*, pfodStream*, print()/println() etc) are unchanged.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "pfodPrint.h"


class pfodStream : public pfodPrint {
  public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
};

#endif //pfodStream_h
