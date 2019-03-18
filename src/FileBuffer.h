/*
 FileBuffer.h - Circular filebuffer library for Arduino.
 Copyright (c) 2019 Jozef Sovcik.
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as 
 published by the Free Software Foundation, either version 3 of the 
 License, or (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef __FILEBUFFER_H__
#define __FILEBUFFER_H__

#include <Arduino.h>
#include <FS.h>

//#define DEBUG_FILEBUFFER
#ifdef DEBUG_FILEBUFFER
#ifdef DEBUG_ESP_PORT
#define DEBUG_FB_PRINT(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG_FB_PRINT(...) os_printf( __VA_ARGS__ )
#endif
#else
#define DEBUG_FB_PRINT(...)
#endif

#define FILEBUFFER_IDX_TYPE  uint16_t
#define FILEBUFFER_IDX_SIZE  2

template <typename T, size_t S> 
class FileBuffer {

    public:

    /** 
     * Capacity how many records can be stored in the buffer
     */
    static constexpr uint16_t capacity = static_cast<uint16_t>(S);

    /**
     * Size of the buffer record in bytes
     */
    static constexpr uint16_t recordSize = static_cast<uint16_t>(sizeof(T));

    /**
     * Buffer file size in bytes
     */
    static constexpr uint32_t maxFileSize = static_cast<uint32_t>((sizeof(T)+FILEBUFFER_IDX_SIZE)*S);

    constexpr FileBuffer();

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
    inline bool isFull(){return (_count == capacity);};

    /**
     * How many more records can be stored in the buffer
     */
    inline uint16_t available(){ return capacity-_count;};

    /**
     * Count of records available in the buffer
     */
    inline uint16_t size(){return _count;};
    
    /**
     * Removes all records from the buffer
     */
    void clear();

    #ifdef DEBUG_FILEBUFFER
    void showBuff();
    #endif


    private:
    uint16_t _count;    // count of active records
    uint32_t _head;     // position of the newest record
    uint32_t _tail;     // position of the oldest record
    uint32_t _idx;      // current index (newer records have index hgher than older ones)
    bool _circular;     // is buffer circular (head starts overwriting tail if capacity is reached)
    bool _open;         // was 'open' method called?
    File _file;

    void setHeadTail();

};


#include "FileBuffer.hpp"

#endif
