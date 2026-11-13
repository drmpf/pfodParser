// pfodDebugPtr.cpp

// call setDebugPtr( ) to set output to send debug to
// NOTE: Still need to #define DEBUG in each file you want debug output from

/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */

#include "pfodDebugPtr.h"

static pfodPrint* debugPtr = NULL;
pfodPrint* getDebugPtr() {
    return debugPtr;
}

void setDebugPtr(pfodPrint *outPtr) {
    debugPtr = outPtr;
}
    
