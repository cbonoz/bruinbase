
/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{   
    RC error;

    //Keys are assumed to be non-zero, but we can check it anyway
    if(key<0)
        return RC_INVALID_ATTRIBUTE;
    
    //If tree doesn't exist yet, we simply need to start a new tree (root)
    if(treeHeight==0)
    {
        //Create a new leaf node with inserted value to act as root
        BTLeafNode newTree;
        newTree.insert(key, rid);
        
        //Update BTree's root pid
        //If endPid is zero (file just created), set it to 1 so the first PageId can be accessed
        //We want rootPid to start at 1 so we can use the PageFile at pid=0 for storing private data
        if(pf.endPid()==0)
            rootPid = 1;
        else
            rootPid = pf.endPid();
        
        //If insert successful, increment tree height
        treeHeight++;
        
        //Write tree into specified pid in PageFile
        return newTree.write(rootPid, pf);
    }
    
    //Otherwise, we'll have to traverse the tree and insert where possible
    //Again, start with pid at 1, since we won't insert at a pid of 0
    //Current height also starts at 1, since we should have a root by now
    
    //These variables will be used to store keys that must be inserted in higher level nodes
    int insertKey = -1;
    PageId insertPid = -1;
    
    error = insertRec(key, rid, 1, rootPid, insertKey, insertPid);
    
    if(error!=0)
        return error;
    
    return 0;
}

//Recursive function for inserting key into correct leaf and non-leaf nodes alike
RC BTreeIndex::insertRec(int key, const RecordId& rid, int currHeight, PageId thisPid, int& tempKey, PageId& tempPid)
{
    //If anything breaks along the way, return as error
    RC error;
    
    //These variables may be used later to facilitate splitting and parent inserting
    //Between different levels of recursion
    tempKey = -1;
    tempPid = -1;
    
    //We start at a height of 1
    //If we reach the tree's max height, we can simply add a leaf node
    if(currHeight==treeHeight)
    {
        //Generate "new" leaf node and read contents over
        BTLeafNode thisLeaf;
        thisLeaf.read(thisPid, pf);

        //Try inserting leaf node
        //If succesful, write back into PageFile and return
        if(thisLeaf.insert(key, rid)==0)
        {   
            thisLeaf.write(thisPid, pf);
            return 0;
        }

        //At this point, insert was not successful (likely due to overflow)
        //Try inserting leaf node via splitting
        BTLeafNode anotherLeaf;
        int anotherKey;
        error = thisLeaf.insertAndSplit(key, rid, anotherLeaf, anotherKey);
        
        if(error!=0)
            return error;
        
        //Right now, anotherKey is the median key that needs to be placed into parent
        //We'll utilize the parameters for that purpose
        int lastPid = pf.endPid();
        tempKey = anotherKey;
        tempPid = lastPid;

        //Write new contents into thisLeaf and anotherLeaf
        anotherLeaf.setNextNodePtr(thisLeaf.getNextNodePtr());
        thisLeaf.setNextNodePtr(lastPid);

        //Notice that anotherLeaf starts writing at the end of the last pid
        //The node anotherLeaf gets tacked on to the end of the PageFile, incrementing endPid()
        error = anotherLeaf.write(lastPid, pf);
        
        if(error!=0)
            return error;
        
        error = thisLeaf.write(thisPid, pf);
        
        if(error!=0)
            return error;
        
        //If we just split a root, we'll now need a new single non-leaf node
        //The new first value of the sibling node (anotherLeaf) gets inserted into root
        if(treeHeight==1)
        {
            //We create a root that has pid pointers to both the new children (which we just split)
            //The new root's value is anotherKey, which is the median key that was pushed up in split
            BTNonLeafNode newRoot;
            newRoot.initializeRoot(thisPid, anotherKey, lastPid);
            treeHeight++;
            
            //Update the rootPid, then write into the PageFile and return
            rootPid = pf.endPid();
            newRoot.write(rootPid, pf);
        }
        
        return 0;
    }
    else
    {
        //Otherwise, we're still somewhere in the middle of the tree
        BTNonLeafNode midNode;
        midNode.read(thisPid, pf);
        
        //Since we're in the middle, we find the corresponding child node for key
        PageId childPid = -1;
        midNode.locateChildPtr(key, childPid);
        
        int insertKey = -1;
        PageId insertPid = -1;
        
        //Recursive part: keep traversing through the tree, inserting at the next node closer to leaf
        error = insertRec(key, rid, currHeight+1, childPid, insertKey, insertPid);
        
        //Error in inserting to node due to reaching full capacity
        //We split the level below to make more space
        //if(error!=0)
        //This means some node was split, and we'll have to add a median key to the parent node
        if(!(insertKey==-1 && insertPid==-1)) 
        {
            if(midNode.insert(insertKey, insertPid)==0)
            {
                //If we were able to successfully insert the child's median key into midNode
                //Write it into PageFile
                midNode.write(thisPid, pf);
                return 0;
            }
            
            //Otherwise, this level had no space either (insert was not successful; overflow)
            //We'll have to insert and split again and propagate the median key upwards to the next parent
            BTNonLeafNode anotherMidNode;
            int anotherKey;
            
            midNode.insertAndSplit(insertKey, insertPid, anotherMidNode, anotherKey);
            
            //As before, even if we split in a nonleaf node, we'll still need its median key for the parent
            //Update the parameters
            int lastPid = pf.endPid();
            tempKey = anotherKey;
            tempPid = lastPid;
            
            //Write new contents into midNode and anotherMidNode
            error = midNode.write(thisPid, pf);
            
            if(error!=0)
                return error;
            
            error = anotherMidNode.write(lastPid, pf);
            
            if(error!=0)
                return error;
            
            //If we just split a root, we'll now need a new single non-leaf node
            //The new first value of the sibling node (anotherMidNode) gets inserted into root
            if(treeHeight==1)
            {
                //We create a root that has pid pointers to both the new children (which we just split)
                //The new root's value is anotherKey, which is the median key that was pushed up in split
                BTNonLeafNode newRoot;
                newRoot.initializeRoot(thisPid, anotherKey, lastPid);
                treeHeight++;
                
                //Update the rootPid, then write into the PageFile and return
                rootPid = pf.endPid();
                newRoot.write(rootPid, pf);
            }
            
        }
        return 0;
    }
}
