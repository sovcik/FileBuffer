#include "buffer-test.h"

BufferTest::BufferTest(const char *id, Stream* log) : Test(id, log){
    buff = new FileBuffer<LogEntry>(200);
}

BufferTest::~BufferTest(){
    delete buff;
}

void BufferTest::setup(){
    test1();
    test2();

}

void BufferTest::checkBuffer(const char* testName, int *a, FileBuffer<int> &fb){
    int x;

    log->printf("TEST:%s size=%d items=", testName, fb.size());

    for(int i=0; i<fb.capacity(); i++){
        bool active = fb.getRaw(i, &x);

        // if active item, then it should be equal array item
        // if not active, then array item should be -1
        if ((active && x != *(a+i)) || (!active && *(a+i)!=-1))  {
            log->printf("X\nERROR at element #%d. buff=%u array=%u\n", i, x, *(a+i));
            abort();
        } else {
            if (active)
                log->printf(" %u",*(a+i));
            else
                log->printf(" -");
        }
    }
    log->printf("\n");

}

void BufferTest::test1(){
    buff->open("logbuffer.bin", true);  // do not reset with each open in order save uncommited entries

    LogEntry le;
    le.ms = millis();
    strcpy(le.line,"Log entry 1");
    buff->push(le);

    le.ms = millis();
    strcpy(le.line,"Log entry 2");
    buff->push(le);

    LogEntry le2;

    le2 = buff->pop();
    log->printf("[%ul] %s\n", le2.ms, le2.line);
    le2 = buff->pop();
    log->printf("[%ul] %s\n", le2.ms, le2.line);

    log->printf("going to overfill heap=%lu\n", ESP.getFreeHeap());
    for (int i = 0;i<buff->capacity()+10;i++){
        le.ms = millis();
        strcpy(le.line,String("OVR Log entry "+String(i)).c_str());
        buff->push(le);
    }
    log->printf("going to check contents heap=%lu\n", ESP.getFreeHeap());
    while(!buff->isEmpty()){
        le2 = buff->pop();
        log->printf("[%ul] %s remaining=%d\n", le2.ms, le2.line, buff->size());
    }

    log->printf("completed heap=%lu\n", ESP.getFreeHeap());

    buff->close();

}

void BufferTest::test2(){
    FileBuffer<int> fb1(5);
    log->printf("[test2] size=%d\n",fb1.capacity());

    // open buffer file "buff1"
    fb1.open("buff1", true, true);

    log->printf("[test2] empty=%d\n",fb1.isEmpty());
    
    // push 6 items
    fb1.push(1);
    fb1.push(2);
    fb1.push(3);
    fb1.push(4);
    fb1.push(5);
    fb1.push(6);

    {
        // buffer should contain [6,2,3,4,5]
        int a[] = {6,2,3,4,5};
        checkBuffer("test1", a, fb1);
    }

    int x;
    x = fb1.pop(); // x should be 2
    log->printf("[main] 1 pop=%d\n",x);
    if (x != 2) abort();

    x = fb1.pop();   // x should be 3
    log->printf("[main] 2 pop=%d\n",x);
    if (x != 3) abort();
    
    {
        // buffer should contain [6,-,-,4,5]
        int a[] = {6,-1,-1,4,5};
        checkBuffer("test2", a, fb1);
    }

    fb1.push(7);
    {
        int a[] = {6,7,-1,4,5};
        checkBuffer("test3",a, fb1);
    }

    fb1.pop();
    fb1.pop();
    {
        int a[] = {6,7,-1,-1,-1};
        checkBuffer("test4",a, fb1);
    }

    fb1.pop();
    {
        int a[] = {-1,7,-1,-1,-1};
        checkBuffer("test5",a, fb1);
    }

    fb1.pop();
    {
        int a[] = {-1,-1,-1,-1,-1};
        checkBuffer("test-empty",a, fb1);
    }    
    log->printf("Buffer should be empty at this point. isEmpty=%d\n", fb1.isEmpty());
    if (!fb1.isEmpty()) abort();

    fb1.push(8);
    fb1.push(9);
    fb1.push(10);
    fb1.push(11);
    fb1.push(12);
    fb1.push(13);

    {
        int a[] = {13,9,10,11,12};
        checkBuffer("test6",a, fb1);
    }

    fb1.pop();
    fb1.pop();

    {
        int a[] = {13,-1,-1,11,12};
        checkBuffer("test7",a, fb1);
    }

    log->printf("Tests completed successfully.\n");    

};

void BufferTest::loop(){
    alive();
}