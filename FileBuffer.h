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

#ifndef __FILEBUFFER_H__
#define __FILEBUFFER_H__

#include <Arduino.h>
#include <FS.h>

#define DEBUG_FILEBUFFER
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
     * Retrieve oldest record from the buffer without removing it
     */
    const T peek(size_t idx);

    /**
     * Retrieve raw record from the buffer storage. 
     * Provided object will be filled with data.
     * False is returned if raw record does not contain any data.
     */
    bool getRaw(size_t idx, T* rec);

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

    void showBuff();


    private:
    uint16_t _count;    // count of active records
    uint32_t _head;     // position of the newest record
    uint32_t _tail;     // position of the oldest record
    uint32_t _idx;      // current index (newer records have index hgher than older ones)
    bool _circular;     // is buffer circular (head starts overwriting tail if capacity is reached)
    File _file;

    void setHeadTail();

};


template<typename T, size_t S>
constexpr FileBuffer<T,S>::FileBuffer() {
}

template<typename T, size_t S>
bool FileBuffer<T,S>::open(const char* fileName, bool reset, bool circular){
    const char* module = "fbuff:begin";
    _circular = circular;
    
    reset = reset | !SPIFFS.exists(fileName);

    _file = SPIFFS.open(fileName, reset?"w+":"r+");
    if (!_file) {
        DEBUG_FB_PRINT("[%s] failed opening buffer file %s\n", module, fileName);
        return 0;
    }

    // do test read in order to detect corrupted file
    uint8_t b;
    if (_file.available() && _file.read(&b,1) != 1){
        DEBUG_FB_PRINT("[%s] ERROR - buffer file corrupted\n", module);
        return 0;
    }

    reset |= _file.size() < maxFileSize; // if file is too small

    if (reset){
        clear();
    }

    DEBUG_FB_PRINT("[%s] buffer file ready '%s'\n", module, fileName);

    // locate buffer head & tail
    setHeadTail();

    DEBUG_FB_PRINT("[%s] head=%u tail=%u\n", module, _head, _tail);

    return 1;
}

template<typename T, size_t S>
void FileBuffer<T,S>::close(){
    _file.close();
}

template<typename T, size_t S>
void FileBuffer<T,S>::setHeadTail(){
    T rec;
    
    FILEBUFFER_IDX_TYPE tidx = 0;   // tail index
    FILEBUFFER_IDX_TYPE ridx = 0;   // record index

    _idx = 0;   // set head index to zero
    _head = 0;  // set buffer head to beggining of the file
    _tail = 0;
    _count = 0;
    
    _file.seek(0,SeekSet);

    size_t frs = FILEBUFFER_IDX_SIZE + recordSize;  // full record size (including index)
    size_t rbc = frs;              // read byte count

    while (rbc == frs){  // read while there is anything to read

        rbc = _file.read((uint8_t*)&ridx, FILEBUFFER_IDX_SIZE);
        rbc += _file.read((uint8_t*)&rec, recordSize);

        if (rbc == frs) {           // if complete record was read succesfully
            DEBUG_FB_PRINT("\n[fbuff:setHT] %u, i=%u", _file.position()-rbc, ridx);

            if (ridx > _idx) {   // if current record is newer then previously found
                _head = _file.position() - rbc;  // head points to the beginning of the record
                _idx = ridx;
                _count++;
                DEBUG_FB_PRINT(" head");
                if (tidx == 0) {
                    tidx = ridx;  // if tail was not found yet, then this is it
                    DEBUG_FB_PRINT(" tail");
                }
            } else {                // if current is older or or not active
                if (ridx > 0) {  // if active record    
                    _count++;
                    if (ridx < tidx) {   // if it is older record, then new tail was found
                        _tail = _file.position() - rbc;
                        tidx = ridx;
                        DEBUG_FB_PRINT(" tail");
                    } 

                }
            }
        } 
    }

    DEBUG_FB_PRINT("\n");

    DEBUG_FB_PRINT("[fbuff:setHead] index=%u, head=%u, tail=%u\n", _idx, _head, _tail);

}

