/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#include "pfodLabel.h"

pfodLabel::pfodLabel()  {
}

void pfodLabel::init(pfodPrint *_out, struct pfodDwgVALUES* _values) {
  initValues(_values);
  valuesPtr = _values;
  out = _out;
  //valuesPtr->lastDwg = this;
}

pfodLabel &pfodLabel::offset(float _colOffset, float _rowOffset) {
  valuesPtr->colOffset = _colOffset;
  valuesPtr->rowOffset = _rowOffset;
  return *this;
}

pfodLabel &pfodLabel::fontSize(int _font) {
  valuesPtr->fontSize = _font;
  return *this;
}

pfodLabel &pfodLabel::bold() {
  valuesPtr->bold = 1;
  return *this;
}

pfodLabel &pfodLabel::center() {
  valuesPtr->align = 'C';
  return *this;
}

pfodLabel &pfodLabel::left() {
  valuesPtr->align = 'L';
  return *this;
}

pfodLabel &pfodLabel::right() {
  valuesPtr->align = 'R';
  return *this;
}

pfodLabel &pfodLabel::italic() {
  valuesPtr->italic = 1;
  return *this;
}

pfodLabel &pfodLabel::underline() {
  valuesPtr->underline = 1;
  return *this;
}

pfodLabel &pfodLabel::idx(uint16_t _idx) {
  valuesPtr->idx = _idx;
  return *this;

}

pfodLabel &pfodLabel::idx(pfodAutoIdx &a_idx) {
  idx(getAutoIdx(a_idx.idx));
  return *this;
}

pfodLabel &pfodLabel::text(const char *txt) {
  valuesPtr->text = txt;
  return *this;
}

pfodLabel &pfodLabel::units(const char *_units) {
  valuesPtr->units = _units;
  return *this;
}

pfodLabel &pfodLabel::floatReading(float _value) {
  return value(_value);
}

pfodLabel &pfodLabel::value(float _value) {
  valuesPtr->reading = _value;
  valuesPtr->haveReading = 1;
  valuesPtr->haveValue = 0;
  return *this;
}

pfodLabel &pfodLabel::intValue(int32_t _value) {
  valuesPtr->value = _value;
  valuesPtr->haveValue = 1;
  valuesPtr->haveReading = 0;
  return *this;
}

pfodLabel &pfodLabel::displayMax(float _displayMax) {
  valuesPtr->displayMax = _displayMax;
  return *this;
}

pfodLabel &pfodLabel::displayMin(float _displayMin) {
  valuesPtr->displayMin = _displayMin;
  return *this;
}

pfodLabel &pfodLabel::maxValue(int32_t _max) {
  valuesPtr->max = _max;
  return *this;
}

pfodLabel &pfodLabel::minValue(int32_t _min) {
  valuesPtr->min = _min;
  return *this;
}

pfodLabel &pfodLabel::decimals(int _decPlaces) {
  if (_decPlaces > 6) {
    _decPlaces = 6;
  }
  if (_decPlaces < -6) {
    _decPlaces = -6;
  }
  valuesPtr->decPlaces = _decPlaces;
  return *this;
}

pfodLabel &pfodLabel::color(int _color) {
  valuesPtr->color = _color;
  return *this;
}

 // replace restricted chars in text and units, the default
pfodLabel &pfodLabel::encode() {
	valuesPtr->encodeOutput = 1;
  return *this;
}

pfodLabel &pfodLabel::doNotEncode() {
	valuesPtr->encodeOutput = 0;
  return *this;
}


void pfodLabel::send(char _startChar) {
  // else
  out->print(_startChar);

  if (valuesPtr->haveValue != 0) {
    out->print('v');
  } else {
    out->print('t');
  }
  startText();
  if (valuesPtr->text != NULL) {
  	encodeText(out,valuesPtr->encodeOutput,valuesPtr->text);
  }
  if (valuesPtr->haveReading != 0) {
    float f = valuesPtr->reading;
    if (f < 0.0f) {
      out->print('-');
      f = -f;
    }
    printFloatDecimals(out,f,valuesPtr->decPlaces);
    if (valuesPtr->units != NULL) {
      encodeText(out,valuesPtr->encodeOutput,valuesPtr->units);
    }
  }
  sendColRowOffset();
  // cannot have both haveValue and haveReading
  if (valuesPtr->haveValue != 0) {
    sendValue(valuesPtr->value);
    out->print('~');
    if (valuesPtr->units != NULL) {
      encodeText(out,valuesPtr->encodeOutput,valuesPtr->units);
    }
    out->print('`');
    out->print(valuesPtr->max);
    out->print('`');
    out->print(valuesPtr->min);
    printFloat(valuesPtr->displayMax);
    printFloat(valuesPtr->displayMin);
    out->print('`');
    out->print(valuesPtr->decPlaces);
  }
  if (valuesPtr->align != 'C') {
    out->print('~');
    out->print(valuesPtr->align);
  }  	  
}

void pfodLabel::sendValue(int32_t val) {
  out->print('`');
  if (val == TOUCHED_COL) {
    out->print('c');
  } else if (val == TOUCHED_ROW) {
    out->print('r');
  } else {
    out->print(val);
  }
}



