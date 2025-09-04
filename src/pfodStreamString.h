#ifndef PFOD_STREAM_STRING_H
#define PFOD_STREAM_STRING_H
// String Print class to capture JSON output
class pfodStreamString :  public Stream, public String {
  protected:
    char u_buffer[6];
    int u_buffer_pos = 0;

    int is_hex_digit(char c) {
      return ((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'));
    }

    void process_char_normally(uint8_t c) {
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
          concat((char)c);
          break;
      }
    }

    size_t flush_u_buffer() {
      size_t rtn = u_buffer_pos;
      for (int i = 0; i < u_buffer_pos; i++) {
        process_char_normally(u_buffer[i]);
      }
      u_buffer_pos = 0;
      return rtn;
    }

    size_t writeJSON(uint8_t c) {
      if (u_buffer_pos == 0 && c == '\\') {
        // Start accumulating potential \u sequence
        u_buffer[u_buffer_pos++] = c;
        return 0;  // Haven't processed the character yet
      } else if (u_buffer_pos == 1 && c == 'u') {
        // Continue accumulating
        u_buffer[u_buffer_pos++] = c;
        return 0;  // Haven't processed the character yet
      } else if (u_buffer_pos >= 2 && u_buffer_pos <= 5 && is_hex_digit(c)) {
        // Continue accumulating hex digits
        u_buffer[u_buffer_pos++] = c;
        if (u_buffer_pos == 6) {
          // Complete valid \u sequence - output without escaping the backslash
          for (int i = 0; i < 6; i++) {
            concat(u_buffer[i]);
          }
          u_buffer_pos = 0;
          return 6;  // Processed 6 original characters (\u1234)
        }
        return 0;  // Haven't processed the character yet
      } else if (u_buffer_pos > 0) {
        // Invalid \u sequence - flush buffer and process current char
        for (int i = 0; i < u_buffer_pos; i++) {
          process_char_normally(u_buffer[i]);
        }
        int buffered_count = u_buffer_pos;
        u_buffer_pos = 0;
        process_char_normally(c);
        return buffered_count + 1;  // Return count of original chars processed
      } else {
        // Normal processing
        process_char_normally(c);
        return 1;  // Processed 1 original character
      }
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
          n += flush_u_buffer();
          char itemSeparator[] = "\",\r\n\"";
          if (concat(itemSeparator)) {
            n += strlen(itemSeparator);
          }
          if (concat((char)c)) {
            n += 1;
          }
        } else {
          n = writeJSON(c);
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
};

#endif
