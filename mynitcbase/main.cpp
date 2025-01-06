#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <cstring>
#include <iostream>

int main(int argc, char *argv[])
{
  Disk disk_run;
  // unsigned char buffer[BLOCK_SIZE];

  // Disk::readBlock(buffer, 7000);

  // char message[] = "hello world my name is mihir";
  // memcpy(buffer + 20, message, 10);
  // Disk::writeBlock(buffer, 7000);

  // unsigned char buffer2[BLOCK_SIZE];
  // char message2[10];
  // Disk::readBlock(buffer2, 7000);
  // memcpy(message2, buffer2 + 20, 10);

  // std::cout << message2 << std::endl;

  unsigned char BAM[BLOCK_SIZE];
  Disk::readBlock(BAM,0);

  for(int i=0;i<BLOCK_ALLOCATION_MAP_SIZE+100;i++){
    std::cout<<(int)BAM[i] << " ";
    
  }
  // return FrontendInterface::handleFrontend(argc, argv);
  return 0;
}