/*
   pfodDrawing.cpp
   (c)2014-2026 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provided this copyright is maintained.
*/

#include <pfodDrawing.h>

/**
 * Constructor, only initialises the pointers. Does NOT allocate the pfodDwgs or
 * register this drawing with the parser, init() does that, so a drawing that is
 * defined but never init()'d (e.g. a generated default instance that a subclass
 * replaces) costs no list node and no pfodDwgs.
 */
pfodDrawing::pfodDrawing() {
  parserPtr = NULL;
  dwgsPtr = NULL;
}

/**
 * REQUIRED, call once before use. Allocates this drawing's pfodDwgs and adds it
 * to pfodParser's list of drawings. Does nothing if already called.
 * NOTE: each pfodDrawing allocates its own pfodDwgs (and a linked list node)
 * on the heap.  This heap usage is NOT included in the compiler's static RAM
 * report, so programs with many drawings on small micros need to allow extra
 * free RAM for it.
 */
void pfodDrawing::init() {
  if (initializedDrawing) {
    return;
  }
  initializedDrawing = true;
  dwgsPtr = new pfodDwgs;
  pfodParser::addDwg(this); // allocates pointer for linked list
}

// deprecated use init() and setParser()
pfodDrawing::pfodDrawing(pfodParser *_parserPtr, pfodDwgs* _dwgsPtr) {
   setParserDwgs(_parserPtr,_dwgsPtr);
}

// overrides the output stream
void pfodDrawing::setParser(pfodParser *_parserPtr) {
  parserPtr = _parserPtr;
  dwgsPtr->setOut(_parserPtr); 
}        

// deprecated use init() and setParser()
void pfodDrawing::setParserDwgs(pfodParser *_parserPtr , pfodDwgs* _dwgsPtr) {
  parserPtr = _parserPtr;
  dwgsPtr = _dwgsPtr;
  parserPtr->addDwg(this);
}

bool pfodDrawing::sendDwg() { return false; } // returns is dwg sent else false i.e. not this dwg's loadCmd
bool pfodDrawing::processDwgCmds() {return false;} // return true if handled else false
