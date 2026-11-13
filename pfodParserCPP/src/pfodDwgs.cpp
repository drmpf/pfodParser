/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include <cmath>
#include "pfodDwgs.h"

#include "dwgs/pfodDwgsBase.h"

// Arduino's DEG_TO_RAD constant, as a float so the maths stays single precision
// (hardware FPU on Cortex-M4F / M33, no double soft-float code)
static const float DEG_TO_RAD = 0.017453292519943295769236907684886f;

pfodDwgs::pfodDwgs() {
   pfodDwgsBase::initValues(&values);
}

// the x position at the end of this radius at this angle (in degrees)
float pfodDwgs::xRadius(float angle, float radius) {
	return radius*std::cos(angle*DEG_TO_RAD); // float overload of cos
}

// the y position at the end of this radius at this angle (in degrees)
// note y goes down for +ve values so need to return -ve here
float pfodDwgs::yRadius(float angle, float radius) {
  return -radius*std::sin(angle*DEG_TO_RAD); // float overload of sin
}

pfodCircle& pfodDwgs::circle() {
  c.init(out, &values); // set output
  return c;
}

pfodLabel& pfodDwgs::label() {
  t.init(out, &values); // set output
  return t;
}

pfodRectangle& pfodDwgs::rectangle() {
  r.init(out, &values); // set output
  return r;
}

pfodTouchZone& pfodDwgs::touchZone() {
  x.init(out, &values); // set output
  return x;
}

pfodTouchAction& pfodDwgs::touchAction() {
  X.init(out, &values); // set output
  return X;
}

pfodTouchActionInput& pfodDwgs::touchActionInput() {
  XI.init(out, &values); // set output
  return XI;
}

pfodLine& pfodDwgs::line() {
  l.init(out, &values); // set output
  return l;
}

pfodArc& pfodDwgs::arc() {
  a.init(out, &values); // set output
  return a;
}

pfodErase& pfodDwgs::erase() {
  e.init(out, &values); // set output
  return e;
}

pfodHide& pfodDwgs::hide() {
  h.init(out, &values); // set output
  return h;
}
pfodUnhide& pfodDwgs::unhide() {
  u.init(out, &values); // set output
  return u;
}
pfodIndex& pfodDwgs::index() {
  i.init(out, &values); // set output
  return i;
}

pfodInsertDwg& pfodDwgs::insertDwg() {
  d.init(out, &values); // set output
  return d;
}

pfodDwgs::pfodDwgs(pfodPrint *_outPtr) {
  out = _outPtr;
}

void pfodDwgs::start(int cols, int rows, int backgroundColor,  uint8_t moreData) {
  out->print('{'); out->print('+'); out->print(backgroundColor); out->print('`');  out->print(cols); out->print('`'); out->print(rows);
  out->print('~'); // need this for parsing even if no 'm'
  if (moreData != 0) {
    out->print('m');
  }
}

void pfodDwgs::startUpdate(uint8_t moreData) {
    out->print('{'); out->print('+');
    if (moreData != 0) {
      out->print('~');
      out->print('m');
    }
}

void pfodDwgs::end() {
    out->print('}');
}


void pfodDwgs::pushZero(float col, float row, float scale) {
  out->print('|'); out->print('z');
  printFloat((float)col); printFloat((float)row);
  if (scale != 1) {
    printFloat((float)scale);
  }
}

void pfodDwgs::popZero() {
  out->print('|'); out->print('z');
}

int pfodDwgs::reserveIdx(int numToReserve) {
  int rtn = _idx;
  _idx = _idx + numToReserve;
  return rtn;
}

int pfodDwgs::_idx = 1;



