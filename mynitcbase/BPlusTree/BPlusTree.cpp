#include "BPlusTree.h"

#include <cstring>

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
    // index id which will store  search index of attrName
    IndexId searchIndex;
    AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    // load attrCat entry
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    int block, index;

    // if search is done for first time
    if (searchIndex.block == -1 && searchIndex.index == -1)
    {
        block = attrCatEntry.rootBlock;
        index = 0;
        // if attrName dont have B+ tree
        if (block == -1)
        {
            return RecId{-1, -1};
        }
    }
    else
    {
        // search resumed from next index
        block = searchIndex.block;
        index = searchIndex.index + 1;

        // load block into leaf
        IndLeaf leaf(block);
        // load the leafHead
        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries)
        {
            // all the entries in the current block has been searched , go to rblock
            block = leafHead.rblock;
            index=0;
            // check if it is last block of relation
            if (block == -1)
            {
                return RecId{-1, -1};
            }
        }
    }

    /******  Traverse through all the internal nodes according to value
        of attrVal and the operator op                             ******/

    /* (This section is only needed when
        - search restarts from the root block (when searchIndex is reset by caller)
        - root is not a leaf
        If there was a valid search index, then we are already at a leaf block
        and the test condition in the following loop will fail)
    */

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL)
    {
        // load the block to internalBlk
        IndInternal internalBlk(block);

        // load the header
        HeadInfo intHead;
        internalBlk.getHeader(&intHead);

        // to store entry of internalBlk
        InternalEntry intEntry;

        if (op == NE || op == LE || op == LT)
        {
            /* if op=
                        - NE: need to search the entire linked list of leaf indices of the B+ Tree,
                        starting from the leftmost leaf index. Thus, always move to the left.

                        - LT and LE: the attribute values are arranged in ascending order in the
                        leaf indices of the B+ Tree. Values that satisfy these conditions, if
                        any exist, will always be found in the left-most leaf index. Thus,
                        always move to the left.
                        */
            
            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;
        }
        else
        {
            /*
        - EQ, GT and GE: move to the left child of the first entry that is
        greater than (or equal to) attrVal
        (we are trying to find the first entry that satisfies the condition.
        since the values are in ascending order we move to the left child which
        might contain more entries that satisfy the condition)
        */

            // traverse through all entries of internalBlk and find entry thatt satifies the condition
            int finder = -1;
            for (int i = 0; i < intHead.numEntries; i++)
            {
                internalBlk.getEntry(&intEntry, i);
                if ((op == GE || op == EQ))
                {

                    if (compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType) >= 0)
                    {
                        finder = i;
                        break;
                    }
                }
                else if (op == GT)
                {
                    if (compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType) > 0)
                    {
                        finder = i;
                        break;
                    }
                }
            }
            // if you found an entry
            if (finder != -1)
            {
                internalBlk.getEntry(&intEntry, finder);
                block = intEntry.lChild;
            }
            else
            {
                internalBlk.getEntry(&intEntry, intHead.numEntries - 1);
                block = intEntry.rChild;
            }
        }
    }

    // now block has block=block num of leafblock
    /******  Identify the first leaf index entry from the current position
                that satisfies our condition (moving right)             ******/

    while (block != -1)
    {
        // load block into leafBlk
        IndLeaf leafBlk(block);

        // get the header
        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);

        Index leafEntry;

        while (index < leafHead.numEntries)
        {
            // load entry corresponding to block and index
            leafBlk.getEntry(&leafEntry, index);

            /* comparison between leafEntry's attribute value
                            and input attrVal using compareAttrs()*/
            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);

            // if entry satisfying the condition found)
            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0))
            {
                // set search index
                searchIndex = {block, index};
                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);
                return RecId({leafEntry.block, leafEntry.slot});
            }
            /*future entries will not satisfy EQ, LE, LT since the values
                    are arranged in ascending order in the leaves */
            else if ((op == EQ || op == LE || op == LT) && cmpVal > 0)
            {
                return RecId({-1, -1});
            }
            // search next index
            ++index;
        }

        /*only for NE operation do we have to check the entire linked list;
        for all the other op it is guaranteed that the block being searched
        will have an entry, if it exists, satisying that op. */

        if (op != NE)
        {
            break;
        }
        block = leafHead.rblock;
        index = 0;
    }

    return RecId({-1, -1});
}

