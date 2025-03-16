#include "StaticBuffer.h"
#include <cstring>
#include<cstdlib>

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

int StaticBuffer::noOfComp=0; //initialization to keep track of count (ex-2)

StaticBuffer::StaticBuffer()
{

  /*we declare the blockAllocMap member field in staticbuffer because the block allocation map
  too is loaded into memory during the runtime of the database. and update our constructor and
   destructor to initialise and write-back the block alloc map between the disk and memory.*/

  // copy blockAllocMap blocks from disk to buffer
   for (int i = 0; i < 4; i++) {
        unsigned char buffer[BLOCK_SIZE];
        Disk::readBlock(buffer, i);
        memcpy(blockAllocMap + i*BLOCK_SIZE, buffer, BLOCK_SIZE);
    }


  for (int i = 0; i < BUFFER_CAPACITY; i++)
  {
    metainfo[i].free = true;
    metainfo[i].dirty = false;
    metainfo[i].timeStamp = -1;
    metainfo[i].blockNum = -1;
  }
}


StaticBuffer::~StaticBuffer()
{
  // copy blockAllocMap blocks from buffer to disk
   for (int i = 0; i < 4; i++) {
        unsigned char buffer[BLOCK_SIZE];
        memcpy(buffer, blockAllocMap + i*BLOCK_SIZE, BLOCK_SIZE);
        Disk::writeBlock(buffer, i);
    }


  for (int i = 0; i < BUFFER_CAPACITY; i++)
  {
    if (metainfo[i].free == false && metainfo[i].dirty == true)
    {
      Disk::writeBlock(StaticBuffer::blocks[i], metainfo[i].blockNum);
    }
  }
}

//if no free buff is found we use LRU policy to free a buff and it is implemented here
int StaticBuffer::getFreeBuffer(int blockNum)
{
  if (blockNum < 0 || blockNum > DISK_BLOCKS)
  {
    return E_OUTOFBOUND;
  }

  for (int i = 0; i < BUFFER_CAPACITY; i++)
  {
    if (metainfo[i].free == false)
    {
      metainfo[i].timeStamp++;
    }
  }

  int bufferNum = -1;

  for (int i = 0; i < BUFFER_CAPACITY; i++)
  {
    if (metainfo[i].free == true)
    {
      bufferNum = i;
      break;
    }
  }

  int maxtime = 0;
  if (bufferNum == -1)
  {
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
      if (metainfo[i].timeStamp > maxtime)
      {
        maxtime = metainfo[i].timeStamp;
        bufferNum = i;
      }
    }
    if (metainfo[bufferNum].dirty == true)
    {
      Disk::writeBlock(StaticBuffer::blocks[bufferNum], metainfo[bufferNum].blockNum);
    }
  }

  metainfo[bufferNum].free = false;
  metainfo[bufferNum].blockNum = blockNum;
  metainfo[bufferNum].dirty = false;
  metainfo[bufferNum].timeStamp = 0;

  return bufferNum;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum)
{

  if (blockNum < 0 || blockNum > DISK_BLOCKS)
  {
    return E_OUTOFBOUND;
  }

  for (int i = 0; i < BUFFER_CAPACITY; i++)
  {
    if (metainfo[i].blockNum == blockNum)
    {
      return i;
    }
  }

  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum)
{
  int buffInd = StaticBuffer::getBufferNum(blockNum);
  if (buffInd == E_BLOCKNOTINBUFFER)
    return E_BLOCKNOTINBUFFER;
  if (buffInd == E_OUTOFBOUND)
    return E_OUTOFBOUND;

  else
  {
    metainfo[buffInd].dirty = true;
  }

  return SUCCESS;
}

//Returns the block type of the block corresponding to the input block number
int StaticBuffer::getStaticBlockType(int blockNum){
  //check if blockNum is Valid
  if(blockNum<0 || blockNum>DISK_BLOCKS){
    return E_OUTOFBOUND;
  }

  unsigned char bufferPtr=StaticBuffer::blockAllocMap[blockNum];
  int blockType=(int )bufferPtr;

  return blockType;
}