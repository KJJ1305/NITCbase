#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char relname[ATTR_SIZE]) {
    // the OpenRelTable::openRel() function returns the rel-id if successful
  // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
  // error codes will be negative
   int ret=OpenRelTable::openRel(relname);

   if(ret>=0){
    return SUCCESS;
   }

   return ret;
}

int Schema::closeRel(char relname[ATTR_SIZE]) {
  
    if(strcmp(relname, RELCAT_RELNAME) == 0 || strcmp(relname, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

     // this function returns the rel-id of a relation if it is open or
    // E_RELNOTOPEN if it is not. we will implement this later.

    int relId = OpenRelTable::getRelId(relname);

    if(relId < 0) {
        return E_RELNOTOPEN;
    }

    return OpenRelTable::closeRel(relId);

}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
    
    if(strcmp(oldRelName, RELCAT_RELNAME) == 0 || strcmp(oldRelName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    if(strcmp(newRelName, RELCAT_RELNAME) == 0 || strcmp(newRelName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    if(OpenRelTable::getRelId(oldRelName) >= 0) {
        return E_RELOPEN;
    }

    int retVal = BlockAccess::renameRelation(oldRelName, newRelName);
    return retVal;
}

int Schema::renameAttr(char relName[ATTR_SIZE], char oldAttrName[ATTR_SIZE], char newAttrName[ATTR_SIZE]) {
    
    if(strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    if(OpenRelTable::getRelId(relName) >= 0) {
        return E_RELOPEN;
    }

    int retVal = BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
    return retVal;
}


