#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType)
{
    double diff;

    if (attrType == STRING)
    {
        diff = strcmp(attr1.sVal, attr2.sVal);
    }
    else
    {
        diff = attr1.nVal - attr2.nVal;
    }

    if (diff > 0)
        return 1;
    else if (diff < 0)
        return -1;
    else
        return 0;
}

// constructor 1 (whenever new block is allocated in disk)
BlockBuffer::BlockBuffer(char blockType)
{
    int intBlockType;
    if (blockType == 'R')
    {
        intBlockType = REC;
    }
    else if (blockType == 'I')
    {
        intBlockType = IND_INTERNAL;
    }
    else if (blockType == 'L')
    {
        intBlockType = IND_LEAF;
    }
    else
    {
        intBlockType = UNUSED_BLK;
    }

    // allocate a block in disk and buffer
    int blocknum = BlockBuffer::getFreeBlock(intBlockType);

    // set blocknum field of object with allocated block

    if (blocknum < 0 || blocknum >= DISK_BLOCKS)
    {
        this->blockNum = blocknum;
        return;
    }
    this->blockNum = blocknum;
}

// constructor 2
BlockBuffer::BlockBuffer(int blockNum)
{
    // initialise this.blockNum with the argument
    this->blockNum = blockNum;
}

// Constructor 1 - called if new record block is allocated in disk
RecBuffer::RecBuffer() : BlockBuffer('R')
{
}

// Constructor 2- calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum)
{
}

/*
Used to get the header of the block into the location pointed to by `head`
NOTE: this function expects the caller to allocate memory for `head`
*/
int BlockBuffer::getHeader(struct HeadInfo *head)
{

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret; // return any errors that might have occured in the process
    }

    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;
    head->pblock = bufferHeader->pblock;
    head->lblock = bufferHeader->lblock;
    head->rblock = bufferHeader->rblock;
    head->numEntries = bufferHeader->numEntries;
    head->numAttrs = bufferHeader->numAttrs;
    head->numSlots = bufferHeader->numSlots;
    head->blockType=bufferHeader->blockType;

    return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head)
{
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;
    bufferHeader->pblock = head->pblock;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numSlots = head->numSlots;
    bufferHeader->blockType=head->blockType;

    int dirty = StaticBuffer::setDirtyBit(this->blockNum);
    if (dirty != SUCCESS)
        return dirty;

    return SUCCESS;
}

/*
Used to get the record at slot `slotNum` into the array `rec`
NOTE: this function expects the caller to allocate memory for `rec`
*/
int RecBuffer::getRecord(union Attribute *rec, int slotNum)
{
    struct HeadInfo head;
    this->getHeader(&head);

    int numAttr = head.numAttrs;
    int slotCount = head.numSlots;

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    int recSize = numAttr * ATTR_SIZE;
    unsigned char *slotPointer = bufferPtr + HEADER_SIZE + slotCount + (recSize * slotNum);

    memcpy(rec, slotPointer, recSize);

    return SUCCESS;
}

// set the recored
int RecBuffer::setRecord(union Attribute *rec, int slotNum)
{
    unsigned char *bufferptr;
    int ret = loadBlockAndGetBufferPtr(&bufferptr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    HeadInfo head;
    this->getHeader(&head);

    int numAttr = head.numAttrs;
    int slots = head.numSlots;
    int recSize = ATTR_SIZE * numAttr;
    if (slotNum < 0 || slotNum >= slots)
        return E_OUTOFBOUND;

    unsigned char *offset = bufferptr + HEADER_SIZE + slots + (slotNum * recSize);

    memcpy(offset, rec, recSize);
    StaticBuffer::setDirtyBit(this->blockNum);

    return SUCCESS;
}

/*
Used to load a block to the buffer and get a pointer to it.
NOTE: this function expects the caller to allocate memory for the argument
*/
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr)
{
    // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if (bufferNum == E_BLOCKNOTINBUFFER)
    {
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        if (bufferNum == E_OUTOFBOUND)
        {
            return E_OUTOFBOUND;
        }

        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    }
    else if (bufferNum != E_BLOCKNOTINBUFFER)
    {

        for (int i = 0; i < BUFFER_CAPACITY; i++)
        {
            if (!StaticBuffer::metainfo[i].free)
            {
                StaticBuffer::metainfo[i].timeStamp++;
            }
        }
        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    }

    // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
    *buffPtr = StaticBuffer::blocks[bufferNum];

    return SUCCESS;
}

