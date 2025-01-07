#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <cstring>
#include <iostream>

void printRelation()
{
  Disk disk_run;
  //Creates buffers for relation catalog and attribute catalog 
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader; // Stores block header information
  HeadInfo attrCatHeader;

  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);

  for (int i = 0; i < relCatHeader.numEntries; i++)
  {
    Attribute relCatRecord[RELCAT_NO_ATTRS]; // Stores relation catalog records
    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    int currBlock = ATTRCAT_BLOCK; //    // first block of attribute catalog

    while (currBlock != -1)  //it should not exceed 19(total number of attributes)
    {
      RecBuffer currAttrBuffer(currBlock);
      HeadInfo currAttrHeader;
      currAttrBuffer.getHeader(&currAttrHeader);

      for (int j = 0; j < currAttrHeader.numEntries; j++)
      {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS]; // Stores attribute catalog records
        currAttrBuffer.getRecord(attrCatRecord, j);

        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0)
        {
          const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
          printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
        }
      }

      currBlock = currAttrHeader.rblock;
    }
    printf("\n");
  }
}

void exercise2()
{
  Disk disk_run;
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;

  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);

  for (int i = 0; i < relCatHeader.numEntries; i++)
  {
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    // first block of attribute catalog
    int currBlock = ATTRCAT_BLOCK;

    while (currBlock != -1)
    {
      RecBuffer currAttrBuffer(currBlock);
      HeadInfo currAttrHeader;
      currAttrBuffer.getHeader(&currAttrHeader);

      for (int j = 0; j < currAttrHeader.numEntries; j++)
      {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        currAttrBuffer.getRecord(attrCatRecord, j);

        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0 && strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Batch") == 0)
        {
              strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,"Class");

              attrCatBuffer.setRecord(attrCatRecord,j); //to save the change
          
        }
        
      }

      currAttrBuffer.getHeader(&currAttrHeader);
      currBlock = currAttrHeader.rblock;
    }
    printf("\n");
  }
}

int main(int argc, char *argv[])
{

  printRelation();
  exercise2();
  printRelation();
  return 0;
}
