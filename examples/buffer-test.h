#include "test.h"
#include "../lib/FileBuffer/FileBuffer.h"

class BufferTest : public Test {
    public:
    struct LogEntry {
        uint32_t ms;
        char line[80];
    };

    FileBuffer<LogEntry> *buff;

    public:
    BufferTest(const char *id, Stream *log);
    virtual ~BufferTest();
    void setup() override;
    void loop() override;
    
    void test1();
    void test2();
    void checkBuffer(const char* testName, int *a, FileBuffer<int> &fb);


};