/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#ifndef PFOD_STREAM_STRING_H
#define PFOD_STREAM_STRING_H
// String pfodPrint class to capture JSON output
#include <string>
#include "pfodStream.h"

class pfodStreamString : public pfodStream {
  private:
    std::string buf;

  public:

    size_t escapeJSON(uint8_t c) {
      switch (c) {
        case '"':
          buf += '\\';
          buf += '"';
          break;
        case '\\':
          buf += '\\';
          buf += '\\';
          break;
        case '/':
          buf += '\\';
          buf += '/';
          break;
        case '\b':
          buf += '\\';
          buf += 'b';
          break;
        case '\f':
          buf += '\\';
          buf += 'f';
          break;
        case '\n':
          buf += '\\';
          buf += 'n';
          break;
        case '\r':
          buf += '\\';
          buf += 'r';
          break;
        case '\t':
          buf += '\\';
          buf += 't';
          break;
        default:
            // Handle remaining control characters (0x00-0x07, 0x0E-0x1F) with \u sequences
            if (c <= 0x1F) {
                buf += '\\';
                buf += 'u';
                buf += '0';
                buf += '0';
                // Convert to hex
                uint8_t high = (c >> 4) & 0x0F;
                uint8_t low = c & 0x0F;
                buf += (high < 10 ? '0' + high : 'A' + high - 10);
                buf += (low < 10 ? '0' + low : 'A' + low - 10);
            } else {
                buf += (char)c;
            }
          break;
      }
      return 1;
    }

  public:

    size_t write(const uint8_t *data, size_t size) {
      if (size && data) {
        size_t n = 0;
        while (size--) {
          n += write(*data++); // to do the replacements
        }
        return n;
      }
      return 0;
    }


    size_t write(uint8_t c) {
      buf += (char)c;
      return 1;
    }

    int available() {
      return (int)buf.length();
    }

    int read() {
      if (!buf.empty()) {
        char c = buf[0];
        buf.erase(0, 1);
        return c;
      }
      return -1;
    }

    int peek() {
      if (!buf.empty()) {
        return buf[0];
      }
      return -1;
    }

    void flush() {}
    void clear() {
      buf.clear();
    }

    const char* c_str() const { return buf.c_str(); }
    size_t length() const { return buf.length(); }
};

#endif
