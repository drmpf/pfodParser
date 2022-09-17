
#include <Arduino.h> // for String
#include "pfodAutoCmd.h"

size_t pfodAutoCmd::_cmdInt = 1;
pfodAutoCmd::pfodAutoCmd() {
  String cmdString('_'); // released at the end of this method
  cmdString += _cmdInt++;
  strncpy((char*)cmd,cmdString.c_str(),sizeof(cmd));
  ((char*)cmd)[(sizeof(cmd)-1)] = '\0'; // always terminate
}
