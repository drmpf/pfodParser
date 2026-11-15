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
  Create a pfodDrawing, then call init() on it -- init() is what adds that
  pfodDrawing to the linked list of dwgs in the parser.  The CONSTRUCTOR does not.
  !! A pfodDrawing you never call init() on is never registered, so it is
     never sent and never receives touches.
  A drawing has no pfodDwgs of its own: init() points dwgsPtr at the one
  pfodDwgs shared by all drawings (a static in pfodDrawing.cpp, linked only
  if a drawing is used), so dwgsPtr is NULL until init(). setParser() then
  directs its output to the parser being answered -- parse() calls setParser()
  on every registered drawing before dispatching each msg.
  Registering in init() rather than the constructor means a drawing object
  that is defined but never used (e.g. a generated default instance that a
  subclass replaces) costs no list node.
  Then when parser.parse( ) is called and finds a complete { .. } input
  it will automatically loop through the listOfDrawings and
  a) send that dwg if the cmd matches (the test is done in you implementation of sendDwg()
  b) or call your processDwgCmds() to handle a cmd from a touchZone for this dwg
  if the msg has been processed the return from parser.parse() is 0 and
  there is nothing else to be done, otherwise the first byte of the parser cmd is
  returned and can be actioned in the main loop.
  
        // first point EVERY registered dwg at this parser, so a handler that sends a
        // different dwg's update replies through the parser being answered
        for (pfodDrawing *p = pfodParser::listOfDrawings().getFirst(); p; p = pfodParser::listOfDrawings().getNext()) {
          p->setParser(this);
        }
        pfodDrawing *dwgPtr = pfodParser::listOfDrawings().getFirst();
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
      	  dwgPtr = pfodParser::listOfDrawings().getNext();
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
    // init(), REQUIRED, call once before use -- adds this drawing to pfodParser's
    // list of drawings.  Idempotent: guarded by initializedDrawing, so a second
    // call does not double-add.
    void init();
    void setParser(pfodParser* _parserPtr); // overrides the output stream
    virtual bool sendDwg(); // returns is dwg sent else false i.e. not this dwg's loadCmd
    virtual bool processDwgCmds(); // return true if handled else false
    pfodDrawing(pfodParser *parserPtr, pfodDwgs* dwgsPtr); // deprecated use init() and setParser()
    void setParserDwgs(pfodParser *_parserPtr , pfodDwgs* _dwgsPtr); // deprecated use init() and setParser()
 protected:
 	pfodParser *parserPtr;
    pfodDwgs *dwgsPtr;
    // Guards init().  Initialised HERE, not just in the constructor, so a
    // pfodDrawing on the stack or heap is as safe as a global -- a global
    // would be zeroed by .bss, but an automatic one would start as garbage
    // and could make init() early-return, leaving the drawing unregistered.
    bool initializedDrawing = false;
 };
 
#endif