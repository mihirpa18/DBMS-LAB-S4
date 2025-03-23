#include "BPlusTree.h"

#include <cstring>

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
    // index id which will store  search index of attrName
    IndexId searchIndex;
    AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    // load attrCat entry
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    int block, index;

    // if search is done for first time
    if (searchIndex.block == -1 && searchIndex.index == -1)
    {
        block = attrCatEntry.rootBlock;
        index = 0;
        // if attrName dont have B+ tree
        if (block == -1)
        {
            return RecId{-1, -1};
        }
    }
    else
    {
        // search resumed from next index
        block = searchIndex.block;
        index = searchIndex.index + 1;

        // load block into leaf
        IndLeaf leaf(block);
        // load the leafHead
        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries)
        {
            // all the entries in the current block has been searched , go to rblock
            block = leafHead.rblock;
            index = 0;
            // check if it is last block of relation
            if (block == -1)
            {
                return RecId{-1, -1};
            }
        }
    }

    /******  Traverse through all the internal nodes according to value
        of attrVal and the operator op                             ******/

    /* (This section is only needed when
        - search restarts from the root block (when searchIndex is reset by caller)
        - root is not a leaf
        If there was a valid search index, then we are already at a leaf block
        and the test condition in the following loop will fail)
    */

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL)
    {
        // load the block to internalBlk
        IndInternal internalBlk(block);

        // load the header
        HeadInfo intHead;
        internalBlk.getHeader(&intHead);

        // to store entry of internalBlk
        InternalEntry intEntry;

        if (op == NE || op == LE || op == LT)
        {
            /* if op=
                        - NE: need to search the entire linked list of leaf indices of the B+ Tree,
                        starting from the leftmost leaf index. Thus, always move to the left.

                        - LT and LE: the attribute values are arranged in ascending order in the
                        leaf indices of the B+ Tree. Values that satisfy these conditions, if
                        any exist, will always be found in the left-most leaf index. Thus,
                        always move to the left.
                        */

            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;
        }
        else
        {
            /*
        - EQ, GT and GE: move to the left child of the first entry that is
        greater than (or equal to) attrVal
        (we are trying to find the first entry that satisfies the condition.
        since the values are in ascending order we move to the left child which
        might contain more entries that satisfy the condition)
        */

            // traverse through all entries of internalBlk and find entry that satifies the condition
            int finder = -1;
            for (int i = 0; i < intHead.numEntries; i++)
            {
                internalBlk.getEntry(&intEntry, i);
                if ((op == GE || op == EQ))
                {

                    if (compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType) >= 0)
                    {
                        finder = i;
                        break;
                    }
                }
                else if (op == GT)
                {
                    if (compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType) > 0)
                    {
                        finder = i;
                        break;
                    }
                }
            }
            // if you found an entry
            if (finder != -1)
            {
                internalBlk.getEntry(&intEntry, finder);
                block = intEntry.lChild;
            }
            else
            {
                internalBlk.getEntry(&intEntry, intHead.numEntries - 1);
                block = intEntry.rChild;
            }
        }
    }

    // now block has block=block num of leafblock
    /******  Identify the first leaf index entry from the current position
                that satisfies our condition (moving right)             ******/

    while (block != -1)
    {
        // load block into leafBlk
        IndLeaf leafBlk(block);

        // get the header
        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);

        Index leafEntry;

        while (index < leafHead.numEntries)
        {
            // load entry corresponding to block and index
            leafBlk.getEntry(&leafEntry, index);

            /* comparison between leafEntry's attribute value
                            and input attrVal using compareAttrs()*/
            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);

            // if entry satisfying the condition found)
            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0))
            {
                // set search index
                searchIndex = {block, index};
                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);
                return RecId({leafEntry.block, leafEntry.slot});
            }
            /*future entries will not satisfy EQ, LE, LT since the values
                    are arranged in ascending order in the leaves */
            else if ((op == EQ || op == LE || op == LT) && cmpVal > 0)
            {
                return RecId({-1, -1});
            }
            // search next index
            ++index;
        }

        /*only for NE operation do we have to check the entire linked list;
        for all the other op it is guaranteed that the block being searched
        will have an entry, if it exists, satisying that op. */

        if (op != NE)
        {
            break;
        }
        block = leafHead.rblock;
        index = 0;
    }

    return RecId({-1, -1});
}

