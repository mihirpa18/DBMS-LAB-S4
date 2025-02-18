#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char relname[ATTR_SIZE])
{
    int ret = OpenRelTable::openRel(relname);
    if (ret >= 0)
    {
        return SUCCESS;
    }
    return ret;
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE])
{

    if (strcmp(oldRelName, (char *)RELCAT_RELNAME) == 0 || strcmp(oldRelName, (char *)ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;
    if (strcmp(newRelName, (char *)RELCAT_RELNAME) == 0 || strcmp(newRelName, (char *)ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;

    if (OpenRelTable::getRelId(oldRelName) != E_RELNOTOPEN)
        return E_RELOPEN;

    int retVal = BlockAccess::renameRelation(oldRelName, newRelName);

    return retVal;
}

int Schema::renameAttr(char relName[ATTR_SIZE], char oldAttrName[ATTR_SIZE], char newAttrName[ATTR_SIZE])
{
    if (strcmp(relName, (char *)RELCAT_RELNAME) == 0 || strcmp(relName, (char *)ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;
    
    if(OpenRelTable::getRelId(relName)!=E_RELNOTOPEN)
        return E_RELOPEN;
    
    int retVal=BlockAccess::renameAttribute(relName,oldAttrName,newAttrName);

    return retVal;
}

int Schema::closeRel(char relname[ATTR_SIZE])
{
    if (strcmp(relname, RELCAT_RELNAME) == 0 && strcmp(relname, ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }
    int relId = OpenRelTable::getRelId(relname);

    if (relId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    return OpenRelTable::closeRel(relId);
}