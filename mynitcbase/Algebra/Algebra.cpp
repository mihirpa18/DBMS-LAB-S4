#include "Algebra.h"
#include <iostream>
#include <cctype>
#include <cstring>

bool isNumber(char *str)
{
    int len;
    float ignore;

    int ret = sscanf(str, "%f %n", &ignore, & len);
    return ret == 1 && len == strlen(str);
}

// This function creates a new target relation with attributes as that of source relation.
// It inserts the records of source relation which satisfies the given condition into the target Relation.
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE])
{
    // get source rel relid to check if it is open
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }
    // get attrcat entry to check if attribute exist
    AttrCatEntry attrCatEntry;
    int at = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
    if (at == E_ATTRNOTEXIST)
    {
        return E_ATTRNOTEXIST;
    }

    /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/

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
    
    // creating and opening target rel
    RelCatEntry srcRelCatEnt;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEnt);

    int src_nAttrs = srcRelCatEnt.numAttrs;
   
    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];

    for (int i = 0; i < src_nAttrs; i++)
    {
        AttrCatEntry srcAttrCatEnt;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &srcAttrCatEnt);
        strcpy(attr_names[i], srcAttrCatEnt.attrName);
        attr_types[i] = srcAttrCatEnt.attrType;
    }

    // create newRel
    int ret = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
    if (ret != SUCCESS)
        return ret;

    // open it
    int tarId = OpenRelTable::openRel(targetRel);
    // if opening fails delete it because we already created it
    if (tarId < 0 || tarId >= MAX_OPEN)
    {
        Schema::deleteRel(targetRel);
        return tarId;
    }

    /*** Selecting and inserting of records***/

    Attribute record[src_nAttrs];

    // reset search index of both relcache and attrcache
    RelCacheTable::resetSearchIndex(srcRelId);
    //  AttrCacheTable::resetSearchIndex(srcRelId,offset);

    // read every record that satisfies the condition by repeatedly
    while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS)
    {
        int insert = BlockAccess::insert(tarId, record);
        if (insert != SUCCESS)
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return insert;
        }
    }

    // close the target rel
    Schema::closeRel(targetRel);

    return SUCCESS;
}

// This function creates a copy of the source relation in the target relation.
//  Every record of the source relation is inserted into the target relation.
int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE])
{
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    // get the relcat entry and num of attrs
    RelCatEntry relCatBuff;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatBuff);
    int src_nAttrs = relCatBuff.numAttrs;

    char attrNames[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];

    /*** Checking if attributes of target are present in the source relation
         and storing its offsets and types ***/
    for (int i = 0; i < src_nAttrs; i++)
    {
        AttrCatEntry attrCatBuff;
        int ret = AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatBuff);
        if (ret != SUCCESS)
        {
            return E_ATTRNOTEXIST;
        }

        strcpy(attrNames[i], attrCatBuff.attrName);
        attr_types[i] = attrCatBuff.attrType;
    }

    /*** Creating and opening the target relation ***/
    int ret = Schema::createRel(targetRel, src_nAttrs, attrNames, attr_types);
    if (ret != SUCCESS)
    {
        return ret;
    }

    int tar_relId = OpenRelTable::openRel(targetRel);
    if (tar_relId < 0 || tar_relId >= MAX_OPEN)
    {
        Schema::deleteRel(targetRel);
        return tar_relId;
    }

    // inserting projected records into target rel
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[src_nAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS)
    {

        ret = BlockAccess::insert(tar_relId, record);
        if (ret != SUCCESS)
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    Schema::closeRel(targetRel);
    return SUCCESS;
}

// This function creates a new target relation with list of attributes
//  specified in the arguments. For each record of the source relation,
//   it inserts a new record into the target relation with the attribute
//   values corresponding to the attributes specified in the attribute list
int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE])
{
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    // get the relcat entry and num of attrs
    RelCatEntry relCatBuff;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatBuff);
    int src_nAttrs = relCatBuff.numAttrs;

    int attr_offset[tar_nAttrs];
    int attr_types[tar_nAttrs];

    /*** Checking if attributes of target are present in the source relation
         and storing its offsets and types ***/
    for (int i = 0; i < tar_nAttrs; i++)
    {
        AttrCatEntry attrCatBuff;
        int ret = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatBuff);
        if (ret != SUCCESS)
        {
            return E_ATTRNOTEXIST;
        }

        attr_offset[i] = attrCatBuff.offset;
        attr_types[i] = attrCatBuff.attrType;
    }

    /*** Creating and opening the target relation ***/
    int ret = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types);
    if (ret != SUCCESS)
    {
        return ret;
    }

    int tar_relId = OpenRelTable::openRel(targetRel);
    if (tar_relId < 0 || tar_relId >= MAX_OPEN)
    {
        Schema::deleteRel(targetRel);
        return tar_relId;
    }

    // inserting projected records into target rel
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[src_nAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS)
    {
        Attribute proj_record[tar_nAttrs];
        for (int i = 0; i < tar_nAttrs; i++)
        {
            proj_record[i] = record[attr_offset[i]];
        }
        ret = BlockAccess::insert(tar_relId, proj_record);
        if (ret != SUCCESS)
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    Schema::closeRel(targetRel);
    return SUCCESS;
}



int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE])
{   
    //rel should not be relcat or attrcat
    if (strcmp(relName, (char *)RELCAT_RELNAME) == 0 || strcmp(relName, (char *)ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }

    //CHECK WHETHER REL IS OPEN
    int id = OpenRelTable::getRelId(relName);
    if (id == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    //get relcat entry from relcache
    RelCatEntry relCatBuff;
    RelCacheTable::getRelCatEntry(id,&relCatBuff);
    if (relCatBuff.numAttrs != nAttrs)
        return E_NATTRMISMATCH;

    Attribute recordValues[nAttrs];
    
    //attrcat
    for (int i = 0; i < nAttrs; i++)
    {
        AttrCatEntry attrCatBuff;
        AttrCacheTable::getAttrCatEntry(id, i, &attrCatBuff);
        int type = attrCatBuff.attrType;
        //compare type of attribute in attrcat and record[]
        if (type == NUMBER)
        {
            if (!isNumber(record[i]))
            {
                return E_ATTRTYPEMISMATCH;
            }
            recordValues[i].nVal = atof(record[i]);
        }
        else if (type == STRING)
        {
            strcpy(recordValues[i].sVal, record[i]);
        }
    }

    int retVal = BlockAccess::insert(id, recordValues);
    return retVal;
}