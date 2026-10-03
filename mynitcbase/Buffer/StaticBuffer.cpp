#include "StaticBuffer.h"
#include<stdio.h>

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {

  // copy blockAllocMap blocks from disk to buffer (using readblock() of disk)
  for(int i=0,index=0;i<4;i++){
    unsigned char buffer[BLOCK_SIZE];
    Disk::readBlock(buffer,i);
    for(int j=0;j<BLOCK_SIZE;j++,index++){
      blockAllocMap[index] = buffer[j];
    }
  }

  // initialise all blocks as free
  for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
    metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].blockNum = INVALID_BLOCKNUM;
    metainfo[bufferIndex].dirty = false;
    metainfo[bufferIndex].timeStamp = 0;
  }
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer() {
  for(int i=0;i<4;i++){
    unsigned char buffer[BLOCK_SIZE];
    for(int j=0;j<BLOCK_SIZE;j++){
      buffer[j] = blockAllocMap[i*BLOCK_SIZE+j];
    }
    Disk::writeBlock(buffer,i);
  }

  for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
     if(!metainfo[bufferIndex].free && metainfo[bufferIndex].dirty) {
            Disk::writeBlock(blocks[bufferIndex], metainfo[bufferIndex].blockNum);
        }
  }
}

int StaticBuffer::getFreeBuffer(int blockNum) {
  if(blockNum < 0 || blockNum > DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }
    // increase the timeStamp in metainfo of all occupied buffers
    
    for(int idx = 0; idx < BUFFER_CAPACITY; idx++) {
        if(metainfo[idx].free == false) {
            metainfo[idx].timeStamp++;
        }
    }
    int allocatedBuffer = -1;
    int timeStamp = -1;
    int index = -1;
    for(int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        if(metainfo[bufferIndex].timeStamp > timeStamp) {
            timeStamp = metainfo[bufferIndex].timeStamp;
            index = bufferIndex;
        }
        if(metainfo[bufferIndex].free == true) {
            allocatedBuffer = bufferIndex;
            break;
        }
    }
    // write back functionality 
    if(allocatedBuffer==-1 && metainfo[index].dirty) {
        Disk::writeBlock(blocks[index], metainfo[index].blockNum);
        allocatedBuffer = index;
    }
    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].blockNum = blockNum;
    metainfo[allocatedBuffer].dirty = false;
    metainfo[allocatedBuffer].timeStamp = 0;
    return allocatedBuffer;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/


int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS)
  // and return E_OUTOFBOUND if not valid.
  if(blockNum < 0 || blockNum > DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  // find and return the bufferIndex which corresponds to blockNum (check metainfo)
  for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
    if (!metainfo[bufferIndex].free && metainfo[bufferIndex].blockNum == blockNum) {
      return bufferIndex;
    }
  }

  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum) {
   int bufferNum = getBufferNum(blockNum);
    if(bufferNum == E_BLOCKNOTINBUFFER) {
        return E_BLOCKNOTINBUFFER;
    }
    if(bufferNum == E_OUTOFBOUND) {
        return E_OUTOFBOUND;
    }
    else {
        metainfo[bufferNum].dirty = true;
    }
    return SUCCESS;
}







  