#include "OpenRelTable.h"
#include <cstring>
#include <iostream>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable()
{

    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; ++i)
    {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        OpenRelTable::tableMetaInfo[i].free = true;
    }

    /************ Setting up Relation Cache entries ************/
    // (we need to populate relation cache with entries for the relation catalog
    //  and attribute catalog.)

    /**** setting up Relation Catalog relation in the Relation Cache Table****/
    RecBuffer relCatBlock(RELCAT_BLOCK);

    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

    struct RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

    // allocate this on the heap because we want it to persist outside this function
    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

    /**** setting up Attribute Catalog relation in the Relation Cache Table ****/

    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    struct RelCacheEntry attrCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &attrCacheEntry.relCatEntry);
    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
    attrCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrCacheEntry;

    /************ Setting up Attribute cache entries ************/
    // (we need to populate attribute cache with entries for the relation catalog
    //  and attribute catalog.)

    /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    struct AttrCacheEntry *head = nullptr;
    struct AttrCacheEntry *curr = nullptr;

    for (int i = 0; i < RELCAT_NO_ATTRS; i++)
    {
        attrCatBlock.getRecord(attrCatRecord, i);

        struct AttrCacheEntry *newentry = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newentry->attrCatEntry);
        newentry->recId.block = ATTRCAT_BLOCK;
        newentry->recId.slot = i;
        newentry->next = nullptr;

        if (head == nullptr)
        {
            head = newentry;
            curr = newentry;
        }
        else
        {
            curr->next = newentry;
            curr = newentry;
        }
    }

    // set the next field in the last entry to nullptr
    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    head = nullptr;
    curr = nullptr;

    for (int i = RELCAT_NO_ATTRS; i < RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS; i++)
    {
        attrCatBlock.getRecord(attrCatRecord, i);

        struct AttrCacheEntry *newentry = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newentry->attrCatEntry);
        newentry->recId.block = ATTRCAT_BLOCK;
        newentry->recId.slot = i;
        newentry->next = nullptr;

        if (head == nullptr)
        {
            head = newentry;
            curr = newentry;
        }
        else
        {
            curr->next = newentry;
            curr = newentry;
        }
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

    /************ Setting up tableMetaInfo entries ************/
    tableMetaInfo[RELCAT_RELID].free = false;
    tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
}

int OpenRelTable::getFreeOpenRelTableEntry()
{
    for (int i = 0; i < MAX_OPEN; i++)
    {
        if (tableMetaInfo[i].free == true)
        {
            return i;
        }
    }
    return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{

    for (int i = 0; i < MAX_OPEN; i++)
    {
        if (tableMetaInfo[i].free == false && strcmp(relName, tableMetaInfo[i].relName) == 0)
        {
            return i;
        }
    }

    return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE])
{

    int getid = getRelId(relName);
    if (getid != E_RELNOTOPEN)
    {
        return getid;
    }

    int freeSlot = OpenRelTable::getFreeOpenRelTableEntry();

    if (freeSlot == E_CACHEFULL)
    {
        return E_CACHEFULL;
    }

    int relId = freeSlot;

    /****** Setting up Relation Cache entry for the relation ******/

    Attribute Relcacheattr;
    memcpy(Relcacheattr.sVal, relName, ATTR_SIZE);
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME, Relcacheattr, EQ);

    if (relcatRecId.block == -1 && relcatRecId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    RecBuffer relcatRecord(relcatRecId.block);
    Attribute record[RELCAT_NO_ATTRS];
    relcatRecord.getRecord(record, relcatRecId.slot);
    RelCacheEntry *relcacheEntry=(RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(record, &(relcacheEntry->relCatEntry));

    relcacheEntry->recId=relcatRecId;
    RelCacheTable::relCache[relId] = relcacheEntry;

    /****** Setting up Attribute Cache entry for the relation ******/
    AttrCacheEntry *listHead = nullptr;
    AttrCacheEntry *curr = nullptr;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    
    
    while (true)
    {
        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID,(char *)RELCAT_ATTR_RELNAME, Relcacheattr, EQ);

        if (attrcatRecId.block == -1 && attrcatRecId.slot == -1)
            break;

        struct AttrCacheEntry *attrCacheEntry = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        RecBuffer attrCatBuff(attrcatRecId.block);
        attrCatBuff.getRecord(attrCatRecord, attrcatRecId.slot);

        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));

        attrCacheEntry->recId = attrcatRecId;
        attrCacheEntry->next = nullptr;

        if (listHead == nullptr)
        {
            listHead = attrCacheEntry;
            curr = attrCacheEntry;
        }
        else
        {
            curr->next = attrCacheEntry;
            curr = attrCacheEntry;
        }
    }

    AttrCacheTable::attrCache[relId] = listHead;

    /****** Setting up metadata in the Open Relation Table for the relation******/

    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);

    return relId;
}

int OpenRelTable::closeRel(int relId)
{
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
        return E_NOTPERMITTED;

    if (relId < 0 || relId >= MAX_OPEN)
        return E_OUTOFBOUND;

    if (OpenRelTable::tableMetaInfo[relId].free)
        return E_RELNOTOPEN;

    // free the memory allocated in the relation and attribute caches which was
    // allocated in the OpenRelTable::openRel() function
    free(RelCacheTable::relCache[relId]);
    struct AttrCacheEntry *curr = AttrCacheTable::attrCache[relId];
    while (curr != nullptr)
    {
        struct AttrCacheEntry *nxt = curr->next;
        free(curr);
        curr = nxt;
    }

    // update `tableMetaInfo` to set `relId` as a free slot
    // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
    tableMetaInfo[relId].free = true;
    AttrCacheTable::attrCache[relId] = nullptr;
    RelCacheTable::relCache[relId] = nullptr;

    return SUCCESS;
}


OpenRelTable::~OpenRelTable()
{

    for (int i = 2; i < MAX_OPEN; i++)
    {
        if (!tableMetaInfo[i].free)
        {
            OpenRelTable::closeRel(i);
        }
    }
    for (int i = 0; i < MAX_OPEN; i++)
    {
        if (RelCacheTable::relCache[i] != nullptr)
        {
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr;
        }
    }

    for (int i = 0; i < MAX_OPEN; i++)
    {
        struct AttrCacheEntry *curr = AttrCacheTable::attrCache[i];
        while (curr != nullptr)
        {
            struct AttrCacheEntry *nxt = curr->next;
            free(curr);
            curr = nxt;
        }
        AttrCacheTable::attrCache[i] = nullptr;
    }
}