int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE])
{
    // if relId is either RELCAT_RELID or ATTRCAT_RELID:
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
    {
        return E_NOTPERMITTED;
    }

    // get the attrcatentry
    AttrCatEntry attrCatBuff;
    int attrCat = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuff);
    if (attrCat != SUCCESS)
    {
        return attrCat;
    }

    // if index already exists for attribute return success
    if (attrCatBuff.rootBlock != -1)
    {
        return SUCCESS;
    }

    /******Creating a new B+ Tree ******/

    // get a free leaf block using constuctor 1 to allocate new block
    IndLeaf rootBlockbuf;
    // get block num
    int rootBlock = rootBlockbuf.getBlockNum();

    // if there is no more free block to create index
    if (rootBlock == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    attrCatBuff.rootBlock=rootBlock;
    AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatBuff);

    // load the relation catalog entry into relCatEntry
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int block = relCatEntry.firstBlk;

    /***** Traverse all the blocks in the relation and insert them one
          by one into the B+ Tree *****/
    while (block != -1)
    {
        // declare recBuff
        RecBuffer recBuff(block);
        // load slotmap
        unsigned char slotMap[relCatEntry.numSlotsPerBlk];
        recBuff.getSlotMap(slotMap);

        for (int i = 0; i < relCatEntry.numSlotsPerBlk; i++)
        {
            if (slotMap[i] == SLOT_OCCUPIED)
            {
                Attribute record[relCatEntry.numAttrs];
                // load the record corresponding to slot
                recBuff.getRecord(record, i);

                RecId recId = {block, i};

                int retVal = BPlusTree::bPlusInsert(relId, attrName, record[attrCatBuff.offset], recId);
                if (retVal == E_DISKFULL)
                {
                    return E_DISKFULL;
                }
            }
        }
        HeadInfo head;
        recBuff.getHeader(&head);
        block = head.rblock;
    }

    return SUCCESS;
}

// Used to delete a B+ Tree rooted at a particular block passed as input to the method.
//  The method recursively deletes the constituent index blocks, both internal and leaf index blocks,
// until the full B+ Tree is deleted.
int BPlusTree::bPlusDestroy(int rootBlockNum)
{
    // if rootBlock lies outside valid range return error
    if (rootBlockNum < 0 || rootBlockNum >= DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

    // get the type of block
    int type = StaticBuffer::getStaticBlockType(rootBlockNum);

    // if type is leafBlock
    if (type == IND_LEAF)
    {
        // just release the block
        IndLeaf currLeafNode(rootBlockNum);
        currLeafNode.releaseBlock();

        return SUCCESS;
    }
    else if (type == IND_INTERNAL)
    {
        IndInternal currIntNode(rootBlockNum);
        HeadInfo header;
        currIntNode.getHeader(&header);

        // get the first entry
        InternalEntry tempEntry;
        currIntNode.getEntry(&tempEntry, 0);
        // destroy left child
        if (tempEntry.lChild != -1)
        {
            BPlusTree::bPlusDestroy(tempEntry.lChild);
        }

        // destroy the rchild of rest entris
        for (int i = 0; i < header.numEntries; i++)
        {
            currIntNode.getEntry(&tempEntry, i);
            if (tempEntry.rChild != -1)
            {
                BPlusTree::bPlusDestroy(tempEntry.rChild);
            }
        }
        // release the block
        currIntNode.releaseBlock();

        return SUCCESS;
    }
    else
    {
        // block is not an index block
        return E_INVALIDBLOCK;
    }
}

int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recId)
{
    // get attrcat entry
    AttrCatEntry attrCatBuff;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuff);
    if (ret != SUCCESS)
    {
        return ret;
    }

    // get root block num using attrcat entry
    int blockNum = attrCatBuff.rootBlock;
    // no index
    if (blockNum == -1)
    {
        return E_NOINDEX;
    }

    // find the leaf block to which insertion is to be done using the
    // findLeafToInsert() function
    int leafBlkNum = BPlusTree::findLeafToInsert(blockNum, attrVal, attrCatBuff.attrType);

    // insert the attrVal and recId to the leaf block at blockNum using the
    // insertIntoLeaf() function.
    Index entry;
    entry.attrVal = attrVal, entry.block = recId.block, entry.slot = recId.slot;
    ret = BPlusTree::insertIntoLeaf(relId, attrName, leafBlkNum, entry);
    // NOTE: the insertIntoLeaf() function will propagate the insertion to the
    //       required internal nodes by calling the required helper functions
    //       like insertIntoInternal() or createNewRoot()

    // if insertion fails by diskfull than destroy tree and set rootBlock in attrCat
    if (ret == E_DISKFULL)
    {
        BPlusTree::bPlusDestroy(blockNum);
        attrCatBuff.rootBlock = -1;
        AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatBuff);

        return E_DISKFULL;
    }
    return SUCCESS;
}

