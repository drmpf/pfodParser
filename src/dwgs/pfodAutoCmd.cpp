
#include <Arduino.h> // for Print used by printTo()
#include "pfodAutoCmd.h"

size_t pfodAutoCmd::_cmdInt = 1;

/**
 * Constructor.  Builds this object's auto-assigned cmd, "c1", "c2", ...
 * The digits are formatted directly into the cmd[] buffer (no Arduino String,
 * no heap) because pfodAutoCmd objects are normally global and construct
 * during static initialization, before setup() runs.
 * NOTE: the cmd[5] buffer allows for cmds c1 to c999.  Sketches must not
 * create more than 999 pfodAutoCmd objects (pfodDrawings included, each is
 * one); beyond that only the lowest 3 digits are kept, so cmds collide.
 */
pfodAutoCmd::pfodAutoCmd() {
  // cannot use _ as the cmd prefix as this interferes with the security handshake
  size_t n = _cmdInt++;
  char digits[3]; // up to 3 digits, cmds c1 to c999
  size_t i = 0;
  do {
    digits[i++] = '0' + (n % 10);
    n /= 10;
  } while ((n != 0) && (i < sizeof(digits)));
  size_t p = 0;
  cmd[p++] = 'c';
  while (i != 0) {
    cmd[p++] = digits[--i]; // digits were collected least-significant first
  }
  cmd[p] = '\0';
}

size_t pfodAutoCmd::printTo(Print& p) const {
  return p.print(cmd);
}
