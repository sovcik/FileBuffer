# FileBuffer
File system based circular FIFO buffer library if you e.g.
* need persistent buffer (surviving device restarts, e.g. for logging purposes)
* larger buffer

## Usage

### Declaration
Buffer for 50 integers.
```
#include <FileBuffer.h>
FileBuffer<int,50> buff;
```
Buffer for 100 more complex structures
```
#include <FileBuffer.h>
struct LogEntry {
  uint32_t datetime;
  char line[100];
};
FileBuffer<LogEntry,100> buff;
```

### Opening Buffer
Use file "buffer.bin" for buffering. 
```
buff.open("buffer.bin");                // keep existing content + allow owerwriting of older entries
buff.open("buffer.bin", false);         // clean file + allow owerwriting of older entries
buff.open("buffer.bin", false, false);  // clean file + do not owerwrite older entries
```

### Storing and Retrieving Data
```
myType a,b;
bool x;

buff.push(a);        // store data to buffer (if circular, then newer items will 
                     // overwrite older ones after reaching buffer capacity
                 
b = buff.pop();      // get oldest item;

b = buff.peek(3);    // get 3rd oldest item. Exception will occur if less than 3 items are in the buffer.

x = buff.getRaw(3, &b);  // load 3rd item from buffer storage to variable b
                         // will return false if 3rd item was inactive (no data)
                         
buff.clear();        // remove all items                         
```

### Status
```
buff.circular         // is buffer circular? newer items overwrite older if capacity is reached
buff.capacity         // how many items can fit into buffer
buff.size();          // number of items in buffer
buff.isEmpty();       // empty?
buff.isFull();        // size() == capacity?
```

### Simple Example
```
// define your own structure
struct myStruct {...};

// create buffer for 100 items
FileBuffer<myStruct,100> buff;

// make sure to initialize your file system before opening buffer file
SPIFFS.begin();

// open buffer file
buff.open("myBuffFile.bin");

// create your data object
myStruct a;

// push it to buffer
buff.push(a);

myStruct b;
// and retrieve it from buffer
b = buff.pop();
```