// Used to find the leaf index block to which an attribute would be inserted to in the B+ insertion process.
//  If this leaf turns out to be full, the caller will need to handle the splitting of this block to insert the entry.
int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType)
{
    int blockNum = rootBlock;
    while (StaticBuffer::getStaticBlockType(blockNum) != IND_LEAF)
    {
        IndInternal IndInt(blockNum);
        HeadInfo header;
        IndInt.getHeader(&header);

        InternalEntry intEntry;
        int finder = -1;
        /* iterate through all the entries, to find the first entry whose
           attribute value >= value to be inserted.*/
        for (int i = 0; i < header.numEntries; i++)
        {
            IndInt.getEntry(&intEntry, i);
            if (compareAttrs(intEntry.attrVal, attrVal, attrType) > 0)
            {
                finder = i;
                break;
            }
        }
        // if no such entry is found ,set block num to rchild of rightmost entry
        if (finder == -1)
        {
            IndInt.getEntry(&intEntry, header.numEntries - 1);
            blockNum = intEntry.rChild;
        }
        // if entry is found ,set block num to lchild of that entry
        else
        {
            IndInt.getEntry(&intEntry, finder);
            blockNum = intEntry.lChild;
        }
    }

    return blockNum;
}

// Used to insert an index entry into a leaf index block of an existing B+ tree.
// If the leaf is full and requires splitting, this function will call other B+ Tree Layer
// functions to handle any updation required to the parent internal index blocks of the B+ tree
int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry)
{
    // get attrcat entry
    AttrCatEntry attrCatBuff;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuff);

    // declare indLeaf Instance
    IndLeaf leafBlock(blockNum);
    HeadInfo blockHeader;
    leafBlock.getHeader(&blockHeader);

    // the following variable will be used to store a list of index entries with
    // existing indices + the new index to insert
    Index indices[blockHeader.numEntries + 1];

    /*
   Iterate through all the entries in the block and copy them to the array indices.
   Also insert `indexEntry` at appropriate position in the indices array maintaining
   the ascending order.
   */
    Index tempEntry;
    int tar_ind = -1;
    for (int i = 0; i < blockHeader.numEntries; i++)
    {
        leafBlock.getEntry(&tempEntry, i);
        if (compareAttrs(tempEntry.attrVal, indexEntry.attrVal, attrCatBuff.attrType) > 0)
        {
            tar_ind = i;
            break;
        }
    }
    if (tar_ind == -1)
    {
        tar_ind = blockHeader.numEntries;
    }
    for (int i = 0; i < tar_ind; i++)
    {
        leafBlock.getEntry(&indices[i], i);
    }

    indices[tar_ind] = indexEntry;

    for (int i = tar_ind; i < blockHeader.numEntries; i++)
    {
        leafBlock.getEntry(&indices[i + 1], i);
    }

    // if leaf block has not reached its max limit
    if (blockHeader.numEntries != MAX_KEYS_LEAF)
    {
        // set block header
        blockHeader.numEntries += 1;
        leafBlock.setHeader(&blockHeader);

        // set entries of block using indices
        for (int i = 0; i < blockHeader.numEntries; i++)
        {
            leafBlock.setEntry(&indices[i], i);
        }

        return SUCCESS;
    }

    // we reached max limit
    //  If we reached here, the `indices` array has more than entries than can fit
    //  in a single leaf index block. Therefore, we will need to split the entries
    //  in `indices` between two leaf blocks. We do this using the splitLeaf() function.
    //  This function will return the blockNum of the newly allocated block or
    //  E_DISKFULL if there are no more blocks to be allocated.

    int newRightBlk = BPlusTree::splitLeaf(blockNum, indices);
    if (newRightBlk == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    // if current block is not root blk
    if (blockHeader.pblock != -1)
    {
        InternalEntry entry;
        entry.attrVal = indices[MIDDLE_INDEX_LEAF].attrVal;
        entry.lChild = blockNum;
        entry.rChild = newRightBlk;
        return BPlusTree::insertIntoInternal(relId, attrName, blockHeader.pblock, entry);
    }
    else
    {
        // the current block was the root block and is now split. a new internal index
        // block needs to be allocated and made the root of the tree.
        return BPlusTree::createNewRoot(relId, attrName, indices[MIDDLE_INDEX_LEAF].attrVal, blockNum, newRightBlk);
    }

    return SUCCESS;
}

