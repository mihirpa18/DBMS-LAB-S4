#include "BlockAccess.h"
#include <iostream>
#include <cstring>

RecId BlockAccess::linearSearch(int relId,char attrName[ATTR_SIZE],union Attribute attrval,int op){
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);
    
    int block,slot;

    if(prevRecId.block==-1 && prevRecId.slot==-1){
        RelCatEntry relcatBuff;
        RelCacheTable::getRelCatEntry(relId,&relcatBuff);

        block=relcatBuff.firstBlk;
        slot=0;
    }
    else{
        block=prevRecId.block;
        slot=prevRecId.slot+1;
    }

    while(block!=-1){
        RecBuffer recBuff(block);
        HeadInfo head;

        int recSize=head.numAttrs;
        Attribute rec[recSize];

        recBuff.getRecord(rec,slot);
        recBuff.getHeader(&head);

        int slotMapSize=head.numSlots;
        unsigned char slotMap[slotMapSize];
        recBuff.getSlotMap(slotMap);

        if(slot>slotMapSize){
            block=head.rblock;
            slot=0;
            continue;
        }
        if(slotMap[slot]==SLOT_UNOCCUPIED){
            slot++;
            continue;
        }

        AttrCatEntry attrCatBuff;
        
        AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatBuff);

        Attribute currRec[recSize];
        recBuff.getRecord(currRec,slot);
        int offset=attrCatBuff.offset;

        int cmpVal=compareAttrs(currRec[offset],attrval,attrCatBuff.attrType);

        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            RecId searchINd={block,slot};
            RelCacheTable::setSearchIndex(relId,&searchINd);

            return RecId{block,slot};
        }

        slot++;
    }

    return RecId{-1,-1};
}