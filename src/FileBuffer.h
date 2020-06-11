/*
MIT License

Copyright (c) 2019 Jozef Sovcik

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#ifndef __FILEBUFFER_H__
#define __FILEBUFFER_H__

#include <Arduino.h>
#include <FS.h>

#define FILEBUFFER_IDX_TYPE  uint16_t
#define FILEBUFFER_IDX_SIZE  2

template <typename T> 
class FileBuffer {

    public:

    FileBuffer(uint16_t capacity=50);

	/**
	 * Disables copy constructor
	 */
	FileBuffer(const FileBuffer&) = delete;
	FileBuffer(FileBuffer&&) = delete;

	/**
	 * Disables assignment operator
	 */
	FileBuffer& operator=(const FileBuffer&) = delete;
	FileBuffer& operator=(FileBuffer&&) = delete;    
    
    /**
     * Open buffer specifying file name
     */
    bool open(const char* fileName, bool reset = false, bool circular = true);

    void close();

    /**
     * Add record to the buffer
     * If buffer is full and circular buffer is enabled, then added record overwrites
     * the oldest record stored in the buffer.
     */
    bool push(T record);

    /**
     * Retrieve oldest record from the buffer and remove it
     */
    const T pop();

    /**
     * Retrieve specified record from the buffer without removing it
     */
    const T peek(size_t idx);

    /**
     * Retrieve raw record from the buffer storage. 
     * Provided object will be filled with data.
     * False is returned if raw record does not contain any data.
     */
    bool getRaw(size_t idx, T* rec);

    /**
     * Is buffer ready?
     */
    inline bool isReady(){return _open;};

    /**
     * Returns true if buffer is empty
     */
    inline bool isEmpty(){return (_count == 0);};

    /**
     * Returns true if buffer reached its capacity.
     */
    inline bool isFull(){return (_count == _capacity);};

    /**
     * How many more records can be stored in the buffer
     */
    inline uint16_t available(){ return _capacity-_count;};

    /**
     * Count of records available in the buffer
     */
    inline uint16_t size(){return _count;};

    inline uint16_t capacity(){return _capacity;};
    
    /**
     * Removes all records from the buffer
     */
    void clear();

    #ifdef DEBUG_FILEBUFFER
    void showBuff();
    #endif

    private:
    /** 
     * Capacity how many records can be stored in the buffer
     */
    uint16_t _capacity;

    /**
     * Size of the buffer record in bytes
     */
    uint16_t recordSize = static_cast<uint16_t>(sizeof(T));

    /**
     * Buffer file size in bytes
     */
    uint32_t maxFileSize;


    private:
    uint16_t _count;    // count of active records
    uint32_t _head;     // position of the newest record
    uint32_t _tail;     // position of the oldest record
    uint32_t _idx;      // current index (newer records have index hgher than older ones)
    bool _circular;     // is buffer circular (head starts overwriting tail if capacity is reached)
    bool _open;         // was 'open' method called?
    File _file;

    size_t setHeadTail();

};


#include "FileBuffer.hpp"

#endif
