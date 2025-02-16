#include "BlockAccess.h"
#include <iostream>
#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrval, int op)
{
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;

    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)

        RelCatEntry relcatBuff;
        RelCacheTable::getRelCatEntry(relId, &relcatBuff);

        // get the first record block of the relation from the relation cache
        block = relcatBuff.firstBlk;
        slot = 0;
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    /* The following code searches for the next record in the relation
           that satisfies the given condition
           We start from the record id (block, slot) and iterate over the remaining
           records of the relation
        */
    while (block != -1)
    {
        RecBuffer recBuff(block);
        HeadInfo head;
        recBuff.getHeader(&head);  

        int recSize = head.numAttrs;
        Attribute rec[recSize];

        recBuff.getRecord(rec, slot);
        

        int slotMapSize = head.numSlots;
        unsigned char slotMap[slotMapSize];
        recBuff.getSlotMap(slotMap);

        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if (slot >= slotMapSize)
        {
            block = head.rblock;
            slot = 0;
            continue; // continue to the beginning of this while loop
        }
        // if slot is free skip the loop
        if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
            continue;
        }

         // compare record's attribute value to the the given attrVal as below
        AttrCatEntry attrCatBuff;

        AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuff);

        Attribute currRec[recSize];
        recBuff.getRecord(currRec, slot);
        /* use the attribute offset to get the value of the attribute from
        current record */
        int offset = attrCatBuff.offset;

        int cmpVal = compareAttrs(currRec[offset], attrval, attrCatBuff.attrType);

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) || // if op is "not equal to"
            (op == LT && cmpVal < 0) ||  // if op is "less than"
            (op == LE && cmpVal <= 0) || // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) || // if op is "equal to"
            (op == GT && cmpVal > 0) ||  // if op is "greater than"
            (op == GE && cmpVal >= 0)    // if op is "greater than or equal to"
        )
        {
            
            // set the search index in the relation cache as
            // the record id of the record that satisfies the given condition
            RecId searchINd = {block, slot};
            RelCacheTable::setSearchIndex(relId, &searchINd);

            return RecId{block, slot};
        }
        slot++;
    }
    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}