int RecBuffer::getSlotMap(unsigned char *SlotMap)
{
    unsigned char *bufferptr;

    int ret = loadBlockAndGetBufferPtr(&bufferptr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    struct HeadInfo head;
    getHeader(&head);

    int slotCount = head.numSlots;

    unsigned char *SlotMapInBuffer = bufferptr + HEADER_SIZE;

    memcpy(SlotMap, SlotMapInBuffer, slotCount);

    return SUCCESS;
}

int RecBuffer::setSlotMap(unsigned char *slotMap)
{
    unsigned char *buffptr;
    int ret = BlockBuffer::loadBlockAndGetBufferPtr(&buffptr);
    if (ret != SUCCESS)
        return ret;

    struct HeadInfo head;
    getHeader(&head);
    int numSlots = head.numSlots;

    unsigned char *slotMapBuff = buffptr + HEADER_SIZE;
    memcpy(slotMapBuff, slotMap, numSlots);

    int dirty = StaticBuffer::setDirtyBit(this->blockNum);

    if (dirty != SUCCESS)
        return dirty;

    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType)
{
    unsigned char *bufferptr;
    int ret = loadBlockAndGetBufferPtr(&bufferptr);
    if (ret != SUCCESS)
        return ret;

    // store the input block type in the first 4 bytes of the buffer.
    *((int32_t *)bufferptr) = blockType;

    StaticBuffer::blockAllocMap[this->blockNum] = blockType;

    int dirty = StaticBuffer::setDirtyBit(this->blockNum);
    if (dirty != SUCCESS)
        return dirty;

    return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType)
{
    // iterate through the StaticBuffer::blockAllocMap and find the block number
    // of a free block in the disk.
    int freeblk = -1;
    for (int i = 0; i < DISK_BLOCKS; i++)
    {
        if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK)
        {
            freeblk = i;
            break;
        }
    }
    if (freeblk == -1)
        return E_DISKFULL;

    // set the object's blockNum to the block number of the free block
    this->blockNum = freeblk;

    // This method makes use of the setHeader() method to set up the
    //  header in the newly acquired disk block and the setBlockType()
    //  method to update the type of the block in the block allocation map.

    // allocate free buff for the block
    int freebuff = StaticBuffer::getFreeBuffer(freeblk);

    // initialize blocks header with initial values
    struct HeadInfo head;
    head.pblock = -1;
    head.lblock = -1;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = 0;
    head.numSlots = 0;
    BlockBuffer::setHeader(&head);

    // set blocks type
    BlockBuffer::setBlockType(blockType);

    return freeblk;
}

int BlockBuffer::getBlockNum()
{
    return this->blockNum;
}

// The block number to which this instance of BlockBuffer is associated
//(given by the blockNum member field) is freed from the buffer and the disk.
void BlockBuffer::releaseBlock()
{
    // if blockNum is INVALID_BLOCKNUM (-1), or it is invalidated already, do nothing
    if (blockNum == INVALID_BLOCKNUM)
    {
        return;
    }

    int buffNum = StaticBuffer::getBufferNum(blockNum);
    if (buffNum != E_BLOCKNOTINBUFFER)
    {
        StaticBuffer::metainfo[buffNum].free = true;
        StaticBuffer::blockAllocMap[blockNum] = UNUSED_BLK;
    }
    // set the object's blockNum to INVALID_BLOCK (-1)
    this->blockNum = INVALID_BLOCKNUM;
}