// Distributes an array of index entries between an existing leaf index block and a newly allocated leaf index block.
int BPlusTree::splitLeaf(int leafBlockNum, Index indices[])
{
    // declare rightBlk, (new block to be allocated)
    IndLeaf rightBlk;
    // get the block num
    int rightBlkNum = rightBlk.getBlockNum();
    // declare leftblk,current block
    IndLeaf leftBlk(leafBlockNum);
    int leftBlkNum = leafBlockNum;

    // if failed to obtain a new leaf index block because the disk is full
    if (rightBlkNum == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    // get the header of both blocks
    HeadInfo leftBlkHeader, rightBlkHeader;
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);

    // set the headers of right block(newly created one)
    rightBlkHeader.numEntries = (MAX_KEYS_LEAF + 1) / 2;
    rightBlkHeader.lblock = leftBlkNum;
    rightBlkHeader.rblock = leftBlkHeader.rblock;
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlk.setHeader(&rightBlkHeader);

    // set the header of left one
    leftBlkHeader.numEntries = (MAX_KEYS_LEAF + 1) / 2;
    leftBlkHeader.rblock = rightBlkNum;
    leftBlk.setHeader(&leftBlkHeader);

    // set the first 32 entries of leftBlk = the first 32 entries of indices array
    // and set the first 32 entries of newRightBlk = the next 32 entries of indices
    int half = (MAX_KEYS_LEAF + 1) / 2;
    for (int i = 0; i < half; i++)
    {
        leftBlk.setEntry(&indices[i], i);
    }
    for (int i = 0; i < half; i++)
    {
        rightBlk.setEntry(&indices[i + half], i);
    }

    return rightBlkNum;
}

// Used to insert an index entry into an internal index block of an existing B+ tree.
//  This function will call itself to handle any updation required to it's parent internal index blocks.
int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intlBlockNum, InternalEntry intEntry)
{
    AttrCatEntry attrCatBuff;
    int  ret=AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuff);
    if (ret != SUCCESS)
        return ret;

    // declare intBlk
    IndInternal intBlock(intlBlockNum);
    // get the header of block
    HeadInfo blockHeader;
    intBlock.getHeader(&blockHeader);

    // declare internalEntries to store all existing entries + the new entry
    InternalEntry internalEntries[blockHeader.numEntries + 1];

    /*
   Iterate through all the entries in the block and copy them to the array
   `internalEntries`. Insert `indexEntry` at appropriate position in the
   array maintaining the ascending order.
       - use IndInternal::getEntry() to get the entry
       - use compareAttrs() to compare two structs of type Attribute

   Update the lChild of the internalEntry immediately following the newly added
   entry to the rChild of the newly added entry.
   */
    InternalEntry tempEntry;
    int tar_ind = -1;
    for (int i = 0; i < blockHeader.numEntries; i++)
    {
        intBlock.getEntry(&tempEntry, i);
        if (compareAttrs(tempEntry.attrVal, intEntry.attrVal, attrCatBuff.attrType) > 0)
        {
            tar_ind = i;
            break;
        }
    }
    if (tar_ind == -1)
    {
        tar_ind = blockHeader.numEntries;
    }
    for (int i = 0; i < tar_ind; i++)
    {
        intBlock.getEntry(&internalEntries[i], i);
    }

    internalEntries[tar_ind] = intEntry;

     for (int i = tar_ind; i < blockHeader.numEntries; i++)
    {
        intBlock.getEntry(&internalEntries[i + 1], i);
    }

    if (tar_ind < blockHeader.numEntries)
    {
        internalEntries[tar_ind + 1].lChild = internalEntries[tar_ind].rChild;
    }

   

    if (blockHeader.numEntries != MAX_KEYS_INTERNAL)
    {
        // set the header
        blockHeader.numEntries += 1;
        intBlock.setHeader(&blockHeader);

        for (int i = 0; i < blockHeader.numEntries; i++)
        {
            intBlock.setEntry(&internalEntries[i], i);
        }

        return SUCCESS;
    }

    // If we reached here, the `internalEntries` array has more than entries than
    // can fit in a single internal index block. Therefore, we will need to split
    // the entries in `internalEntries` between two internal index blocks. We do
    // this using the splitInternal() function.
    // This function will return the blockNum of the newly allocated block or
    // E_DISKFULL if there are no more blocks to be allocated.
    int newRightBlk = BPlusTree::splitInternal(intlBlockNum, internalEntries);
    if (newRightBlk == E_DISKFULL)
    {
        BPlusTree::bPlusDestroy(intEntry.rChild);

        return E_DISKFULL;
    }

    if (blockHeader.pblock != -1)
    {
        InternalEntry entry;
        entry.attrVal = internalEntries[MIDDLE_INDEX_INTERNAL].attrVal, entry.lChild = intlBlockNum, entry.rChild = newRightBlk;
        return BPlusTree::insertIntoInternal(relId, attrName, blockHeader.pblock, entry);
    }
    else
    {
        return BPlusTree::createNewRoot(relId, attrName, internalEntries[MIDDLE_INDEX_INTERNAL].attrVal, intlBlockNum, newRightBlk);
    }

    return SUCCESS;
}

