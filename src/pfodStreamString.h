#ifndef PFOD_STREAM_STRING_H
#define PFOD_STREAM_STRING_H
// String Print class to capture JSON output
class pfodStreamString :  public Stream, public String {
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
      if (splitCmds) {
        if ((c == '|') || (c == '}')) {
          char itemSeparator[] = "\",\r\n\"";
          if (concat(itemSeparator)) {
            n += strlen(itemSeparator);
          }
        } else if (c == '\n') { // should do other json filtering here also
          if (concat((char)'\\')) {
            n += 1;
          }
          c = 'n';
        }
      }  // if (splitCmds)
      if (concat((char)c)) {
        n += 1;
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
};

#endif
