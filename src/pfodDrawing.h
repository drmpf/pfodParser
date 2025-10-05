#ifndef PFOD_DRAWING_H
#define  PFOD_DRAWING_H
/*
   pfodDrawing.h
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

/**
  How to use:
  Create a pfodDrawing from the pfodParser ptr and a pfodDwgs ptr
  This will automatically add that pfodDrawing to a linked list of dwg in the parser
  Then when parser.parse( ) is called and finds a complete { .. } input
  it will automatically loop through the listOfDrawings and
  a) send that dwg if the cmd matches (the test is done in you implementation of sendDwg()
  b) or call your processDwgCmds() to handle a cmd from a touchZone for this dwg
  if the msg has been processed the return from parser.parse() is 0 and
  there is nothing else to be done, otherwise the first byte of the parser cmd is
  returned and can be actioned in the main loop.
  
        pfodDrawing *dwgPtr = pfodParser::listOfDrawings.getFirst();
      	while (dwgPtr) {
           dwgPtr->setParser(this); // make this parser the output stream
      	  if (dwgPtr->sendDwg()) {
      	    rtn = 0; // this msg has been replied to
      	    break;
          } else if (dwgPtr->processDwgCmds()) {
      	    rtn = 0; // this msg has been replied to
      	    break; // leave rtn unchanged so main code 
      	    //can detect touch on menu item and pick up changes if necessary
      	  }
      	  dwgPtr = pfodParser::listOfDrawings.getNext();
        }
      }
      return rtn;
**/

#include "pfodParser.h"
#include "pfodDwgs.h"

class pfodParser;
class pfodDwgs;

class pfodDrawing : public pfodAutoCmd {
  public:
    pfodDrawing();
    void init();
    void setParser(pfodParser* _parserPtr); // overrides the output stream
    virtual bool sendDwg(); // returns is dwg sent else false i.e. not this dwg's loadCmd
    virtual bool processDwgCmds(); // return true if handled else false
    pfodDrawing(pfodParser *parserPtr, pfodDwgs* dwgsPtr); // deprecated use init() and setParser()
    void setParserDwgs(pfodParser *_parserPtr , pfodDwgs* _dwgsPtr); // deprecated use init() and setParser()
 protected:
 	pfodParser *parserPtr;
    pfodDwgs *dwgsPtr;
 private:
    bool initializedDrawing;
 };
 
#endif