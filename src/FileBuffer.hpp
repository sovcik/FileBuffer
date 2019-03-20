#pragma once
#include "FileBuffer.h"
#include "Arduino.h"

template<typename T, size_t S>
constexpr FileBuffer<T,S>::FileBuffer() {
}

template<typename T, size_t S>
bool FileBuffer<T,S>::open(const char* fileName, bool reset, bool circular){
    #ifdef DEBUG_FILEBUFFER
    const char* module = "fbuff:begin";
    #endif
    _circular = circular;

    if (reset){
        if (!SPIFFS.remove(fileName)){
            DEBUG_FB_PRINT("[%s] ERROR failed to remove old buffer file '%s', exists=%d, reset=%d\n", module, fileName, SPIFFS.exists(fileName), reset);
        }
    }
    
    reset = reset | !SPIFFS.exists(fileName);

    _file = SPIFFS.open(fileName, reset?"w+":"r+");
    if (!_file) {
        FSInfo fi;
        DEBUG_FB_PRINT("[%s] failed opening buffer file %s, exists=%d, reset=%d\n", module, fileName, SPIFFS.exists(fileName), reset);
        if (SPIFFS.info(fi))
            DEBUG_FB_PRINT("[%s] buffer file info:total bytes=%u used bytes=%u\n", module, fi.totalBytes, fi.usedBytes);
        
        return 0;
    }

    // do test read in order to detect corrupted file
    uint8_t b;
    if (_file.available() && _file.read(&b,1) != 1){
        DEBUG_FB_PRINT("[%s] ERROR - buffer file corrupted\n", module);
        return 0;
    }

    reset |= _file.size() < maxFileSize; // if file is too small

    _open = true;

    if (reset){
        clear();
    }

    DEBUG_FB_PRINT("[%s] buffer file open '%s', record size=%u\n", module, fileName, sizeof(T));

    // locate buffer head & tail
    size_t corruptedAt = setHeadTail();
    if (corruptedAt){
        DEBUG_FB_PRINT("[%s] ERROR buffer file corrupted at %u\n", module, corruptedAt);
        return 0;
    }

    DEBUG_FB_PRINT("[%s] head=%u tail=%u size=%u\n", module, _head, _tail, _count);
    
    return 1;
}

template<typename T, size_t S>
void FileBuffer<T,S>::close(){
    _file.close();
    _open = false;
}

template<typename T, size_t S>
size_t FileBuffer<T,S>::setHeadTail(){
    T* rec = new T();
    size_t corruptedAt = 0;
    
    FILEBUFFER_IDX_TYPE tidx = 0;   // tail index
    FILEBUFFER_IDX_TYPE ridx = 0;   // record index
    FILEBUFFER_IDX_TYPE pidx = 0;   // previous record index (used for detecting corruption)

    _idx = 0;   // set head index to zero
    _head = 0;  // set buffer head to beggining of the file
    _tail = 0;
    _count = 0;
    
    uint8_t falling = 0; // count of falling edges
    
    _file.seek(0,SeekSet);

    size_t frs = FILEBUFFER_IDX_SIZE + recordSize;  // full record size (including index)
    size_t rbc = frs;              // read byte count

    while (rbc == frs){  // read while there is anything to read

        rbc = _file.read((uint8_t*)&ridx, FILEBUFFER_IDX_SIZE);   // read index
        rbc += _file.read((uint8_t*)rec, recordSize);  // read stored record

        if (pidx > ridx) {
            falling++;  // count falling edges
            if (falling > 1) {  // there can be only one falling edge in the whole file
                corruptedAt = _file.position()-rbc;
                DEBUG_FB_PRINT("\n[fbuff:setHT] ERROR corrupted file at postion %u\n", corruptedAt);
                rbc = 0;  // exit while loop
            }
        }

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

        pidx = ridx;
    }

    DEBUG_FB_PRINT("\n");

    delete rec;

    if (!corruptedAt)
        DEBUG_FB_PRINT("[fbuff:setHead] index=%u, head=%u, tail=%u, count=%u\n", _idx, _head, _tail, _count);

    return corruptedAt;

}

template<typename T, size_t S>
bool FileBuffer<T,S>::push(T record){
    
    if (!_open || (isFull() && !_circular)) abort();

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

    _head = wpos; // move head to new position
    _file.seek(wpos, SeekSet);

    _idx++;  // increment index to mark newest record

    // write record to buffer
    size_t wbc = _file.write((uint8_t*)&_idx, FILEBUFFER_IDX_SIZE);
    wbc += _file.write((uint8_t*)&record, recordSize);
    _file.flush();

    if (!isFull()) _count++;

    DEBUG_FB_PRINT("[fbuff:push] index=%u, head=%u, tail=%u, size=%u\n", _idx, _head, _tail, _count);

    return (wbc == frs);

}

template<typename T, size_t S>
const T FileBuffer<T,S>::pop(){
    if (!_open || isEmpty()) abort();

    FILEBUFFER_IDX_TYPE ridx = 0;
    T rec;

    _file.seek(_tail, SeekSet);

    // deactivate record writing zero-index
    size_t rbc = _file.write((uint8_t*)&ridx, FILEBUFFER_IDX_SIZE);  
    // read record content
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

#ifdef DEBUG_FILEBUFFER
template<typename T, size_t S>
void FileBuffer<T,S>::showBuff(){

    if (!_open) abort();

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
#endif

template<typename T, size_t S>
const T FileBuffer<T,S>::peek(size_t idx){
    if (!_open || idx >= _count) abort();
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
    if (!_open || idx >= capacity) abort();
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
    if (!_open) abort();
    
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