// Distributes an array of index entries between an existing internal index block and a newly allocated internal index block.
int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[])
{
    IndInternal rightBlk;
    IndInternal leftBlk(intBlockNum);

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = intBlockNum;

    if (rightBlkNum == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    // GET THE HEADER
    HeadInfo leftBlkHeader, rightBlkHeader;
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);

    // set the rightblk header
    rightBlkHeader.numEntries = (MAX_KEYS_INTERNAL) / 2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlk.setHeader(&rightBlkHeader);

    // set the left blk header
    leftBlkHeader.numEntries = MAX_KEYS_INTERNAL / 2;
    leftBlk.setHeader(&leftBlkHeader);

    int half = MAX_KEYS_INTERNAL / 2;
    for (int i = 0; i < half; i++)
    {
        leftBlk.setEntry(&internalEntries[i], i);
    }
    for (int i = 0; i < half; i++)
    {
        rightBlk.setEntry(&internalEntries[i + half + 1], i);
    }

    /* block type of a child of any entry of the internalEntries array */
    int type = StaticBuffer::getStaticBlockType(internalEntries[0].lChild);

    InternalEntry entryBuffer;
    rightBlk.getEntry(&entryBuffer, 0);
    BlockBuffer firstChildBlk(entryBuffer.lChild);
    HeadInfo firstHead;
    firstChildBlk.getHeader(&firstHead);
    firstHead.pblock = rightBlkNum;
    firstChildBlk.setHeader(&firstHead);

    for (int i = 0; i < rightBlkHeader.numEntries; i++)
    {
         rightBlk.getEntry(&entryBuffer, i);
        BlockBuffer childBlk(entryBuffer.rChild);
        HeadInfo childHead;
        childBlk.getHeader(&childHead);
        childHead.pblock = rightBlkNum;
        childBlk.setHeader(&childHead);
    }

    return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild)
{
    AttrCatEntry attrCatBuff;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuff);

    // declare newRootBlk, an instance of IndInternal using appropriate constructor
    // to allocate a new internal index block on the disk
    IndInternal newRootBlk;
    // get the block num of new root blk
    int newRootBlkNum = newRootBlk.getBlockNum();

    if (newRootBlkNum == E_DISKFULL)
    {
        // Using bPlusDestroy(), destroy the right subtree, rooted at rChild.
        // This corresponds to the tree built up till now that has not yet been
        // connected to the existing B+ Tree
        BPlusTree::bPlusDestroy(rChild);
        return E_DISKFULL;
    }

    HeadInfo rootHeader;
    newRootBlk.getHeader(&rootHeader);
    rootHeader.numEntries = 1;
    newRootBlk.setHeader(&rootHeader);

    // create a struct InternalEntry with lChild, attrVal and rChild from the
    // arguments and set it as the first entry in newRootBlk using IndInternal::setEntry()
    InternalEntry entry;
    entry.attrVal = attrVal, entry.lChild = lChild, entry.rChild = rChild;
    newRootBlk.setEntry(&entry, 0);

    // declare BlockBuffer instances for the `lChild` and `rChild` blocks using
    // appropriate constructor and update the pblock of those blocks to `newRootBlkNum`
    // using BlockBuffer::getHeader() and BlockBuffer::setHeader()
    BlockBuffer lchildBlk(lChild);
    HeadInfo lChildHead;
    lchildBlk.getHeader(&lChildHead);
    lChildHead.pblock = newRootBlkNum;
    lchildBlk.setHeader(&lChildHead);

    BlockBuffer rChildBlk(rChild);
    HeadInfo rChildHead;
    rChildBlk.getHeader(&rChildHead);
    rChildHead.pblock = newRootBlkNum;
    rChildBlk.setHeader(&rChildHead);

    // update rootBlock = newRootBlkNum for the entry corresponding to `attrName`
    // in the attribute cache using AttrCacheTable::setAttrCatEntry().
    attrCatBuff.rootBlock = newRootBlkNum;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatBuff);

    return SUCCESS;
}
