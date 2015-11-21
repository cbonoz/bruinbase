/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    //empty tree index contructor
    rootPid = -1;
    treeHeight = 0;

}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
	RC ret=0;
	ret = pf.open(indexname,mode);
	if (ret)
		return RC_FILE_OPEN_FAILED;
	else {

		char *buffer = (char *) malloc(P_SIZE * sizeof(char));
		memset(buffer,0,P_SIZE);

		//page file is not empty
		PageId pf_length = pf.endPid();
		if (pf_length) {
			ret = pf.read(pf_length-1,buffer);
			if (ret) {
				free(buffer);
				return RC_FILE_READ_FAILED;
			}


			//we are storing the rootpid and the treeheight in each node in the first pair slot
			//rootPid at offsettposition 0
			//treeHeight at offset position PID_SIZE
			memcpy(&rootPid, buffer,PID_SIZE);
			memcpy(&treeHeight, buffer+PID_SIZE, sizeof(int));
			rootPid = rootPid >= 0 ? rootPid : NONE;
			treeHeight = treeHeight > 0 ? treeHeight : 0;
			
		} else { //pagefile is empty, set members to empty (initial) valuesn
			rootPid=-1;
			treeHeight=0;
			ret = pf.write(0,buffer);
			if (ret) {
				free(buffer);
				return RC_FILE_WRITE_FAILED;
			}
		}

		free(buffer);
	}


	

	return ret; 

}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
    RC ret=0;
    //smallest unit of read or write in pagefile is 1 page (need to allocate 
    //the space of one page)
    char *buffer = (char *) malloc(P_SIZE * sizeof(char));
	memset(buffer,0,P_SIZE);

    //destination, source, size
    //copy the rootpid and treeheight values back into the buffer before we close

    memcpy(buffer, &rootPid, PID_SIZE);
    memcpy(buffer+PID_SIZE, &treeHeight, sizeof(int));

    //write the buffer to position 0 in the pagefile
    ret=pf.write(0, buffer);
    if (ret) {
    	free(buffer);
    	return RC_FILE_WRITE_FAILED;
    }

    //lastly close the file
    ret=pf.close();
    if (ret) {
    	free(buffer);
    	return RC_FILE_CLOSE_FAILED;
    }


    free(buffer);


    return ret;
}

RC BTreeIndex::insertInParent(int mid_key, vector<PageId> &pids) {
    RC ret=0;

    PageId siblingPid = pids.back(); 
    pids.pop_back();

    PageId lnodePid= pids.back(); 
    pids.pop_back();

    PageId parentPid= pids.back(); 
    pids.pop_back();


    if (parentPid == rootPid) {
        BTNonLeafNode root;
        root.initializeRoot(lnodePid, mid_key, siblingPid);
        PageId newRootPid = pf.endPid();

        root.write(newRootPid, pf);

        if (ret)
            return RC_FILE_WRITE_FAILED;

        rootPid = newRootPid;
        treeHeight++;
        return ret;
    }

    BTNonLeafNode parent;
    ret=parent.read(parentPid, pf);
    if (ret)
        return RC_FILE_READ_FAILED;

    int kc=parent.getKeyCount();

    if (kc<N_KEY) {
        //if there is space in the parent node
        parent.insert(mid_key, siblingPid);
    } else {
        //no space in the parent node - split
        PageId newSiblingPid=pf.endPid();
        BTNonLeafNode newSibling;
        int new_mid_key;


        //push l_node pid
        pids.push_back(parentPid);
        //push sibling node pid
        pids.push_back(newSiblingPid);

        //no space in the parent node
        parent.insertAndSplit(mid_key, parentPid, newSibling, new_mid_key);
        ret=newSibling.write(newSiblingPid, pf);
        if (ret)
            return RC_FILE_WRITE_FAILED;


        ret = insertInParent(new_mid_key, pids);

    }


    return ret;
}
/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */

 //following the book algorithm
