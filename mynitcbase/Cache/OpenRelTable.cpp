#include "OpenRelTable.h"

#include <cstring>
#include <stdlib.h>
#include <iostream>
OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {
    for(int i = 0; i < MAX_OPEN; i++) {
        tableMetaInfo[i].free = true;
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }
    
    RecBuffer relCatBuffer(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    HeadInfo relCatHeader;
    relCatBuffer.getHeader(&relCatHeader);
    
    for(int i = 0; i <= ATTRCAT_RELID; i++) {
        relCatBuffer.getRecord(relCatRecord, i); 
        RelCacheEntry relCacheEntry;
        RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
        relCacheEntry.recId.block = RELCAT_BLOCK;
        relCacheEntry.recId.slot = i;
        relCacheEntry.searchIndex = {-1, -1};

        RelCacheTable::relCache[i] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
        *(RelCacheTable::relCache[i]) = relCacheEntry;
    }

    RecBuffer attrCatBuffer(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    AttrCacheEntry* attrCacheEntry = nullptr;
    AttrCacheEntry* head = nullptr;
    AttrCacheEntry* prev = nullptr;

   for(int i = RELCAT_RELID, attrSlotIndex = 0; i <= ATTRCAT_RELID ; i++) {
        RelCatEntry relCatEntry;
        HeadInfo attrCatHeader;
        attrCatBuffer.getHeader(&attrCatHeader);
        relCatEntry = RelCacheTable::relCache[i]->relCatEntry;
        int noOfAttributes = relCatEntry.numAttrs;
        for(int j = 0; j < noOfAttributes; j++, attrSlotIndex++) {
            attrCatBuffer.getRecord(attrCatRecord, attrSlotIndex);
            attrCacheEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
            AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
            attrCacheEntry->recId.block = ATTRCAT_BLOCK;
            attrCacheEntry->recId.slot = attrSlotIndex; 

            if(!head) {
                head = attrCacheEntry;
                prev = attrCacheEntry; 
                continue;
            }
            if(attrSlotIndex == attrCatHeader.numSlots - 1) {
                attrSlotIndex = -1;
                HeadInfo attrCatHeader; 
                attrCatBuffer.getHeader(&attrCatHeader);
                attrCatBuffer = RecBuffer(attrCatHeader.rblock);
            }
            prev->next = attrCacheEntry;
            prev = attrCacheEntry;
        }
        attrCacheEntry->next = nullptr;
        AttrCacheTable::attrCache[i] = head;
        head = nullptr;
        prev = nullptr;
   }

    tableMetaInfo[RELCAT_RELID].free = false;
    tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(tableMetaInfo[RELCAT_RELID].relName, "RELATIONCAT");
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName, "ATTRIBUTECAT");
}

int OpenRelTable::getFreeOpenRelTableEntry() {

  /* traverse through the tableMetaInfo array,
    find a free entry in the Open Relation Table.*/
    for(int i = 0; i < MAX_OPEN; i++) {
        if(tableMetaInfo[i].free) {
            return i;
        }
    }

  // if found return the relation id, else return E_CACHEFULL.
  return E_CACHEFULL;
}

void freeLinkedList(AttrCacheEntry** head) {
    if (!head || !*head) return;  
    
    AttrCacheEntry* current = *head;
    while (current) {
        AttrCacheEntry* nextNode = current->next;  
        free(current);  
        current = nextNode; 
    }
    
    *head = nullptr;
}

/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

     /* traverse through the tableMetaInfo array,
    find the entry in the Open Relation Table corresponding to relName.*/
  for(int i = 0; i < MAX_OPEN; i++) {
        if(!tableMetaInfo[i].free && strcmp(tableMetaInfo[i].relName, relName) == 0) {
            return i;
        }
    }

  return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
    int relCheck = getRelId(relName);
    if(relCheck != E_RELNOTOPEN) {
        return relCheck;
    }
    
    int emptySlot = getFreeOpenRelTableEntry();
    if(emptySlot == E_CACHEFULL) {
        return E_CACHEFULL;
    }

    int relId = emptySlot;
    // Setting up Relation Cache Entry of the relation
    // first search for the relation entry in the relation catalog

    Attribute attrVal;
    strcpy(attrVal.sVal, relName);

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    RecId searchIndex = BlockAccess::linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, attrVal, EQ);

    if(searchIndex.block == -1 && searchIndex.slot == -1) {
        return E_RELNOTEXIST;
    }

    // loading the records with block and slot into relation catalog

    RecBuffer relationBuffer(searchIndex.block);
    Attribute record[RELCAT_NO_ATTRS];
    relationBuffer.getRecord(record, searchIndex.slot);

    RelCacheEntry *relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(record, &relCacheEntry->relCatEntry);

    relCacheEntry->recId = searchIndex;
    relCacheEntry->searchIndex = {-1, -1};
    RelCacheTable::relCache[relId] = relCacheEntry;

    // loading the attributes to the attribute cache

    Attribute attrRecord[ATTRCAT_NO_ATTRS];
    AttrCacheEntry* prev = nullptr;
    AttrCacheEntry* head = nullptr;

    int numberOfAttributes = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    for(int i = 0; i < numberOfAttributes; i++) {
        RecId attributeSearchIndex = BlockAccess::linearSearch(ATTRCAT_RELID, (char*)RELCAT_ATTR_RELNAME, attrVal, EQ);

        if(attributeSearchIndex.block == -1 || attributeSearchIndex.slot == -1) {
            free(relCacheEntry);
            freeLinkedList(&head);
            RelCacheTable::relCache[relId] = nullptr;
            return E_ATTRNOTEXIST;
        }

        RecBuffer attrCatBlock = RecBuffer(attributeSearchIndex.block);
        attrCatBlock.getRecord(attrRecord, attributeSearchIndex.slot);
        AttrCacheEntry* attrCacheEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrRecord, &(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId = attributeSearchIndex;
        
        if(!head) {
            head = attrCacheEntry;
        } else {
            prev->next = attrCacheEntry;
        }
        prev = attrCacheEntry;
    }
    if(prev != nullptr) {
        prev->next = nullptr;
    }
    AttrCacheTable::attrCache[relId] = head;

    //update the metainfoa
    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);
    
    // debugging


    return relId;
}

int OpenRelTable::closeRel(int relId){
    if(relId==0 || relId==1){
        return E_NOTPERMITTED;
    }

    if(relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }

    if(tableMetaInfo[relId].free) {
        return E_RELNOTOPEN;
    }
    if(RelCacheTable::relCache[relId]->dirty) {
        Attribute record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry), record);
        
        RecId recData = RelCacheTable::relCache[relId]->recId;
        RecBuffer relCatBuffer(recData.block);
        relCatBuffer.setRecord(record, recData.slot);

    }

     // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function
    free(RelCacheTable::relCache[relId]);
    freeLinkedList(&AttrCacheTable::attrCache[relId]);

  // update `tableMetaInfo` to set `relId` as a free slot
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
  
    tableMetaInfo[relId].free = true;
    RelCacheTable::relCache[relId] = nullptr;
    AttrCacheTable::attrCache[relId] = nullptr;

  return SUCCESS;
}

OpenRelTable::~OpenRelTable() {
    for (int i = 2; i < MAX_OPEN; ++i) {
    if (tableMetaInfo[i].free==false) {
      OpenRelTable::closeRel(i); // we will implement this function later
    }
  }

    free(RelCacheTable::relCache[0]);
    free(RelCacheTable::relCache[1]);

    freeLinkedList(&AttrCacheTable::attrCache[0]);
    freeLinkedList(&AttrCacheTable::attrCache[1]);
}