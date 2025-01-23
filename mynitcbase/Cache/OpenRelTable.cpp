#include "OpenRelTable.h"
#include <cstring>
#include <iostream>

OpenRelTable::OpenRelTable()
{

    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; ++i)
    {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
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

    /**** setting up student  relation in the Relation Cache Table ****/

    relCatBlock.getRecord(relCatRecord, 2);
    struct RelCacheEntry student;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &student.relCatEntry);
    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
    attrCacheEntry.recId.slot = 2;

    RelCacheTable::relCache[2] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[2]) = student;

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

    head = nullptr;
    curr = nullptr;
    for (int i = RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS; i < RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS + 4; i++)
    {
        attrCatBlock.getRecord(attrCatRecord, i);

        char name[] = "Students";

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
    AttrCacheTable::attrCache[2] = head;
}

OpenRelTable::~OpenRelTable()
{
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

int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{
    if (strcmp(relName, RELCAT_RELNAME) == 0)
        return RELCAT_RELID;
    if (strcmp(relName, ATTRCAT_RELNAME) == 0)
        return ATTRCAT_RELID;

    return E_RELNOTOPEN;
}