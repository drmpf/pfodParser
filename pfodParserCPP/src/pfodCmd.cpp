/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */

#include "pfodCmd.h"

size_t pfodCmd::_cmdInt = 1;

/**
 * Constructor.  Builds this object's auto-assigned cmd, "m1", "m2", ...
 * The digits are formatted directly into the cmd[] buffer (no Arduino String,
 * no heap) because pfodCmd objects are normally global and construct during
 * static initialization, before setup() runs.
 * NOTE: the cmd[5] buffer allows for cmds m1 to m999.  Sketches must not
 * create more than 999 pfodCmd objects; beyond that only the lowest 3 digits
 * are kept, so cmds collide.
 */
pfodCmd::pfodCmd() {
  // cannot use _ as the cmd prefix as this interferes with the security handshake
  size_t n = _cmdInt++;
  char digits[3]; // up to 3 digits, cmds m1 to m999
  size_t i = 0;
  do {
    digits[i++] = '0' + (n % 10);
    n /= 10;
  } while ((n != 0) && (i < sizeof(digits)));
  size_t p = 0;
  cmd[p++] = 'm';
  while (i != 0) {
    cmd[p++] = digits[--i]; // digits were collected least-significant first
  }
  cmd[p] = '\0';
}

size_t pfodCmd::printTo(pfodPrint& p) const {
  return p.print(cmd);
}
