#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>
#include <cstdio>
#include <cstring>


void displayRelations() {
  /*
    Instiantiating relCatBuffer and AttrCatBuffer with RecBuffer
  */

  RecBuffer relCatBuffer(RELCAT_BLOCK); // index of the start of relation catalog buffer
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  /*
    Getting the headers from the buffers
  */

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;
  
  /* Loading the header data*/

  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);

  for(int i = 0, attrcatslotindex=0; i < relCatHeader.numEntries; i++) {
    /* Loading the relation one by one */
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    for(int j = 0; j < relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal; j++,attrcatslotindex++) {

      /* Loading the attribute record from attribute catalog for the relation one by one */
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      attrCatBuffer.getRecord(attrCatRecord, attrcatslotindex);

      //Checking if the relation name in the attribute catalog matches the relation name in the relation catalog
      if(strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal) == 0) {
        // Displaying the attribute name and its type
        const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER? "NUM" : "STR"; 
        printf(" %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
      }

      // once all the slots are traversed in the existing attribute catalog block, we update the block number
			// of the attrCatBuffer to the next block, and then attrCatHeader
			// is updated to the header of next block
      if(attrcatslotindex == attrCatHeader.numSlots - 1) {

        //If no more blocks are present, we break the loop
        if(attrCatHeader.rblock == -1) {
          break;
        }

        attrcatslotindex = -1; // reset the slot index for the next block
        attrCatBuffer=RecBuffer(attrCatHeader.rblock); // move to the next block
        attrCatBuffer.getHeader(&attrCatHeader); // update the header for the next block
    }
  }

    printf("\n");
}
}

void updateAttributeName(const char* relName, const char* oldAttrName, const char* newAttrName) {
  
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;

  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);

  for (int i = 0, attrIndex = 0; i < attrCatHeader.numEntries; i++, attrIndex++)
{
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    attrCatBuffer.getRecord(attrCatRecord, attrIndex);

    if(strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName) == 0 &&
       strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldAttrName) == 0)
    {
        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newAttrName);

        if(attrCatBuffer.setRecord(attrCatRecord, attrIndex) == SUCCESS)
            printf("Attribute updated successfully\n");

        return;
    }

    if(attrIndex == attrCatHeader.numSlots - 1)
    {
        if(attrCatHeader.rblock == -1)
            break;

        attrIndex = -1;
        attrCatBuffer = RecBuffer(attrCatHeader.rblock);
        attrCatBuffer.getHeader(&attrCatHeader);
    }
}
}

int main(int argc, char *argv[]) {

  Disk disk_run;

  
  updateAttributeName("Sample", "CGPA", "Marks");
  displayRelations();
  return 0;
  // StaticBuffer buffer;
  // OpenRelTable cache;

  //return FrontendInterface::handleFrontend(argc, argv);
}