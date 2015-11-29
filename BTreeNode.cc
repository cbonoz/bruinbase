#include "BTreeNode.h"
/*
We have these available as well

#include "RecordFile.h"
#include "PageFile.h"
*/

using namespace std;

/**
* Constructor
*/
BTLeafNode::BTLeafNode()
{
	memset(buffer, NONE, PageFile::PAGE_SIZE);
}

/**
* Helper function
* Prints out the keys to the screen
*/
void BTLeafNode::printKeys()
{
	int key;
	RecordId rid;
	printf("=== Leaf Node: [rid(pid, sid), key] ===\n");
	for(int i = 0; i < getKeyCount(); i++) {
		readEntry(i, key, rid);
		printf("[(%d, %d), %d]\t", rid.pid, rid.sid, key);
	}
	printf("\n");

	// char *kstart = buffer + RID_SIZE;
	// printf("BTLeafNode::printKeys - Printing keys for Leaf Node\n");
	// for (int i = 0; i < getKeyCount(); i++) {
	//     printf("%d ", *((int *) kstart));
	//     kstart += L_OFFSET;
	// }
	// printf("\n");
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{
	return pf.read(pid,buffer);
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{
	return pf.write(pid,buffer);
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{
	char *kstart = buffer;
	char *end = buffer + (N_KEY * L_OFFSET) - K_SIZE;
	// int curRid;
	// int i = 0;

	// while(kstart < end) {
	// 	curRid = *(kstart);

	// 	if (curRid == NONE) 
	// 		return i;

	// 	kstart += L_OFFSET;
	// 	i++;
	// }

	// return i;
	int curRid = -1;
	int i = 0;

	while (kstart <= end) {
		curRid = *((int *) kstart);

		if(curRid == NONE) {
			return i;
		}
		
		kstart += L_OFFSET;
		i++;
	}

	return i;
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{
	int pos;
	// find key location
	BTLeafNode::locate(key, pos);
	// printf("BTLeafNode::locate - located key pos = %d\n", pos);

	// find copy location
	char *loc = buffer + pos * L_OFFSET;
	// maybe because L_OFFSET is unsigned long; oh offset is 4

	// shift pairs from pos one space to the right
	BTLeafNode::shiftKeysRight(pos);

	// copy in inserted pair
	// printf("copy <rid = (%d,%d)> in inserted pair\n", rid.pid, rid.sid);
	memcpy(loc, &rid, RID_SIZE);
	loc += RID_SIZE;
	// printf("copy <key = %d> in inserted pair\n", key);
	memcpy(loc, &key, K_SIZE);
	// printf("==BTreeNode:: the node being insert is now\n");
	// printKeys();
	return 0;
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{
	// number of keys (85)
	// int N = N_PTR - 1;
	// mid_key is the position to split
	int mid_key = N_KEY / 2;
	// printf("BTLeafNode::insertAndSplit - mid_key is %d\n", mid_key);

	int pos;
	BTLeafNode::locate(key, pos);
	
	// pointer to start of right half
	int num_copy = N_KEY - mid_key;
	// printf("BTLeafNode::insertAndSplit - we should copy %d keys\n", num_copy);

	if (pos > mid_key) {
		num_copy--;
		// printf("BTLeafNode::insertAndSplit - wait, copy 1 less key\n");
	}

	char *sib_start = buffer + ((N_KEY - num_copy) * L_OFFSET);

	sibling.initBuffer(sib_start, (num_copy * L_OFFSET));
	memset(sib_start, NONE, (num_copy * L_OFFSET));

	if (pos > mid_key) {
		// insert in sibling
		sibling.insert(key,rid);
	} else {
		// insert in current node
		insert(key, rid);
	}

	RecordId siblingRid;
	sibling.readEntry(0, siblingKey, siblingRid);

	return 0;
}

/**
 * If searchKey exists in the node, set eid to the index entry
 * with searchKey and return 0. If not, set eid to the index entry
 * immediately after the largest index key that is smaller than searchKey,
 * and return the error code RC_NO_SUCH_RECORD.
 * Remember that keys inside a B+tree node are always kept sorted.
 * @param searchKey[IN] the key to search for.
 * @param eid[OUT] the index entry number with searchKey or immediately
                   behind the largest key smaller than searchKey.
 * @return 0 if searchKey is found. Otherwise return an error code.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{
	char *kstart = buffer + RID_SIZE;
	int curKey;
	int i = 0;

	for (int iter = 0; iter < getKeyCount(); iter++) {
		curKey=*((int *) kstart);

		if (searchKey == curKey) {
			eid = i;
			// printf("Match! eid = %d\n", eid);
			return 0;
		}
		
		if (searchKey < curKey) {
			//the index entry immediately after the largest index key that is smaller than searchKey,
			eid = i;
			// printf("NO Match... eid = %d\n", eid);
			return RC_NO_SUCH_RECORD;
		}

		kstart += L_OFFSET;
		i++;
	}

	eid = i; // don't forget to set eid
	return EC;
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{
	char *entryStart = buffer + (eid * L_OFFSET);
	char *end = buffer + P_SIZE;

	if (buffer > end)
		return RC_INVALID_CURSOR;
	memcpy(&rid, entryStart, RID_SIZE);
	entryStart += RID_SIZE;
	memcpy(&key, entryStart, K_SIZE);

	return 0;
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{
	// PageId pid;
	// char* ptr = buffer+1020;
	// memcpy(&pid, ptr, sizeof(PageId));
	// return pid;
	return *((PageId *) (buffer + L_OFFSET * N_KEY));
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{
	// char* ptr = buffer+1020;// buffer+P_SIZE-PID_SIZE;
	// memcpy(ptr, &pid, sizeof(PageId));

	*((PageId *) (buffer + L_OFFSET * N_KEY)) = pid;
	return 0;
}

/**
* Constructor
*/
BTNonLeafNode::BTNonLeafNode()
{
	memset(buffer, NONE, PageFile::PAGE_SIZE);
}

void BTNonLeafNode::printKeys() {
	int key;
	PageId pid;
	printf("=== Non-leaf Node: [pid, key] ===\n");
	for(int i = 0; i < BTNonLeafNode::getKeyCount(); i++) {
		BTNonLeafNode::readEntry(i, key, pid);
			printf("[%d, %d]\t", pid, key);
	}
	printf("\n");

	// char *kstart = buffer+PID_SIZE;
	// int kc=getKeyCount();
	// printf("Printing keys for NoNLeaf Node\n");
	// for (int i = 0; i < kc+1; i++) {
	//     printf("%d ", *((int *) kstart));
	//     kstart+=NL_OFFSET;
	// }
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{
	return pf.read(pid,buffer);
}

/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{
	return pf.write(pid,buffer);
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{
	char *kstart = buffer + NL_OFFSET;
	char *end = buffer + (N_KEY * NL_OFFSET) + PID_SIZE;
	int curPid;
	int i = 0;

	while (kstart <= end) {
		curPid = *((int *) kstart);

		if(curPid == NONE) {
			return i;
		}
		
		kstart += NL_OFFSET;
		i++;
	}

	return i;
}

/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{
	int pos;
	// find key location
	BTNonLeafNode::locate(key, pos);
	// printf("BTNonLeafNode::insert() - located pos = %d\n", pos);
	// printf("BTNonLeafNode::insert() - CHECKPOINT getKeyCount = %d keys\n", getKeyCount());
	// printf("located key pos = %d\n", pos);
	// find copy location
	char *loc = buffer + PID_SIZE + pos * NL_OFFSET;

	// shift pairs from pos one space to the right
	BTNonLeafNode::shiftKeysRight(pos);

	// copy in inserted pair
	// printf("copy <pid = %d> in inserted pair\n", pid);
	memcpy(loc, &key, K_SIZE);
	loc += K_SIZE;
	memcpy(loc, &pid, PID_SIZE);

	return 0;
}

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{
	insert(key, pid);
	int mid_key = (N_KEY + 1) / 2;	// mid_key is the position to split
	char *loc = buffer + PID_SIZE + (mid_key * NL_OFFSET);
	memcpy(&midKey, loc, K_SIZE);
	char *sib_start = loc + K_SIZE;
	int num_copy = N_KEY - mid_key;
	sibling.initBuffer(sib_start, ((num_copy * NL_OFFSET) + PID_SIZE));
	memset(loc, NONE, ((num_copy + 1) * NL_OFFSET));

	return 0;
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{
	int idx;
	locate(searchKey, idx);

	if (idx >= getKeyCount()) {
		idx = getKeyCount() - 1;
	}

	char *keyLoc = buffer + PID_SIZE + (idx * NL_OFFSET);

	int curKey = *((int *) keyLoc);

	if (searchKey >= curKey) {
		pid = *((PageId *) (keyLoc + K_SIZE));
	}
	else {
		pid = *((PageId *) (keyLoc - PID_SIZE));
	}
	return 0;
}

/**
* If searchKey exists in the node, set eid to the index entry
* with searchKey and return 0. If not, set eid to the index entry
* immediately after the largest index key that is smaller than searchKey, 
* and return the error code RC_NO_SUCH_RECORD.
* Remember that keys inside a B+tree node are always kept sorted.
* @param searchKey[IN] the key to search for.
* @param eid[OUT] the index entry number with searchKey or immediately
                  behind the largest key smaller than searchKey.
* @return 0 if searchKey is found. If not, RC_NO_SEARCH_RECORD.
*/
RC BTNonLeafNode::locate(int searchKey, int &eid) {
	char *kstart = buffer + PID_SIZE;
	int curKey;
	int i = 0;

	// printf("BTNonLeafNode::locate - keyCount = %d\n", getKeyCount());
	for(int iter = 0; iter < getKeyCount(); iter++) {
		curKey = *((int *) kstart);

		// printf("BTNonLeafNode::locate - currentKey = %d\n", curKey);
		if (searchKey == curKey) {
			eid = i;
			// printf("Match! eid = %d\n", eid);
			return 0;
		}
		
		if (searchKey < curKey) {
			eid = i;
			// printf("NO Match... eid = %d\n", eid);
			return RC_NO_SUCH_RECORD;
		}

		kstart += NL_OFFSET;
		i++;
	}

	eid = i;

	return EC;
}

/*
 * Read the (key, pid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param pid[OUT] the PageId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::readEntry(int eid, int& key, PageId& pid)
{
	char *entryStart;
	entryStart = buffer + (eid * NL_OFFSET);

	memcpy(&pid, entryStart, PID_SIZE);
	entryStart += PID_SIZE;
	memcpy(&key, entryStart, K_SIZE);

	return 0;
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{
	char *start = buffer;
	memset(buffer, NONE, P_SIZE);
	memcpy(start, &pid1, PID_SIZE);
	start += PID_SIZE;
	memcpy(start, &key, K_SIZE);
	start += K_SIZE;
	memcpy(start, &pid2, PID_SIZE);

	return 0;
}