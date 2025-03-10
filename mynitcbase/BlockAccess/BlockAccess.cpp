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

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;
    memcpy(newRelationName.sVal, newName, ATTR_SIZE);

    RecId id = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, newRelationName, EQ);

    if (id.block != -1 || id.slot != -1)
        return E_RELEXIST;

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute oldRelationName;
    memcpy(oldRelationName.sVal, oldName, ATTR_SIZE);

    id = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, oldRelationName, EQ);

    if (id.block == -1 && id.slot == -1)
        return E_RELNOTEXIST;

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */

    RecBuffer relcatBuff(id.block);
    Attribute relrecord[RELCAT_NO_ATTRS];
    relcatBuff.getRecord(relrecord, id.slot);

    /* update the relation name attribute in the record with newName.
         (use RELCAT_REL_NAME_INDEX) */

    memcpy(&relrecord[RELCAT_REL_NAME_INDEX], &newRelationName, ATTR_SIZE);
    relcatBuff.setRecord(relrecord, id.slot);

    // attribute cat
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for (int i = 0; i < relrecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal; i++)
    {
        id = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);

        RecBuffer attrcatBuff(id.block);
        Attribute attrRecord[ATTRCAT_NO_ATTRS];
        attrcatBuff.getRecord(attrRecord, id.slot);
        memcpy(&attrRecord[ATTRCAT_REL_NAME_INDEX], &newRelationName, ATTR_SIZE);
        attrcatBuff.setRecord(attrRecord, id.slot);
    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;
    memcpy(relNameAttr.sVal, relName, ATTR_SIZE);

    // Search for the relation with name relName in relation catalog using linearSearch()
    RecId id = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    if (id.block == -1 && id.slot == -1)
        return E_RELNOTEXIST;

    // attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    /* iterate over all Attribute Catalog Entry record corresponding to the
           relation to find the required attribute */
    while (true)
    {

        RecId attrId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        if (attrId.block == -1 && attrId.slot == -1)
            break;

        RecBuffer attrCatBuff(attrId.block);
        attrCatBuff.getRecord(attrCatEntryRecord, attrId.slot);

        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0)
        {
            attrToRenameRecId.block = attrId.block;
            attrToRenameRecId.slot = attrId.slot;
        }
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
            return E_ATTREXIST;
    }

    if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1)
        return E_ATTRNOTEXIST;

    RecBuffer renameBuff(attrToRenameRecId.block);
    Attribute renameRec[ATTRCAT_NO_ATTRS];
    renameBuff.getRecord(renameRec, attrToRenameRecId.slot);

    memcpy(renameRec[ATTRCAT_ATTR_NAME_INDEX].sVal, newName, ATTR_SIZE);
    renameBuff.setRecord(renameRec, attrToRenameRecId.slot);

    return SUCCESS;
}


