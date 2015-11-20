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
#include <assert.h>

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = -1;
    treeHeight = 0;

     //zero out the buffer
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
	RC ret;
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
    RC ret;
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

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
    RC ret;




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
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
    RC ret;
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
    RC ret;

    PageId c_pid = cursor.pid;
    int c_eid = cursor.eid;

    BTLeafNode l_node;
    ret=l_node.read(cursor.pid,pf);
    if (ret)
    	return RC_FILE_READ_FAILED;

	ret=l_node.readEntry(c_eid, key, rid);
	if (ret)
		return RC_NO_SUCH_RECORD;


	//TODO: increment cursor and check if at end of leaf node







    return ret;
}
