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

    if (OpenRelTable::getRelId(relName) != E_RELNOTOPEN)
        return E_RELOPEN;

    int retVal = BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);

    return retVal;
}

int Schema::closeRel(char relname[ATTR_SIZE])
{
    if (strcmp(relname, RELCAT_RELNAME) == 0 || strcmp(relname, ATTRCAT_RELNAME) == 0)
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

int Schema::createRel(char relName[], int nAttrs, char attrs[][ATTR_SIZE], int attrtype[])
{
    // Attribute type relname
    Attribute relNameAsAttr;
    strcpy(relNameAsAttr.sVal, relName);

    RecId targetRelId = {-1, -1};

    // reset search index of relcat
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    // search for attrval "RelName" = relNameAsAttribute in relcat
    targetRelId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAsAttr, EQ);
    if (targetRelId.block != -1 || targetRelId.slot != -1)
    {
        return E_RELEXIST;
    }

    // check for duplicate attr
    for (int i = 0; i < nAttrs; i++)
    {
        for (int j = i + 1; j < nAttrs; j++)
        {
            if (strcmp(attrs[i], attrs[j]) == 0)
            {
                return E_DUPLICATEATTR;
            }
        }
    }

    /* declare relCatRecord of type Attribute which will be used to store the
     record corresponding to the new relation which will be inserted
     into relation catalog */
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, relName);
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal = nAttrs;
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal = 0;
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = floor((2016 / (16 * nAttrs + 1)));

    int retVal = BlockAccess::insert(RELCAT_RELID, relCatRecord);
    if (retVal != SUCCESS)
        return retVal;

    for (int i = 0; i < nAttrs; i++)
    {
        /* declare Attribute attrCatRecord[6] to store the attribute catalog
           record corresponding to i'th attribute of the argument passed*/
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName);
        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrs[i]);
        attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrtype[i];
        attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal = i;

        retVal = BlockAccess::insert(ATTRCAT_RELID, attrCatRecord);
        if (retVal != SUCCESS)
        {
            // (this is necessary because we had already created the
            //  relation catalog entry which needs to be removed)
            Schema::deleteRel(relName);
            return E_DISKFULL;
        }
    }

    return SUCCESS;
}

int Schema::deleteRel(char relName[])
{
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    if (strcmp(relName, (char *)RELCAT_RELNAME) == 0 || strcmp(relName, (char *)ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }

    // get relid
    int relId = OpenRelTable::getRelId(relName);

    if (relId != E_RELNOTOPEN)
    {
        return E_RELOPEN;
    }

    int ret = BlockAccess::deleteRelation(relName);

    return ret;
}

// This method creates a bplus indexing on an attribute attrName in a relation relName as specified in arguments.

int Schema::createIndex(char relName[ATTR_SIZE], char attrName[ATTR_SIZE]) {
    if (
        strcmp(relName, (char* )RELCAT_RELNAME) == 0 ||
        strcmp(relName, (char* )ATTRCAT_RELNAME) == 0
    ) {
        return E_NOTPERMITTED;
    }

    int relId = OpenRelTable::getRelId(relName);
    if (relId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    return BPlusTree::bPlusCreate(relId, attrName);
}

// This method drops the bplus indexing on an attribute attrName in a relation relName as specified in arguments.
int Schema::dropIndex(char relName[ATTR_SIZE], char attrName[ATTR_SIZE])
{
    // if the relName is either Relation Catalog or Attribute Catalog
    if (strcmp(relName, (char *)RELCAT_RELNAME) == 0 || strcmp(relName, (char *)ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }

    // check if open or not
    int relId = OpenRelTable::getRelId(relName);
    if (relId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    // get the attribute catalog entry corresponding to the attribute and if it fails return attrnotexist
    AttrCatEntry attrCatBuff;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuff);
    if (ret != SUCCESS)
    {
        return E_ATTRNOTEXIST;
    }

    int rootBlock = attrCatBuff.rootBlock;
    if (rootBlock == -1)
    {
        return E_NOINDEX;
    }

    // destroy tree
    BPlusTree::bPlusDestroy(rootBlock);
    // set rootblock=-1 in attrcache
    attrCatBuff.rootBlock = -1;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatBuff);

    return SUCCESS;
}