int BlockAccess::insert(int relId, Attribute *record)
{
    // get relcat entries
    RelCatEntry relcatBuff;
    RelCacheTable::getRelCatEntry(relId, &relcatBuff);

    // block num is first block of rel
    int blockNum = relcatBuff.firstBlk;
    // rec id is used to store where new records will be inserted
    RecId rec_id = {-1, -1};
    int numOfSlots = relcatBuff.numSlotsPerBlk;
    int numOfAttr = relcatBuff.numAttrs;
    int prevBlock = -1;

    /*
            Traversing the linked list of existing record blocks of the relation
            until a free slot is found OR
            until the end of the list is reached
        */
    while (blockNum != -1)
    {
        RecBuffer buffer(blockNum);

        HeadInfo head;
        buffer.getHeader(&head);

        unsigned char slotMap[numOfSlots];
        buffer.getSlotMap(slotMap);

        int freeSlot = -1;
        for (int i = 0; i < numOfSlots; i++)
        {
            if (slotMap[i] == SLOT_UNOCCUPIED)
            {
                freeSlot = i;
                rec_id.block = blockNum;
                rec_id.slot = i;
                break;
            }
        }

        if (freeSlot == -1)
        {
            prevBlock = blockNum;
            blockNum = head.rblock;
        }
        //no free slot in all blocks 
        else
        {
            break;
        }
    }

    //  if no free slot is found in existing record blocks (rec_id = {-1, -1})
    if (rec_id.block == -1 && rec_id.slot == -1)
    {
        // if relation is RELCAT, do not allocate any more blocks
        if (relId == RELCAT_RELID)
            return E_MAXRELATIONS;

        // get new record
        RecBuffer newBlock;
        int blockNum = newBlock.getBlockNum();
        if (blockNum == E_DISKFULL)
            return E_DISKFULL;

        rec_id.block = blockNum;
        rec_id.slot = 0;

        // set the header of the new record block such that it links with
        //    existing record blocks of the relation
        HeadInfo nHead;
        
        nHead.blockType = REC;
        nHead.pblock = -1;
        nHead.lblock = (prevBlock != -1) ? prevBlock : -1;
        nHead.rblock = -1;
        nHead.numEntries = 0;
        nHead.numAttrs = numOfAttr;
        nHead.numSlots = numOfSlots;
        newBlock.setHeader(&nHead);

        // set block's slot map with all slots marked as free
        unsigned char nSlotMap[numOfSlots];
        
        for (int i = 0; i < numOfSlots; i++)
        {
            nSlotMap[i] = SLOT_UNOCCUPIED;
        }
        newBlock.setSlotMap(nSlotMap);

        //if block is not first block // update the rblock field of the header to the new block
            // number
        if (prevBlock != -1)
        {
            RecBuffer prevBuff(prevBlock);

            HeadInfo prevHead;
            prevBuff.getHeader(&prevHead);

            prevHead.rblock = blockNum;
            prevBuff.setHeader(&prevHead);
        }
        else
        {
            // update first block field in the relation catalog entry to the
            // new block
            relcatBuff.firstBlk = rec_id.block;
            RelCacheTable::setRelCatEntry(relId, &relcatBuff);
        }
         // update last block field in the relation catalog entry to the
        // new block
        relcatBuff.lastBlk = rec_id.block;
        RelCacheTable::setRelCatEntry(relId, &relcatBuff);
    }

    //insert the record into recid'th slot using setrecord
    RecBuffer blockbuff(rec_id.block);
    int ret=blockbuff.setRecord(record, rec_id.slot);
    if(ret!=SUCCESS) return ret;

    //set the slotmap by marking entry into which record is inserted  as occupied
    unsigned char blockSlotMap[numOfSlots];
    blockbuff.getSlotMap(blockSlotMap);
    blockSlotMap[rec_id.slot] = SLOT_OCCUPIED;
    blockbuff.setSlotMap(blockSlotMap);

    //increment numentries in header
    HeadInfo blockHead;
    blockbuff.getHeader(&blockHead);
    blockHead.numEntries += 1;
    blockbuff.setHeader(&blockHead);

    //increment numrecs in relcache entry
    relcatBuff.numRecs += 1;
    RelCacheTable::setRelCatEntry(relId, &relcatBuff);

    return SUCCESS;
}



