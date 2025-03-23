#include "AttrCacheTable.h"

#include <cstring>

AttrCacheEntry *AttrCacheTable::attrCache[MAX_OPEN];

/* returns the attrOffset-th attribute for the relation corresponding to relId
NOTE: this function expects the caller to allocate memory for `*attrCatBuf`
*/
int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry *attrCatBuf)
{

    if (relId < 0 || relId > MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }
    if (attrCache[relId] == nullptr)
        return E_RELNOTOPEN;

    // traverse the linked list of attribute cache entries
    for (AttrCacheEntry *entry = attrCache[relId]; entry != nullptr; entry = entry->next)
    {
        if (entry->attrCatEntry.offset == attrOffset)
        {
            *attrCatBuf = entry->attrCatEntry;
            return SUCCESS;
        }
    }

    // there is no attribute at this offset
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setAttrCatEntry(int relId,int attrOffset,AttrCatEntry *attrCatBuf){
     if (relId < 0 || relId > MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }
    if (attrCache[relId] == nullptr)
        return E_RELNOTOPEN;

    for (auto *entry = attrCache[relId]; entry != nullptr; entry = entry->next)
    {
        if (entry->attrCatEntry.offset==attrOffset)
        {
            entry->attrCatEntry=*attrCatBuf;
            //set the dirty flag
            entry->dirty=true;

            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;

}

int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry *attrCatBuf)
{

    if (relId < 0 || relId > MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }
    if (attrCache[relId] == nullptr)
        return E_RELNOTOPEN;

    for (auto *entry = attrCache[relId]; entry != nullptr; entry = entry->next)
    {
        if (strcmp(entry->attrCatEntry.attrName, attrName) == 0)
        {
            *attrCatBuf=entry->attrCatEntry;
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}

// Sets the Attribute Catalog entry corresponding to the given attribute of the specified relation in the Attribute Cache Table.
int AttrCacheTable::setAttrCatEntry(int relId,char attrName[ATTR_SIZE],AttrCatEntry *attrCatBuf){
     if (relId < 0 || relId > MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }
    if (attrCache[relId] == nullptr)
        return E_RELNOTOPEN;

    for (auto *entry = attrCache[relId]; entry != nullptr; entry = entry->next)
    {
        if (strcmp(entry->attrCatEntry.attrName, attrName) == 0)
        {
            entry->attrCatEntry=*attrCatBuf;
            //set the dirty flag
            entry->dirty=true;

            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;

}




/* Converts a attribute catalog record to AttrCatEntry struct
    We get the record as Attribute[] from the BlockBuffer.getRecord() function.
    This function will convert that to a struct AttrCatEntry type.
*/
void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS],
                                          AttrCatEntry *attrCatEntry)
{
    strcpy(attrCatEntry->relName, record[ATTRCAT_REL_NAME_INDEX].sVal);
    strcpy(attrCatEntry->attrName, record[ATTRCAT_ATTR_NAME_INDEX].sVal);
    attrCatEntry->attrType = (int)record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
    attrCatEntry->primaryFlag = (int)record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
    attrCatEntry->rootBlock = (int)record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
    attrCatEntry->offset = (int)record[ATTRCAT_OFFSET_INDEX].nVal;
}

void AttrCacheTable::attrCatEntryToRecord(AttrCatEntry *attrCatEntry, Attribute record[ATTRCAT_NO_ATTRS])
{
    strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal, attrCatEntry->relName);
    strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal, attrCatEntry->attrName);

    record[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrCatEntry->attrType;
    record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = attrCatEntry->primaryFlag;
    record[ATTRCAT_ROOT_BLOCK_INDEX].nVal = attrCatEntry->rootBlock;
    record[ATTRCAT_OFFSET_INDEX].nVal = attrCatEntry->offset;

    // copy the rest of the fields in the record to the attrCacheEntry struct
}

//Gives the value of searchIndex field of the given attribute in the specified relation from Attribute Cache Table
int AttrCacheTable::getSearchIndex(int relId,char attrName[ATTR_SIZE],IndexId *searchIndex){
    //if relId is outsied range
    if(relId<0 || relId>=MAX_OPEN){
        return E_OUTOFBOUND;
    }
    //if rel is not open
    if(attrCache[relId]==nullptr){
        return E_RELNOTOPEN;
    }

    for(auto *entry=attrCache[relId];entry!=nullptr;entry=entry->next){
        if(strcmp(attrName,entry->attrCatEntry.attrName)==0){
            *searchIndex=entry->searchIndex;
            return SUCCESS;
        }
    }

    return E_ATTRNOTEXIST;

}

int AttrCacheTable::getSearchIndex(int relId,int attrOffset,IndexId *searchIndex){
    //if relId is outsied range
    if(relId<0 || relId>=MAX_OPEN){
        return E_OUTOFBOUND;
    }
    //if rel is not open
    if(attrCache[relId]==nullptr){
        return E_RELNOTOPEN;
    }

    for(auto *entry=attrCache[relId];entry!=nullptr;entry=entry->next){
        if(entry->attrCatEntry.offset==attrOffset){
            *searchIndex=entry->searchIndex;
            return SUCCESS;
        }
    }

    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setSearchIndex(int relId,char attrName[ATTR_SIZE],IndexId *searchIndex){
    //if relId is outsied range
    if(relId<0 || relId>=MAX_OPEN){
        return E_OUTOFBOUND;
    }
    //if rel is not open
    if(attrCache[relId]==nullptr){
        return E_RELNOTOPEN;
    }

    for(auto *entry=attrCache[relId];entry!=nullptr;entry=entry->next){
        if(strcmp(attrName,entry->attrCatEntry.attrName)==0){
            entry->searchIndex=*searchIndex;
            return SUCCESS;
        }
    }

    return E_ATTRNOTEXIST;

}

int AttrCacheTable::setSearchIndex(int relId,int attrOffset,IndexId *searchIndex){
    //if relId is outsied range
    if(relId<0 || relId>=MAX_OPEN){
        return E_OUTOFBOUND;
    }
    //if rel is not open
    if(attrCache[relId]==nullptr){
        return E_RELNOTOPEN;
    }

    for(auto *entry=attrCache[relId];entry!=nullptr;entry=entry->next){
        if(entry->attrCatEntry.offset==attrOffset){
            entry->searchIndex=*searchIndex;
            return SUCCESS;
        }
    }

    return E_ATTRNOTEXIST;
}

int AttrCacheTable::resetSearchIndex(int relId,char attrName[ATTR_SIZE]){

    IndexId index={-1,-1};
    int ret=AttrCacheTable::setSearchIndex(relId,attrName,&index);

    return ret;
}

int AttrCacheTable::resetSearchIndex(int relId,int attrOffset){
    
    IndexId index={-1,-1};
    int ret=AttrCacheTable::setSearchIndex(relId,attrOffset,&index);

    return ret;
}
