#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <cstring>
#include <iostream>


void cache_print()
{
  for(int i=0;i<2;i++){
    
    RelCatEntry relCatBuffer;
    RelCacheTable::getRelCatEntry(i,&relCatBuffer);

    printf("Relation: %s\n",relCatBuffer.relName);

    for(int j=0;j<relCatBuffer.numAttrs;j++){
      AttrCatEntry attrCatBuffer;
      AttrCacheTable::getAttrCatEntry(i,j,&attrCatBuffer);
      const char * attrtype=(attrCatBuffer.attrType==NUMBER)?"NUM":"STR";
      printf(" %s: %s\n",attrCatBuffer.attrName,attrtype);
    }
  }
}

int main(int argc, char *argv[])
{
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  cache_print();

  return 0;
}
