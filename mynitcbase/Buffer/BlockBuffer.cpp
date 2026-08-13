#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer(int blockNum) {
    this->blockNum = blockNum;
}

RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {} /* Constructor Initializer*/

int BlockBuffer::getHeader(struct HeadInfo* head) {
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
    return ret;   // return any errors that might have occured in the process
    }

    memcpy(&head->numSlots, bufferPtr+24, 4);
    memcpy(&head->numAttrs, bufferPtr+20, 4);
    memcpy(&head->numEntries, bufferPtr+16, 4);
    memcpy(&head->rblock, bufferPtr+12, 4);
    memcpy(&head->lblock, bufferPtr+8, 4);
    memcpy(&head->pblock, bufferPtr+4, 4);

    return SUCCESS;
}

/*
    Need Review
*/
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
    HeadInfo head;

    BlockBuffer::getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

     unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
    return ret;
    }

    int recordStart = HEADER_SIZE + slotCount + slotNum * (ATTR_SIZE * attrCount);
    int recordSize = ATTR_SIZE * attrCount;

    unsigned char *start = bufferPtr + recordStart;

    memcpy(rec, start, recordSize);
    return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    HeadInfo head;
    BlockBuffer::getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;
    
    unsigned char buffer[BLOCK_SIZE];
    Disk::readBlock(buffer, this->blockNum);

    int recordStart = HEADER_SIZE + slotCount + slotNum * (ATTR_SIZE * attrCount);
    int recordSize = ATTR_SIZE * attrCount;

    memcpy(buffer + recordStart, rec, recordSize);
    Disk::writeBlock(buffer, this->blockNum);
    
    return SUCCESS;

}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr){

  // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }
   // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}

