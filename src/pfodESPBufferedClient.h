#ifndef pfodESPBufferedClient_h
#define pfodESPBufferedClient_h
/**
 This pfodESPBufferedClient class works for boards/libraries that have a WiFiClient class
 These include ESP32 ESP2866 and Nano 33 IOT with the WiFiNINA library
*/
 
/**
 (c)2015-2022 Forward Computing and Control Pty. Ltd.
 This code may be freely used for both private and commerical use.
 Provide this copyright is maintained.
*/
#if defined (ESP32) || defined (ESP8266) || defined (ARDUINO_SAMD_NANO_33_IOT) 

#include "Stream.h"
#include "WiFiClient.h"
class pfodESPBufferedClient : public Stream {

  public:
    pfodESPBufferedClient(); // default 1460 byte buffer and default send delay 10mS 
    pfodESPBufferedClient* connect(WiFiClient* _client);  
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);
    virtual int available();
    virtual int availableForWrite();
    virtual int read();
    virtual int peek();
    virtual void flush();   
    virtual void stop();
    virtual uint8_t connected();
    void setDebugStream(Print* out);
  private:
    WiFiClient* client;
    void sendAfterDelay();
    void forceSend();
    size_t _write(uint8_t c);
    static const unsigned long DEFAULT_SEND_DELAY_TIME = 10; // 10mS delay before sending buffer
    //#define WIFICLIENT_MAX_PACKET_SIZE 1460
    static const size_t DEFAULT_SEND_BUFFER_SIZE = 1460; //WIFICLIENT_MAX_PACKET_SIZE; // Max data size for standard TCP/IP packet
   // at 57600 baud can read about 1152 bytes in 0.2sec so limit serial to <=57600 to prevent losing data
    uint8_t sendBuffer[DEFAULT_SEND_BUFFER_SIZE]; //
    size_t sendBufferIdx = 0;
    unsigned long sendTimerStart = 0;
    unsigned long sendDelayTime;
    Print* debugOut;
};

#endif

#endif // pfodESPBufferedClient_h