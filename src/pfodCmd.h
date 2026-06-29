#ifndef PFOD_CMD_H
#define PFOD_CMD_H

#include <Printable.h>

class pfodCmd : public Printable {
  public: 
  	  pfodCmd();
  	  char cmd[5] = "";	// allow for cmds from m1 to m999
      size_t printTo(Print& p) const;
  private:
  	  static size_t _cmdInt;
};
#endif