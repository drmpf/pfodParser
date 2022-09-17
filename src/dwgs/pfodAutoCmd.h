#ifndef PFOD_AUTO_CMD_H
#define PFOD_AUTO_CMD_H

class pfodAutoCmd {
  public: 
  	  pfodAutoCmd();
  	  const char cmd[5] = "";	// allow for cmds from _1 to _999  
  private:
  	  static size_t _cmdInt;
};
#endif