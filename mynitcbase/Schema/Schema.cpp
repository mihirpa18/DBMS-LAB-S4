#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char relname[ATTR_SIZE]){
    int ret=OpenRelTable::openRel(relname);
    if(ret>=0){
        return SUCCESS;
    }
    return ret;
}

int Schema::closeRel(char relname[ATTR_SIZE]){
    if(strcmp(relname,RELCAT_RELNAME)==0 && strcmp(relname,ATTRCAT_RELNAME)==0){
        return E_NOTPERMITTED;
    }
    int relId=OpenRelTable::getRelId(relname);
    
    if(relId==E_RELNOTOPEN){
        return E_RELNOTOPEN;
    }

    return OpenRelTable::closeRel(relId);
}