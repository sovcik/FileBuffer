#ifndef __TESTS_H__
#define __TESTS_H__

#ifndef NODEBUG_PRINT
#ifdef DEBUG_ESP_PORT
#define DEBUG_PRINT(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#define DEBUG_ARRAY(ARR,ARR_L) for (uint16_t _aidx =0; _aidx<ARR_L;_aidx++) {DEBUG_ESP_PORT.printf("%02X ",*(ARR+_aidx)); if (_aidx%20 == 19)DEBUG_ESP_PORT.printf("\n");}
#endif
#endif

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...)
#define DEBUG_ARRAY(...)
#define NODEBUG_PRINT
#endif

#include <Stream.h>
#include <Arduino.h>

class Test {
    private:
    unsigned long _alive;
    const char* id;

    public:
    Stream *log;

    public:
    Test(const char* id, Stream *log){
        this->id = strdup(id);
        this->log=log;
        _alive = millis();
    };

    ~Test(){
        free((void*)id);
    }
    /**
     * Test setup method
     */
    virtual void setup()=0;

    /**
     * Test loop - will be executed in the main loop
     */
    virtual void loop()=0;

    void alive(){
        if (millis()-_alive > 5000){
            _alive = millis();
            log->printf("[%s] mil=%lu Alive. Free heap=%lu\n", id, millis(), ESP.getFreeHeap());
        }
    }

};

#endif