RC BTreeIndex::insert(int key, const RecordId& rid)
{
    RC ret=0;
    vector<int> pids;


    //tree is empty- - initialize it
    if (treeHeight==0) {
        BTLeafNode root;
        ret=root.insert(key ,rid);

        rootPid = pf.endPid();
        //make sure minimal rootPid value is 1 since tree now initialized
        rootPid=rootPid > 0 ? rootPid : 1;

        ret=root.write(rootPid,pf);
        if (ret) 
            return RC_FILE_WRITE_FAILED;

        return ret;
    }

    

    IndexCursor targetIdx;
    locate(key,targetIdx,1);

    PageId targetPid = targetIdx.pid;
    BTLeafNode l_node;
    l_node.read(targetPid, pf);

    int kc=l_node.getKeyCount();

    if (kc<N_KEY) {
        //l_node is not full - insert
        l_node.insert(key, rid);
    } else {
        //no space in the parent node - split

        //check if leadnode is full of keys
        PageId siblingPid = pf.endPid();
        BTLeafNode sibling;
        int mid_key;
    
        //push l_node pid
        pids.push_back(targetPid);
        //push sibling node pid
        pids.push_back(siblingPid);

        //l_node is full - insert and split
        l_node.insertAndSplit(key, rid, sibling, mid_key);
        ret=sibling.write(siblingPid, pf);
        if (ret)
            return RC_FILE_WRITE_FAILED;

        sibling.setNextNodePtr(l_node.getNextNodePtr());

        ret = insertInParent(mid_key, pids);
    }



    




    return ret;
}

/**
 * Run the standard B+Tree key search algorithm and identify the
 * leaf node where searchKey may exist. If an index entry with
 * searchKey exists in the leaf node, set IndexCursor to its location
 * (i.e., IndexCursor.pid = PageId of the leaf node, and
 * IndexCursor.eid = the searchKey index entry number.) and return 0.
 * If not, set IndexCursor.pid = PageId of the leaf node and
 * IndexCursor.eid = the index entry immediately after the largest
 * index key that is smaller than searchKey, and return the error
 * code RC_NO_SUCH_RECORD.
 * Using the returned "IndexCursor", you will have to call readForward()
 * to retrieve the actual (key, rid) pair from the index.
 * @param key[IN] the key to find
 * @param cursor[OUT] the cursor pointing to the index entry with
 *                    searchKey or immediately behind the largest key
 *                    smaller than searchKey.
 * @return 0 if searchKey is found. Othewise an error code
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor, int isTracking=0)
{
    RC ret=0;
    //clear the existing pids chain
    pids.erase(pids.begin(),pids.end());
    //we read the node at the page id 'pid' in the pagefile into a leaf or nonleaf node

    int current_level=0;
    assert(treeHeight>=0);
    assert(rootPid<0);
    
    //BTNonLeafNode
    //BTLeafNode
    PageId curPid;
    BTNonLeafNode nl_node;
    
    while (current_level++ != treeHeight) {
    	//we are not at the leaf level yet
    	ret=nl_node.read(curPid,pf);
	    if (ret)
	    	return RC_FILE_READ_FAILED;



        if (isTracking)
            pids.push_back(curPid);
    	ret=nl_node.locateChildPtr(searchKey, curPid);
	    if (ret)
	    	return RC_NO_SUCH_RECORD;

    }

    BTLeafNode l_node;
    ret=l_node.read(curPid,pf);
    if (ret)
    	return RC_FILE_READ_FAILED;


    cursor.pid=curPid;
    ret=l_node.locate(searchKey, cursor.eid);
    if (ret)
    	return RC_NO_SUCH_RECORD;

    return ret;
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{
    RC ret=0;

    PageId c_pid = cursor.pid;
    int c_eid = cursor.eid;

    BTLeafNode l_node;
    ret=l_node.read(cursor.pid,pf);
    if (ret)
    	return RC_FILE_READ_FAILED;

	ret=l_node.readEntry(c_eid, key, rid);
	if (ret)
		return RC_INVALID_CURSOR;

    int kc = l_node.getKeyCount();

    if (c_eid==kc) {
        cursor.eid=0;
        cursor.pid=l_node.getNextNodePtr();

    } else {
        cursor.eid=c_eid+1;
        cursor.pid=c_pid;
    }

    return ret;
}