int BlockAccess::search(int relId, Attribute record[], char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
    RecId recId;
    recId = BlockAccess::linearSearch(relId, attrName, attrVal, op);

    if (recId.block == -1 || recId.slot == -1)
        return E_NOTFOUND;

    RecBuffer recBuff(recId.block);
    recBuff.getRecord(record, recId.slot);

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE])
{

    // make sure rel is not relcat or attrcat
    if (strcmp(RELCAT_RELNAME, relName) == 0 || strcmp(ATTRCAT_RELNAME, relName) == 0)
    {
        return E_NOTPERMITTED;
    }

    // search on relcat
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr; // stores relName as type attribute
    strcpy(relNameAttr.sVal, relName);

    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, EQ);

    if (recId.block == -1 || recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    // store the relcat record
    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    RecBuffer recBuff(recId.block);
    recBuff.getRecord(relCatEntryRecord, recId.slot);

    // to get first block and num of attributes in the relation
    int firstBlk = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    // delete all records of the block
    int currblk = firstBlk;

    while (currblk != -1)
    {
        RecBuffer currRecBuff(currblk);
        HeadInfo head;
        currRecBuff.getHeader(&head);
        int nxtblk = head.rblock;
        currRecBuff.releaseBlock();

        currblk = nxtblk;
    }

    // deleting attr cat entries

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributesDeleted = 0;

    while (true)
    {
        RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        // if no more attribute -> break
        if (attrCatRecId.block == -1 || attrCatRecId.slot == -1)
        {
            break;
        }
        numberOfAttributesDeleted++;

        Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];
        RecBuffer attrCatBuff(attrCatRecId.block);
        // getting header
        HeadInfo attrCatHead;
        attrCatBuff.getHeader(&attrCatHead);
        // getting record
        attrCatBuff.getRecord(attrCatEntryRecord, attrCatRecId.slot);

        // get the rootblock
        int rootBlock = attrCatEntryRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;

        // setting slotMap
        unsigned char slotMap[attrCatHead.numSlots];
        attrCatBuff.getSlotMap(slotMap);
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        attrCatBuff.setSlotMap(slotMap);

        // decrementing numofentries
        attrCatHead.numEntries -= 1;
        attrCatBuff.setHeader(&attrCatHead);

        // if numentries becomes 0 release block is called
        if (attrCatHead.numEntries == 0)
        {
            int lblock = attrCatHead.lblock;
            int rblock = attrCatHead.rblock;

            // Standard Linked List Delete for a Block
            RecBuffer lBlock(lblock);
            HeadInfo lHead;
            lBlock.getHeader(&lHead);
            lHead.rblock = rblock;
            lBlock.setHeader(&lHead);

            if (rblock != -1)
            {
                RecBuffer rBlock(rblock);
                HeadInfo rHead;
                rBlock.getHeader(&rHead);
                rHead.lblock = lblock;
                rBlock.setHeader(&rHead);
            }
            // if block is last block
            else
            {
                RelCatEntry relCatEnt;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEnt);
                relCatEnt.lastBlk = lblock;
                RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEnt);
            }

            attrCatBuff.releaseBlock();
        }
    }

    // delete the entries corresponding to relation from relation cat
    // get header
    HeadInfo relCatHead;
    recBuff.getHeader(&relCatHead);
    relCatHead.numEntries -= 1;
    recBuff.setHeader(&relCatHead);

    // slotMap
    unsigned char relCatSlotMap[relCatHead.numSlots];
    recBuff.getSlotMap(relCatSlotMap);
    relCatSlotMap[recId.slot] = SLOT_UNOCCUPIED;
    recBuff.setSlotMap(relCatSlotMap);

    // update relcache table
    RelCatEntry relCatEnt;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEnt);
    relCatEnt.numRecs -= 1;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEnt);

    // Update attribute catalog entry
    RelCatEntry attrCatEnt;
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatEnt);
    attrCatEnt.numRecs -= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatEnt);

    return SUCCESS;
}

//This function is used to fetch one record of the relation. 
//Each subsequent call would return the next record until there are no more records to be returned.
// It also updates searchIndex in the cache.
int BlockAccess::project(int relId,Attribute *record){
    
    //get prev search index of the relation from the relcache
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);
    int block,slot;

    //if searchInd is {-1,-1}=> new project operation, start from beginging
    if(prevRecId.block==-1 && prevRecId.slot==-1){
        //get the first record block
        RelCatEntry relCatBuff;
        RelCacheTable::getRelCatEntry(relId,&relCatBuff);
        block=relCatBuff.firstBlk;
        slot=0;
    }
    else{ //project operation is already in progress
        block=prevRecId.block;
        slot=prevRecId.slot+1;
    }

    //finds the next record
    while(block!=-1){
        RecBuffer recBuff(block);
        HeadInfo head;
        recBuff.getHeader(&head);
        unsigned char slotMap[head.numSlots];
        recBuff.getSlotMap(slotMap);

        if(slot>=head.numSlots){
            block=head.rblock;
            slot=0;

        }
        else if(slotMap[slot]==SLOT_UNOCCUPIED){
            slot++;
        }
        else{
            break;
        }
    }

    // if no record was found
    if(block==-1){
        return E_NOTFOUND;
    }
    RecId nextRecId{block,slot};
    RelCacheTable::setSearchIndex(relId,&nextRecId);

    //copy record
    RecBuffer nextBuff(block);
    nextBuff.getRecord(record,slot);

    return SUCCESS;

}