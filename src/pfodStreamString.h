#ifndef PFOD_STREAM_STRING_H
#define PFOD_STREAM_STRING_H
// String Print class to capture JSON output
class pfodStreamString :  public Stream, public String {
  protected:

    size_t escapeJSON(uint8_t c) {
      // note should test that concat actually succeeds
      switch (c) {
        case '"':
          concat('\\');
          concat('"');
          break;
        case '\\':
          concat('\\');
          concat('\\');
          break;
        case '/':
          concat('\\');
          concat('/');
          break;
        case '\b':
          concat('\\');
          concat('b');
          break;
        case '\f':
          concat('\\');
          concat('f');
          break;
        case '\n':
          concat('\\');
          concat('n');
          break;
        case '\r':
          concat('\\');
          concat('r');
          break;
        case '\t':
          concat('\\');
          concat('t');
          break;
        default:
            // Handle remaining control characters (0x00-0x07, 0x0E-0x1F) with \u sequences
            if (c <= 0x1F) {
                concat('\\');
                concat('u');
                concat('0');
                concat('0');
                // Convert to hex
                uint8_t high = (c >> 4) & 0x0F;
                uint8_t low = c & 0x0F;
                concat(high < 10 ? '0' + high : 'A' + high - 10);
                concat(low < 10 ? '0' + low : 'A' + low - 10);
            } else {
                concat((char)c);
            }
          break;
      }
      return 1;
    }
  
  public:
    // set to false when writing incoming msg for the parser to parse
    // and when writing json header and footer
    // set to true when handling parser.prints dwgs and menus
    bool splitCmds = false; // controls splitting on | and }

    size_t write(const uint8_t *data, size_t size) {
      if (size && data) {
        const unsigned int newlen = length() + size;
        if (reserve(newlen + 1)) {
          size_t n = 0;
          while (size--) {
            n += write(*data++); // to do the replacements
          }
          return n;
        }
      }
      return 0;
    }


    size_t write(uint8_t c) {
      size_t n = 0;
      if (splitCmds) { // => outputing JSON
        if ((c == '|') || (c == '}')) {
          char itemSeparator[] = "\",\r\n\"";
          if (concat(itemSeparator)) {
            n += strlen(itemSeparator);
          }
          if (concat((char)c)) {
            n += 1;
          }
        } else {
          n = escapeJSON(c);
        }
      } else {  // if (splitCmds)
        if (concat((char)c)) {
          n += 1;
        }
      }
      return n;
    }

    int available() {
      return length();
    }

    int read() {
      if (length()) {
        char c = charAt(0);
        remove(0, 1);
        return c;
      }
      return -1;
    }

    int peek() {
      if (length()) {
        char c = charAt(0);
        return c;
      }
      return -1;
    }

    void flush() {}
    void clear() { 
      copy("",0);
    }
};

#endif
