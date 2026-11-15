/*
   pfodDrawing.cpp
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <pfodDrawing.h>

/**
 * Constructor, only initialises the pointers. Does NOT register this drawing
 * with the parser, init() does that, so a drawing that is defined but never
 * init()'d (e.g. a generated default instance that a subclass replaces) costs
 * no list node.
 * A drawing has no pfodDwgs of its own: init() points it at the one pfodDwgs
 * shared by all drawings (sharedDwgs()).
 */
pfodDrawing::pfodDrawing() {
  parserPtr = NULL;
  dwgsPtr = NULL;
}

/**
 * The one pfodDwgs drawing builder shared by every pfodDrawing.
 * A function-local static, referenced only from pfodDrawing::init(): a sketch
 * with no drawings never calls init(), so the linker drops this pfodDwgs and
 * the vtables of its item builders entirely. A sketch with drawings gets one
 * pfodDwgs in static RAM (shown in the compiler's RAM report), never on the heap.
 * Sharing is safe because pfodParser is not thread safe and must be driven from
 * one task: sends never overlap, and the builder holds only per-send state.
 * @return the shared pfodDwgs
 */
static pfodDwgs& sharedDwgs() {
  static pfodDwgs dwgs;
  return dwgs;
}

/**
 * REQUIRED, call once before use. Adds this drawing to pfodParser's list of
 * drawings, which costs one small linked list node on the heap, and points it
 * at the shared pfodDwgs. Does nothing if already called.
 */
void pfodDrawing::init() {
  if (initializedDrawing) {
    return;
  }
  initializedDrawing = true;
  dwgsPtr = &sharedDwgs();
  pfodParser::addDwg(this); // allocates pointer for linked list
}

// deprecated use init() and setParser()
pfodDrawing::pfodDrawing(pfodParser *_parserPtr, pfodDwgs* _dwgsPtr) {
   setParserDwgs(_parserPtr,_dwgsPtr);
}

/**
 * Points this drawing at a parser: its output, built with the shared pfodDwgs,
 * goes to that parser. Called by pfodParser::parse() for every registered
 * drawing before each msg is dispatched, and by generated init() code for
 * priming. init() must have been called first (it sets dwgsPtr).
 * @param _parserPtr the parser to send through
 */
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
