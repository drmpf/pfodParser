/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#ifndef PFOD_AUTO_CMD_H
#define PFOD_AUTO_CMD_H

#include "pfodStream.h"

class pfodAutoCmd : public pfodPrintable {
  public: 
  	  pfodAutoCmd();
  	  char cmd[5] = "";	// allow for cmds from c1 to c999
      size_t printTo(pfodPrint& p) const;
  private:
  	  static size_t _cmdInt;
};
#endif