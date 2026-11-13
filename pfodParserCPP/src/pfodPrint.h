/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */
#ifndef pfodPrint_h
#define	pfodPrint_h
/*
 * Plain C++ replacement for the Arduino pfodPrint class.
 * Provides just the subset of that API actually used by pfodParser so
 * existing call sites (pfodPrint*, pfodStream*, print()/println() etc) are unchanged.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "pfodPrintable.h"

typedef uint8_t byte;
typedef bool boolean;


class pfodPrint {
  public:
    virtual ~pfodPrint() {}

    // core write methods, subclasses must implement write(uint8_t)
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) {
      size_t n = 0;
      while (size--) {
        n += write(*buffer++);
      }
      return n;
    }
    size_t write(const char *str) {
      if (!str) {
        return 0;
      }
      return write((const uint8_t*)str, strlen(str));
    }
    virtual void flush() {}

    size_t print(char c) { return write((uint8_t)c); }
    size_t print(const char *str) { return write(str); }
    size_t print(int n) { return print((long)n); }
    size_t print(unsigned int n) { return print((unsigned long)n); }
    size_t print(long n) {
      if (n < 0) {
        size_t written = write((uint8_t)'-');
        return written + print((unsigned long)(-n));
      }
      return print((unsigned long)n);
    }
    size_t print(unsigned long n) {
      char buf[8 * sizeof(unsigned long) + 1];
      char *ptr = buf + sizeof(buf) - 1;
      *ptr = '\0';
      do {
        *--ptr = '0' + (n % 10);
        n /= 10;
      } while (n != 0);
      return write((const uint8_t*)ptr, strlen(ptr));
    }
    /**
     * Print a signed 64 bit (long long) value in decimal.
     * Negative values print a '-' then the magnitude. The magnitude is computed
     * in unsigned arithmetic (0 - n), so LLONG_MIN prints correctly.
     * @param n the value to print
     * @return number of bytes written
     */
    size_t print(long long n) {
      if (n < 0) {
        size_t written = write((uint8_t)'-');
        return written + print(0ULL - (unsigned long long)n);
      }
      return print((unsigned long long)n);
    }
    /**
     * Print an unsigned 64 bit (unsigned long long) value in decimal.
     * Separate from print(unsigned long) so 32 bit micros only use 64 bit
     * division when a long long is actually printed.
     * @param n the value to print
     * @return number of bytes written
     */
    size_t print(unsigned long long n) {
      char buf[8 * sizeof(unsigned long long) + 1];
      char *ptr = buf + sizeof(buf) - 1;
      *ptr = '\0';
      do {
        *--ptr = '0' + (n % 10);
        n /= 10;
      } while (n != 0);
      return write((const uint8_t*)ptr, strlen(ptr));
    }
    size_t print(double n, int digits = 2) {
      size_t written = 0;
      if (n < 0) {
        written += write((uint8_t)'-');
        n = -n;
      }
      double rounding = 0.5;
      for (int i = 0; i < digits; i++) {
        rounding /= 10.0;
      }
      n += rounding;
      unsigned long intPart = (unsigned long)n;
      double remainder = n - (double)intPart;
      written += print(intPart);
      if (digits > 0) {
        written += write((uint8_t)'.');
        while (digits-- > 0) {
          remainder *= 10.0;
          int digit = (int)remainder;
          written += write((uint8_t)('0' + digit));
          remainder -= digit;
        }
      }
      return written;
    }
    size_t print(const pfodPrintable& x) { return x.printTo(*this); }

    size_t println() {
      size_t written = write((uint8_t)'\r');
      written += write((uint8_t)'\n');
      return written;
    }
    size_t println(char c) { size_t n = print(c); return n + println(); }
    size_t println(const char *str) { size_t n = print(str); return n + println(); }
    size_t println(int n) { size_t written = print(n); return written + println(); }
    size_t println(unsigned int n) { size_t written = print(n); return written + println(); }
    size_t println(long n) { size_t written = print(n); return written + println(); }
    size_t println(unsigned long n) { size_t written = print(n); return written + println(); }
    size_t println(long long n) { size_t written = print(n); return written + println(); }
    size_t println(unsigned long long n) { size_t written = print(n); return written + println(); }
    size_t println(double n, int digits = 2) { size_t written = print(n, digits); return written + println(); }
    size_t println(const pfodPrintable& x) { size_t written = print(x); return written + println(); }
};


#endif //pfodPrint_h