template<typename T, size_t S>
bool FileBuffer<T,S>::push(T record){
    
    if (isFull() && !_circular) abort();

    size_t frs = FILEBUFFER_IDX_SIZE + recordSize;

    uint32_t wpos = _head + (isEmpty()?0:frs);  // writing position

    //DEBUG_FB_PRINT("[fbuff:push] wpos=%u, head=%u, frs=%u, size=%u, tail=%u\n", wpos, _head, frs, capacity, _tail);

    if (wpos >= maxFileSize){ // in case writing position will be past buffer max size
        wpos = 0;
    }

    if (!isEmpty() && wpos == _tail) { // if not empty and head would overwrite tail, then move tail
        _tail += frs;
        if (_tail >= maxFileSize) _tail = 0;  // if tail will go outside buffer, then set it to begining
    }

    _head = wpos;
    _file.seek(wpos, SeekSet);

    _idx++;
    size_t wbc = _file.write((uint8_t*)&_idx, FILEBUFFER_IDX_SIZE);
    wbc += _file.write((uint8_t*)&record, recordSize);
    _file.flush();

    if (!isFull()) _count++;

    DEBUG_FB_PRINT("[fbuff:push] index=%u, head=%u, tail=%u, size=%u\n", _idx, _head, _tail, _count);

    return (wbc == frs);

}

template<typename T, size_t S>
const T FileBuffer<T,S>::pop(){
    if (isEmpty()) abort();

    FILEBUFFER_IDX_TYPE ridx = 0;
    T rec;

    _file.seek(_tail, SeekSet);

    size_t rbc = _file.write((uint8_t*)&ridx, FILEBUFFER_IDX_SIZE);  // deactivate record writing zero-index
    rbc += _file.read((uint8_t*)&rec, recordSize);
    _file.flush();

    _count--;

    if (_head == _tail){ // this was the last record
        _head = 0;
        _tail = 0;
        _idx = 0;
    } else {
        _tail = _file.position();
    }

    if (_tail >= maxFileSize)    // if tail has moved to the end of file
        _tail = 0;               // move it to the beginning

    DEBUG_FB_PRINT("[fbuff:pop] head=%u, tail=%u, size=%u\n", _head, _tail, _count);

    return rec;

}

template<typename T, size_t S>
void FileBuffer<T,S>::showBuff(){

    FILEBUFFER_IDX_TYPE ridx = 0;
    T rec;

    _file.seek(0, SeekSet);

    DEBUG_FB_PRINT("BUFFER: size=%d elements=",_count);

    for(int i=0;i<capacity;i++){
        size_t rbc = _file.read((uint8_t*)&ridx, FILEBUFFER_IDX_SIZE);  // deactivate record writing zero-index
        rbc += _file.read((uint8_t*)&rec, recordSize);
        DEBUG_FB_PRINT("%u ",ridx);
    }

    DEBUG_FB_PRINT("\n");

}

template<typename T, size_t S>
const T FileBuffer<T,S>::peek(size_t idx){
    if (idx >= _count) abort();
    uint32_t pos = _tail;

    for(int i=0; i<idx; i++){
        pos += (FILEBUFFER_IDX_SIZE + recordSize);
        if (pos >= maxFileSize) pos = 0;
    }

    _file.seek(pos, SeekSet);
    T rec;
    FILEBUFFER_IDX_TYPE ii = 0;
    _file.read((uint8_t*)&ii, FILEBUFFER_IDX_SIZE);
    _file.read((uint8_t*)&rec, recordSize);

    return rec;
}

template<typename T, size_t S>
bool FileBuffer<T,S>::getRaw(size_t idx, T* rec){
    if (idx >= capacity) abort();
    uint32_t pos = 0;

    for(int i=0; i<idx; i++){
        pos += (FILEBUFFER_IDX_SIZE + recordSize);
        if (pos >= maxFileSize) pos = 0;
    }

    _file.seek(pos, SeekSet);
    FILEBUFFER_IDX_TYPE ii = 0;
    _file.read((uint8_t*)&ii, FILEBUFFER_IDX_SIZE);
    if (ii)
        _file.read((uint8_t*)rec, recordSize);

    return ii != 0;
}

template<typename T, size_t S>
void FileBuffer<T,S>::clear(){
    
    DEBUG_FB_PRINT("[fbuff:flush] flushing buffer");
    T rec;
    FILEBUFFER_IDX_TYPE idx = 0;

    for (uint16_t i=0; i<capacity; i++){
        _file.write((uint8_t*)&idx, FILEBUFFER_IDX_SIZE);
        _file.write((uint8_t*)&rec, recordSize);
        if (i%10 == 0){
            DEBUG_FB_PRINT(".");
        }
    }

    _file.flush();

    DEBUG_FB_PRINT(" done\n");

}

#endif
