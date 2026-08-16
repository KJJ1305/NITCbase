#include "BlockAccess.h"
#include <cstring>
#include <iostream>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {

    //previous record id to keep track of the last record found
    //(use RelCacheTable::getSearchIndex() function)
    RecId prevRecId = {-1, -1};
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    // let block and slot denote the record id of the record being currently checked
    
    int block,slot;
    if(prevRecId.block == -1 && prevRecId.slot == -1) {
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);
        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    RelCatEntry relCatBuffer;
    RelCacheTable::getRelCatEntry(relId, &relCatBuffer);
    while(block != -1){

        RecBuffer recBuffer(block);
        HeadInfo head;
        recBuffer.getHeader(&head);

        unsigned char *slotMap = (unsigned char*)malloc(sizeof(unsigned char) * head.numSlots);
        recBuffer.getSlotMap(slotMap);

        if(slot >= head.numSlots) {
            block = head.rblock;
            slot = 0;
            continue;
        }

        if(slotMap[slot] == 0) {
            slot++;
            continue;
        }

        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

        Attribute *record = (Attribute *)malloc(sizeof(Attribute) * head.numAttrs);
        recBuffer.getRecord(record, slot);

        int attrOffset = attrCatEntry.offset;
        int cmpVal = compareAttrs(record[attrOffset], attrVal, attrCatEntry.attrType);

         if((op == NE && cmpVal != 0 ) ||(op == LT && cmpVal < 0) ||(op == LE && cmpVal <= 0) ||(op == EQ && cmpVal == 0) ||(op == GT && cmpVal > 0) ||(op == GE && cmpVal > 0)) {
            RecId newSearchIndex;
            newSearchIndex.block = block;
            newSearchIndex.slot = slot;
            RelCacheTable::setSearchIndex(relId, &newSearchIndex);
            return RecId{block, slot};
        }   
        slot++;
    }
    return RecId{-1,-1};

   }

  