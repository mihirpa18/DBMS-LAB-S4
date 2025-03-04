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

/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
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

    /*** Selecting records from the source relation ***/

    RelCacheTable::resetSearchIndex(srcRelId);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    /************************
         The following code prints the contents of a relation directly to the output
        console. Direct console output is not permitted by the actual the NITCbase
        specification and the output can only be inserted into a new relation. We will
        be modifying it in the later stages to match the specification.
    ************************/

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

                AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

                if (attrCatEntry.attrType == NUMBER)
                {
                    printf(" %d |", (int)rec[i].nVal);
                }
                else
                {
                    printf(" %s |", rec[i].sVal);
                }
            }
            printf("\n");
        }
        else
        {
            break;
        }
    }
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