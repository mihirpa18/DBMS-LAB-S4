#include "Algebra.h"
#include <iostream>
#include <cctype>
#include <cstring>

bool isNumber(char *str){
    int len;
    float ignore;

    int ret=sscanf(str,"%f %n",&ignore,&len);
    return ret==1 && len==strlen(str);
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE])
{
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry;
    int at = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
    if (at == E_ATTRNOTEXIST)
    {
        return E_ATTRNOTEXIST;
    }

    int type = attrCatEntry.attrType;
    Attribute attrVal;

    if (type == NUMBER)
    {
        if (isNumber(strVal))
        {
            attrVal.nVal = atof(strVal);
        }
        else
        {
            return E_ATTRTYPEMISMATCH;
        }
    }
    else if (type == STRING)
    {
        strcpy(attrVal.sVal, strVal);
    }

    RelCacheTable::resetSearchIndex(srcRelId);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    printf("|");
    for (int i = 0; i < relCatEntry.numAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

        printf(" %s |", attrCatEntry.attrName);
    }
    printf("\n");

    while (true)
    {
        RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

        if (searchRes.block != -1 && searchRes.slot != -1)
        {
            int recSize = relCatEntry.numAttrs;
            Attribute rec[recSize];
            RecBuffer recBuff(searchRes.block);
            recBuff.getRecord(rec, searchRes.slot);

            printf("|");
            for (int i = 0; i < relCatEntry.numAttrs; ++i)
            {
                AttrCatEntry attrCatEntry;

                AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatEntry);
               
               if(attrCatEntry.attrType==NUMBER){
                printf(" %d |",(int)rec[i].nVal);
               }
               else{
                printf(" %s |",rec[i].sVal);
               }
            }
            printf("\n");
        }
        else{
            break;
        }
    }
    return SUCCESS;
}



