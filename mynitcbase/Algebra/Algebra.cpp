#include "Algebra.h"
#include <iostream>
#include <cctype>
#include <cstring>

bool isNumber(char *str)
{
    int len;
    float ignore;

    int ret = sscanf(str, "%f %n", &ignore, &len);
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
    AttrCacheTable::resetSearchIndex(srcRelId, attr);

    StaticBuffer::noOfComp = 0;

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
    printf("Number of comparisons = %d\n", StaticBuffer::noOfComp);

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
    // rel should not be relcat or attrcat
    if (strcmp(relName, (char *)RELCAT_RELNAME) == 0 || strcmp(relName, (char *)ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }

    // CHECK WHETHER REL IS OPEN
    int id = OpenRelTable::getRelId(relName);
    if (id == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    // get relcat entry from relcache
    RelCatEntry relCatBuff;
    RelCacheTable::getRelCatEntry(id, &relCatBuff);
    if (relCatBuff.numAttrs != nAttrs)
        return E_NATTRMISMATCH;

    Attribute recordValues[nAttrs];

    // attrcat
    for (int i = 0; i < nAttrs; i++)
    {
        AttrCatEntry attrCatBuff;
        AttrCacheTable::getAttrCatEntry(id, i, &attrCatBuff);
        int type = attrCatBuff.attrType;
        // compare type of attribute in attrcat and record[]
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

// This function creates a new target relation with attributes constituting from both the source relations
//  (excluding the specified join-attribute from the second source relation)
int Algebra::join(char srcRel1[ATTR_SIZE], char srcRel2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attr1[ATTR_SIZE], char attr2[ATTR_SIZE])
{
    // get relIds of both src rels
    int srcRelId1 = OpenRelTable::getRelId(srcRel1);
    int srcRelId2 = OpenRelTable::getRelId(srcRel2);
    // check if they are open
    if (srcRelId1 == E_RELNOTOPEN || srcRelId2 == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    // get the attribute catalog entries
    AttrCatEntry attrCatEntry1, attrCatEntry2;
    int ret1 = AttrCacheTable::getAttrCatEntry(srcRelId1, attr1, &attrCatEntry1);
    int ret2 = AttrCacheTable::getAttrCatEntry(srcRelId2, attr2, &attrCatEntry2);
    if (ret1 == E_ATTRNOTEXIST || ret2 == E_ATTRNOTEXIST)
    {
        return E_ATTRNOTEXIST;
    }

    // if type of attr1 and attr2 are not same
    if (attrCatEntry1.attrType != attrCatEntry2.attrType)
    {
        return E_ATTRTYPEMISMATCH;
    }

    // get relcat entry
    RelCatEntry relCatEntry1, relCatEntry2;
    ret1 = RelCacheTable::getRelCatEntry(srcRelId1, &relCatEntry1);
    ret2 = RelCacheTable::getRelCatEntry(srcRelId2, &relCatEntry2);
    if (ret1 != SUCCESS)
        return ret1;
    if (ret2 != SUCCESS)
        return ret2;

    AttrCatEntry tempAttrCat1;
    AttrCatEntry tempAttrCat2;
    int n = relCatEntry1.numAttrs;
    int m = relCatEntry2.numAttrs;
    for (int i = 0; i < n; i++)
    {
        AttrCacheTable::getAttrCatEntry(srcRelId1, i, &tempAttrCat1);
        for (int j = 0; j < m; j++)
        {
            AttrCacheTable::getAttrCatEntry(srcRelId2, j, &tempAttrCat2);
            if ((strcmp(tempAttrCat1.attrName, attr1) == 0 && strcmp(tempAttrCat2.attrName, attr2) == 0))
            {
                continue;
            }
            if (strcmp(tempAttrCat1.attrName, tempAttrCat2.attrName) == 0)
            {
                return E_DUPLICATEATTR;
            }
        }
    }

    // if rel 2 doesnt have index, create it
    if (attrCatEntry2.rootBlock == -1)
    {
        int retVal = BPlusTree::bPlusCreate(srcRelId2, attr2);
        if (retVal != SUCCESS)
        {
            return retVal;
        }
    }
    // target rel
    int numOfAttributesInTarget = n + m - 1;
    char targetRelAttrNames[numOfAttributesInTarget][ATTR_SIZE];
    int targetRelAttrTypes[numOfAttributesInTarget];

    // iterate through all the attributes in both the source relations and
    // update targetRelAttrNames[],targetRelAttrTypes[] arrays excluding attribute2
    // in srcRelation2 (use AttrCacheTable::getAttrCatEntry())
    int targ_it=0;
    for (int i = 0; i < n; i++)
    {
        AttrCacheTable::getAttrCatEntry(srcRelId1, i, &tempAttrCat1);
        strcpy(targetRelAttrNames[targ_it], tempAttrCat1.attrName);
        targetRelAttrTypes[targ_it] = tempAttrCat1.attrType;
        targ_it++;
    }
    for (int i = 0; i < m; i++)
    {
        AttrCacheTable::getAttrCatEntry(srcRelId2, i, &tempAttrCat2);
        if (strcmp(tempAttrCat2.attrName, attr2) != 0)
        {
            strcpy(targetRelAttrNames[targ_it], tempAttrCat2.attrName);
            targetRelAttrTypes[targ_it] = tempAttrCat2.attrType;
            targ_it++;
        }
    }

    int retVal = Schema::createRel(targetRelation, numOfAttributesInTarget, targetRelAttrNames, targetRelAttrTypes);
    if (retVal != SUCCESS)
    {
        return retVal;
    }
    int tar_relId = OpenRelTable::openRel(targetRelation);
    if (tar_relId < 0 || tar_relId >= MAX_OPEN)
    {
        Schema::deleteRel(targetRelation);
        return tar_relId;
    }

    int numOfAttributes1 = n;
    int numOfAttributes2 = m;
    Attribute record1[numOfAttributes1];
    Attribute record2[numOfAttributes2];
    Attribute targetRecord[numOfAttributesInTarget];

    RelCacheTable::resetSearchIndex(srcRelId1);
    AttrCacheTable::resetSearchIndex(srcRelId1, attr1);
    // this loop is to get every record of the srcRelation1 one by one
    while (BlockAccess::project(srcRelId1, record1) == SUCCESS)
    {
        RelCacheTable::resetSearchIndex(srcRelId2);
        AttrCacheTable::resetSearchIndex(srcRelId2, attr2);

        // this loop is to get every record of the srcRelation2 which satisfies
        // the following condition:
        // record1.attribute1 = record2.attribute2 (i.e. Equi-Join condition)
        while (BlockAccess::search(srcRelId2, record2, attr2, record1[attrCatEntry1.offset], EQ) == SUCCESS)
        {
            // copy srcRelation1's and srcRelation2's attribute values(except
            // for attribute2 in rel2) from record1 and record2 to targetRecord
            targ_it=0;
            for (int i = 0; i < numOfAttributes1; i++)
            {
                targetRecord[targ_it++] = record1[i];
            }
            for (int i = 0; i < numOfAttributes2; i++)
            {
                if (attrCatEntry2.offset != i)
                {
                    targetRecord[targ_it++] = record2[i];
                }
            }
            // insert
            int retVal = BlockAccess::insert(tar_relId, targetRecord);
            if (retVal != SUCCESS)
            {
                OpenRelTable::closeRel(tar_relId);
                Schema::deleteRel(targetRelation);
                return E_DISKFULL;
            }
        }
    }

    OpenRelTable::closeRel(tar_relId);
    return SUCCESS;
}