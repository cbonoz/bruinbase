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
    // empty tree index contructor
    rootPid = -1;
    treeHeight = 0;

    // should we init vector<PageId> path here too?
}

RC BTreeIndex::insertKey(int seq) {
    RC ret = 0;
    IndexCursor cursor;
    RecordId rid;
    int key;
    rid.sid = rid.pid = seq;
    key = seq * 100;
    printf("\n+++ BTreeIndex::insertKey(): Key = %d, recordId = <%d, %d>\n", key, rid.pid, rid.sid);
    ret = insert(key, rid);
    return ret;
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
    RC ret = 0;
    ret = pf.open(indexname, mode);
    if (ret != 0) {
        return ret; // RC_FILE_OPEN_FAILED;
    } else {
        char *buffer = (char *) malloc(P_SIZE * sizeof(char));
        memset(buffer, 0, P_SIZE);

        PageId indexEndPid = pf.endPid();
        // if (DEBUG) printf("BTreeIndex::open() - endPid for pf is %d\n", indexEndPid);
        if (indexEndPid == 0) { // pagefile is empty, set members to empty (initial) values
            rootPid = -1;
            treeHeight = 0;
            memcpy(buffer, &rootPid, PID_SIZE);
            memcpy((buffer + PID_SIZE), &treeHeight, sizeof(int));

            ret = pf.write(0, buffer);
            // if (DEBUG) printf("BTreeIndex::open() - empty pagefile, written back the rootPid = %d and treeHeight = %d\n", rootPid, treeHeight);
            if (ret != 0) {
                free(buffer);
                return ret; // RC_FILE_WRITE_FAILED;
            }
        } else { // page file is not empty, we read first page of the index file
            // ret = pf.read(indexEndPid - 1, buffer); // should read the first page
            ret = pf.read(0, buffer);   // read the first page to get rootPid and treeHeight
            
            // if (DEBUG) printf("BTreeIndex::open() - pagefile not empty, we read it into buffer\n");
            if (ret != 0) {
                free(buffer);
                return ret; // RC_FILE_READ_FAILED;
            }

            // we are storing the rootpid and the treeheight
            // in the first page of that index file
            // rootPid at offset position 0
            // treeHeight at offset position PID_SIZE
            memcpy(&rootPid, buffer, PID_SIZE);
            memcpy(&treeHeight, buffer + PID_SIZE, sizeof(int));
            // if (DEBUG) printf("BTreeIndex::open() - pagefile not empty, got rootPid = %d, treeheight = %d\n", rootPid, treeHeight);
            // rootPid = rootPid >= 0 ? rootPid : -1;
            // treeHeight = treeHeight > 0 ? treeHeight : 0;
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
    RC ret = 0;
    // smallest unit of read or write in pagefile is 1 page
    // (need to allocate the space of one page)
    char *buffer = (char *) malloc(P_SIZE * sizeof(char));
    memset(buffer, 0, P_SIZE);

    // memcpy(destination, source, size)
    // copy the rootpid and treeheight values back into the buffer before we close
    memcpy(buffer, &rootPid, PID_SIZE);
    memcpy((buffer + PID_SIZE), &treeHeight, sizeof(int));

    // write the buffer to first page in the pagefile
    ret = pf.write(0, buffer);
    if (DEBUG) printf("BTreeIndex::close() - written back the rootPid and treeHeight\n");
    if (ret != 0) {
        free(buffer);
        return RC_FILE_WRITE_FAILED;
    }

    // lastly close the file
    ret = pf.close();
    if (ret != 0) {
        free(buffer);
        return RC_FILE_CLOSE_FAILED;
    }

    if (DEBUG) printf("BTreeIndex::close() - pagefile closed\n");
    free(buffer);
    return ret;
}

RC BTreeIndex::insertInParent(vector<PageId> &path, int siblingKey) {
    RC ret = 0;

    // get pid of the splitted sibling node
    PageId siblingPid = path.back(); 
    path.pop_back();

    // get pid of the current node
    PageId currentPid = path.back(); 
    path.pop_back();

    // get pid of the parent node of the current node
    PageId parentPid = path.back(); 
    path.pop_back();

    if (parentPid > 0 && parentPid != rootPid) { // current node is not a root node, it has a parent, so recursively insert into current node's parent
        BTNonLeafNode parentNode;
        ret = parentNode.read(parentPid, pf);
        if (ret != 0) { return ret; } // RC_FILE_READ_FAILED;
        
        if (parentNode.getKeyCount() < N_KEY) { // if there is space in the parent node
            parentNode.insert(siblingKey, siblingPid);
            ret = parentNode.write(parentPid, pf);
            if (ret != 0) { return ret; } // RC_FILE_WRITE_FAILED;    
        } else { // no space in the parent node -- split
            PageId newSiblingPid = pf.endPid();
            BTNonLeafNode newSiblingNode;
            int newSiblingKey;

            // no space in the parent node
            parentNode.insertAndSplit(siblingKey, siblingPid, newSiblingNode, newSiblingKey);

            ret = newSiblingNode.write(newSiblingPid, pf);
            if (ret != 0) { return ret; } // RC_FILE_WRITE_FAILED;
            ret = parentNode.write(parentPid, pf);
            if (ret != 0) { return ret; } // RC_FILE_WRITE_FAILED;

            path.push_back(parentPid); // push currentNode pid in the vector
            path.push_back(newSiblingPid); // push new sibling node pid in the vector
            ret = insertInParent(path, newSiblingKey);
        }
    } else if (parentPid > 0 && parentPid == rootPid) { // else, insert into current node's parent, which is a root node
        BTNonLeafNode parentNode;
        ret = parentNode.read(parentPid, pf);
        if (ret != 0) { return ret; } // RC_FILE_READ_FAILED;
        
        if (parentNode.getKeyCount() < N_KEY) { // if there is space in the parent node
            parentNode.insert(siblingKey, siblingPid);
            ret = parentNode.write(parentPid, pf);
            if (ret != 0) { return ret; } // RC_FILE_WRITE_FAILED;
        } else { // no space in the parent node -- split
            PageId newSiblingPid = pf.endPid();
            BTNonLeafNode newSiblingNode;
            int newSiblingKey;

            parentNode.insertAndSplit(siblingKey, siblingPid, newSiblingNode, newSiblingKey); // no space in the parent node

            ret = newSiblingNode.write(newSiblingPid, pf);
            if (ret != 0) { return ret; } // RC_FILE_WRITE_FAILED;
            ret = parentNode.write(parentPid, pf);
            if (ret != 0) { return ret; } // RC_FILE_WRITE_FAILED;

            PageId newRootPid = pf.endPid();
            BTNonLeafNode rootNode;
            rootNode.initializeRoot(parentPid, newSiblingKey, newSiblingPid);

            ret = rootNode.write(newRootPid, pf);
            if (ret != 0) { return ret; } // RC_FILE_WRITE_FAILED;

            // update the root node info
            rootPid = newRootPid;
            treeHeight++;
        }

    } else if (currentPid == rootPid) { // if current node is already the root node
        BTNonLeafNode currentNode;  // current root node must be a nonleaf node now
        currentNode.read(currentPid, pf);

        if (currentNode.getKeyCount() >= N_KEY) {
            PageId newRootPid = pf.endPid();
            BTNonLeafNode rootNode;
            rootNode.initializeRoot(currentPid, siblingKey, siblingPid);
            
            ret = rootNode.write(newRootPid, pf);
            if (ret != 0) { return ret; } // RC_FILE_WRITE_FAILED;

            // update the root node info
            rootPid = newRootPid;
            treeHeight++;
        } else {
            currentNode.insert(siblingKey, siblingPid);
            ret = currentNode.write(currentPid, pf);
            if (ret != 0) { return ret; } // RC_FILE_WRITE_FAILED;
        }
        return ret;
    }

    return ret;
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid) // following the book algorithm
{
    RC ret = 0;
    // clear the existing path chain
    path.erase(path.begin(), path.end());
    if (DEBUG) printf("BTreeIndex::insert() - path initialized\n");

    // if (tree is empty)
    // create an empty leaf node, which is also the root
    if (treeHeight == 0) {
        if (DEBUG) printf("BTreeIndex::insert() - empty tree\n");
        BTLeafNode root;
        ret = root.insert(key, rid);
        if (DEBUG) printf("BTreeIndex::insert() - insert into root node\n");
        if (ret != 0) {
            return ret;
        }

        rootPid = pf.endPid();
        if (DEBUG) printf("BTreeIndex::insert() - pf.endPid() = %d\n", rootPid);
        // make sure minimal rootPid value is 1 since tree now initialized
        rootPid = rootPid > 0 ? rootPid : 1;
        treeHeight++;

        ret = root.write(rootPid, pf);
        if (ret != 0) {
            return ret;
        }

        if (DEBUG) printf("BTreeIndex::insert() - root node written back to pf where rootPid = %d\n", rootPid);

        if (DEBUG) printf("BTreeIndex::insert() - and now the root node is:\n");
        if (DEBUG) root.printKeys();
        return ret;
    }

    IndexCursor targetIdx;
    locate(key, targetIdx, true);

    PageId targetPid = targetIdx.pid;
    if (DEBUG) printf("BTreeIndex::insert() - located target pid = %d\n", targetPid);
    BTLeafNode targetLeafNode;
    if (DEBUG) printf("BTreeIndex::insert() - target leaf node just created (should be empty):\n");
    if (DEBUG) targetLeafNode.printKeys();
    targetLeafNode.read(targetPid, pf);
    if (DEBUG) printf("BTreeIndex::insert() - target leaf node read:\n");
    if (DEBUG) targetLeafNode.printKeys();

    // check if targetLeafNode is full of keys
    if (targetLeafNode.getKeyCount() < N_KEY) { // targetLeafNode is not full - insert
        targetLeafNode.insert(key, rid);
        if (DEBUG) printf("BTreeIndex::insert() - targetLeafNode is not full, insert\n");

        if (DEBUG) printf("BTreeIndex::insert() - now after insert, the target leaf node is:\n");
        if (DEBUG) targetLeafNode.printKeys();
        ret = targetLeafNode.write(targetPid, pf); // write back the target leaf node!!
        if (ret != 0) {
            return ret; // RC_FILE_WRITE_FAILED;
        }

    } else {  // no space in the targetLeafNode - split
        PageId siblingPid = pf.endPid();
        BTLeafNode siblingLeafNode;
        int siblingKey;
        if (DEBUG) printf("BTreeIndex::insert() - no space, sibling pid = %d and it is empty now\n", siblingPid);
    
        // push targetLeafNode pid
        path.push_back(targetPid);
        // push sibling node pid
        path.push_back(siblingPid);

        if (DEBUG) printf("BTreeIndex::insert() - pushed targetPid and siblingPid into the path\n");

        // targetLeafNode is full - insert and split
        targetLeafNode.insertAndSplit(key, rid, siblingLeafNode, siblingKey);

        if (DEBUG) printf("BTreeIndex::insert() - after split, target leaf node is:\n");
        if (DEBUG) targetLeafNode.printKeys();
        if (DEBUG) printf("BTreeIndex::insert() - after split, sibling leaf node is:\n");
        if (DEBUG) siblingLeafNode.printKeys();

        siblingLeafNode.setNextNodePtr(targetLeafNode.getNextNodePtr());
        targetLeafNode.setNextNodePtr(siblingPid);

        if (DEBUG) printf("BTreeIndex::insert() - now insert and split, get siblingKey = %d\n", siblingKey);
        ret = siblingLeafNode.write(siblingPid, pf);
        if (ret != 0) {
            return ret; // RC_FILE_WRITE_FAILED;
        }
        ret = targetLeafNode.write(targetPid, pf);
        if (ret != 0) {
            return ret; // RC_FILE_WRITE_FAILED;
        }

        if (DEBUG) printf("BTreeIndex::insert() - after split, next ptr of target leafnode is %d\n", targetLeafNode.getNextNodePtr());
        if (DEBUG) printf("BTreeIndex::insert() - now insert siblingKey = %d into its parent\n", siblingKey);

        if (treeHeight == 1) { // if current leafnode is also the root node, we need to create a new root node, and increment the tree height
            PageId newRootPid = pf.endPid();
            BTNonLeafNode newRootNode;
            newRootNode.initializeRoot(targetPid, siblingKey, siblingPid);

            ret = newRootNode.write(newRootPid, pf);
            if (ret != 0) { return ret; } // RC_FILE_WRITE_FAILED;
            rootPid = newRootPid;
            treeHeight++;
        } else { // current leafnode is not the root node, we need to insert into its parent
            ret = insertInParent(path, siblingKey);
        }
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
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor, bool isTracking)
{
    RC ret = 0;
    // clear the existing path chain
    path.erase(path.begin(), path.end());
    if (DEBUG) printf("BTreeIndex::locate() - path initialized\n");
    // we read the node at the page id 'pid' in the pagefile into a leaf or nonleaf node

    int currentLevel = 1;
    assert(treeHeight >= 0);

    if (DEBUG) printf("BTreeIndex::locate() - treeHeight = %d\n", treeHeight);
    // this one can be wrong if for the second index file, pid > 0!!
    // assert(rootPid > 0);
    
    PageId currentPid = rootPid;
    BTNonLeafNode currentNonLeafNode;
    
    while (currentLevel < treeHeight) {
        // we are not at the leaf level yet
        ret = currentNonLeafNode.read(currentPid, pf);
        if (DEBUG) printf("BTreeIndex::locate() - current level = %d, non-leaf node read:\n", currentLevel);
        if (DEBUG) currentNonLeafNode.printKeys();
        if (isTracking) {
            path.push_back(currentPid);
            if (DEBUG) printf("BTreeIndex::locate() - we are tracking, current pid = %d\n", currentPid);
        }
        ret = currentNonLeafNode.locateChildPtr(searchKey, currentPid);
        if (DEBUG) printf("BTreeIndex::locate() - located child ptr pid = %d\n", currentPid);
        if (ret != 0) {
            return ret; // RC_NO_SUCH_RECORD;
        }
        currentLevel++;
    }

    BTLeafNode currentNode;
    ret = currentNode.read(currentPid, pf);
    if (DEBUG) printf("BTreeIndex::locate() - reached leaf level, leaf pid = %d, and it is:\n", currentPid);
    if (DEBUG) currentNode.printKeys();
    if (ret != 0) {
        return ret; // RC_FILE_READ_FAILED;
    }

    cursor.pid = currentPid;
    ret = currentNode.locate(searchKey, cursor.eid);
    if (DEBUG) printf("BTreeIndex::locate() - we are tracking, leaf eid = %d\n", cursor.eid);
    if (ret != 0) {
        return ret; // RC_NO_SUCH_RECORD;
    }

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
    RC ret = 0;

    PageId currentPid = cursor.pid;
    int currentEid = cursor.eid;
    if (DEBUG) printf("BTreeIndex::readForward() - current pid = %d, eid = %d\n", currentPid, currentEid);

    BTLeafNode currentNode;
    ret = currentNode.read(cursor.pid, pf);
    if (ret != 0) {
        return ret; // RC_FILE_READ_FAILED;
    }

    ret = currentNode.readEntry(currentEid, key, rid);
    if (ret != 0) {
        return ret; // RC_INVALID_CURSOR;
    }
    if (DEBUG) printf("BTreeIndex::readForward() - the entry read at current node: key = %d, rid.pid = %d, rid.sid = %d\n", key, rid.pid, rid.sid);

    // update eid, but check whether it will overflow first
    if (currentEid == currentNode.getKeyCount() - 1) { // eid points to next node
        cursor.eid = 0;
        cursor.pid = currentNode.getNextNodePtr();
        if (DEBUG) printf("BTreeIndex::readForward() - getKeyCount() = %d, eid points to next node and now cursor.pid = %d, cursor.eid = %d\n", currentNode.getKeyCount(), cursor.pid, cursor.eid);

    } else { // no overflow issue
        cursor.eid = ++currentEid;
        cursor.pid = currentPid;
        if (DEBUG) printf("BTreeIndex::readForward() - no overflow issue, now cursor.pid = %d, cursor.eid = %d\n", cursor.pid, cursor.eid);
    }

    return ret;
}

RC BTreeIndex::BTreeInfo() {
    printf("BTreeIndex::BTreeInfo - rootPid: %d\n", rootPid);
    printf("BTreeIndex::BTreeInfo - treeHeight: %d\n", treeHeight);
    return 0;
}
