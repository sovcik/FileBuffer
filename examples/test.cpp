#include <Arduino.h>
#include <FileBuffer.h>

#ifndef NODEBUG_PRINT
#ifdef DEBUG_ESP_PORT
#define DEBUG_PRINT(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#define DEBUG_ARRAY(ARR,ARR_L) for (uint16_t _aidx =0; _aidx<ARR_L;_aidx++) {DEBUG_ESP_PORT.printf("%02X ",*(ARR+_aidx)); if (_aidx%20 == 19)DEBUG_ESP_PORT.printf("\n");}
#else
#define DEBUG_PRINT(...) os_printf( __VA_ARGS__ )
#define DEBUG_ARRAY(ARR,ARR_L) for (uint16_t _aidx =0; _aidx<ARR_L;_aidx++) {os_printf("%02X ",*(ARR+_aidx)); if (_aidx%20 == 19)os_printf("\n");}
#endif
#endif

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...)
#define DEBUG_ARRAY(...)
#define NODEBUG_PRINT
#endif

#define BUFFER_SIZE 5

void test(const char* testName, int a[BUFFER_SIZE], FileBuffer<int, BUFFER_SIZE> &fb){
    int x;

    DEBUG_PRINT("TEST:%s size=%d items=", testName, fb.size());

    for(int i=0; i<fb.capacity; i++){
        bool active = fb.getRaw(i, &x);

        // if active item, then it should be equal array item
        // if not active, then array item should be -1
        if ((active && x != a[i]) || (!active && a[i]!=-1))  {
            DEBUG_PRINT("X\nERROR at element #%d. buff=%u array=%u\n", i, x, a[i]);
            abort();
        } else {
            if (active)
                DEBUG_PRINT(" %u",a[i]);
            else
                DEBUG_PRINT(" -");
        }
    }
    DEBUG_PRINT("\n");

}

void runTests() {
    FileBuffer<int,5> fb1;
    DEBUG_PRINT("[main] size=%d, recSize=%d\n",fb1.capacity, fb1.recordSize);

    // open buffer file "buff1"
    fb1.open("buff1", true, true);

    DEBUG_PRINT("[main] empty=%d\n",fb1.isEmpty());
    
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
        test("test1",a, fb1);
    }

    int x;
    x = fb1.pop(); // x should be 2
    DEBUG_PRINT("[main] 1 pop=%d\n",x);
    if (x != 2) abort();

    x = fb1.pop();   // x should be 3
    DEBUG_PRINT("[main] 2 pop=%d\n",x);
    if (x != 3) abort();
    
    {
        // buffer should contain [6,-,-,4,5]
        int a[] = {6,-1,-1,4,5};
        test("test2",a, fb1);
    }

    fb1.push(7);
    {
        int a[] = {6,7,-1,4,5};
        test("test3",a, fb1);
    }

    fb1.pop();
    fb1.pop();
    {
        int a[] = {6,7,-1,-1,-1};
        test("test4",a, fb1);
    }

    fb1.pop();
    {
        int a[] = {-1,7,-1,-1,-1};
        test("test5",a, fb1);
    }

    fb1.pop();
    {
        int a[] = {-1,-1,-1,-1,-1};
        test("test-empty",a, fb1);
    }    
    DEBUG_PRINT("Buffer should be empty at this point. isEmpty=%d\n", fb1.isEmpty());
    if (!fb1.isEmpty()) abort();

    fb1.push(8);
    fb1.push(9);
    fb1.push(10);
    fb1.push(11);
    fb1.push(12);
    fb1.push(13);

    {
        int a[] = {13,9,10,11,12};
        test("test6",a, fb1);
    }

    fb1.pop();
    fb1.pop();

    {
        int a[] = {13,-1,-1,11,12};
        test("test7",a, fb1);
    }

    DEBUG_PRINT("Tests completed successfully.\n");    

}

void setup_test(){
    Serial.begin(115200);

    // wait few seconds
    for(int i=0;i<5;i++){
        DEBUG_PRINT("waiting %d\n",i);
        delay(1000);
    }

    SPIFFS.begin();
    runTests();

}

void loop_test(){

}