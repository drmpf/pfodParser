#ifndef pfodParserUtils_h
#define pfodParserUtils_h
/**
pfodParserUtils for Arduino
*/
/*
 * (c)2014-2026 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This code may be freely used for both private and commercial use
 * Provided this copyright is maintained.
 */

#include <stddef.h>
#include <stdint.h>

// will always put '\0' at dest[maxLen]
size_t strncpy_safe(char* dest, const char* src, size_t maxLen);
uint32_t ipStrToNum(const char* ipStr);

#endif