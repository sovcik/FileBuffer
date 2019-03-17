#include <Arduino.h>
#include <FileBuffer.h>

void logBuffer() {

    struct LogEntry {
        uint32_t ms;
        char line[80];
    };

    FileBuffer<LogEntry,100> buff;

    buff.open("logbuffer.bin", false);  // do not reset with each open in order save uncommited entries

    LogEntry le;
    le.ms = millis();
    strcpy(le.line,"Log entry 1");
    buff.push(le);

    le.ms = millis();
    strcpy(le.line,"Log entry 2");
    buff.push(le);

    LogEntry le2;

    le2 = buff.pop();
    Serial.printf("[%ul] %s\n", le2.ms, le2.line);
    le2 = buff.pop();
    Serial.printf("[%ul] %s\n", le2.ms, le2.line);

    buff.close();

}

void setup_ex1() {
    Serial.begin(115200);

    // wait few seconds
    for(int i=0;i<5;i++){
        Serial.printf("waiting %d\n",i);
        delay(1000);
    }

    SPIFFS.begin();     
    logBuffer();
}

void loop_ex1() {

}
