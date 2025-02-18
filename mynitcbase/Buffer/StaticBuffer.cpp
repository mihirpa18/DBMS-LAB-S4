#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer()
{

  // initialise all blocks as free
  for (int i = 0; i < BUFFER_CAPACITY; i++)
  {
    metainfo[i].free = true;
    metainfo[i].dirty = false;
    metainfo[i].timeStamp = -1;
    metainfo[i].blockNum = -1;
  }
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer()
{
  for (int i = 0; i < BUFFER_CAPACITY; i++)
  {
    if (metainfo[i].free == false && metainfo[i].dirty == true)
    {
      Disk::writeBlock(StaticBuffer::blocks[i], metainfo[i].blockNum);
    }
  }
}

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