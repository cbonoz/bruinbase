#include "BTreeNode.h"
/*
We have these available as well

#include "RecordFile.h"
#include "PageFile.h"
*/

using namespace std;

BTLeafNode::BTLeafNode()
{
	memset(buffer, NONE, PageFile::PAGE_SIZE);
}

void BTLeafNode::printKeys()
{
	// printf("The # of keys in a leaf node is %lu\n", N_L);
	// printf("The # of keys in a non-leaf node is %lu\n", N_NL);
	// 	printf("The recordID size RID_SIZE is %d\n", RID_SIZE);
	// 	printf("The pageID size PID_SIZE is %d\n", PID_SIZE);
	// 	printf("The key size K_SIZE is %d\n", K_SIZE);
	// 	printf("The page size P_SIZE is %d\n", P_SIZE);
	// 	printf("The offset of leaf node L_OFFSET is %d\n", L_OFFSET );
	// 	printf("The offset of non-leaf node NL_OFFSET is %d\n", NL_OFFSET );

// 	The recordID size RID_SIZE is 8
// The pageID size PID_SIZE is 4
// The key size K_SIZE is 4
// The page size P_SIZE is 1024
// The offset of leaf node L_OFFSET is 12
// The offset of non-leaf node NL_OFFSET is 8

	char *kstart;
	kstart = buffer;
	kstart += 8;
	int kc = getKeyCount();
	int * key;
	printf("BTLeafNode::printKeys - Printing keys for Leaf Node\n");
	for (int i = 0; i < kc; i++) {

// memcpy(key, kstart+RID_SIZE, K_SIZE);
// memcpy(key, kstart, 4);
//    printf("%d ", *(key));
	    printf("%d ", *((int *) kstart));
	    kstart+=12;
	}

	printf("\n");
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
	char *kstart=buffer+RID_SIZE;
	char *end = buffer+P_SIZE;

	int curKey;
	int i=0;
	while(kstart < end) {
		curKey=*(kstart);

		if (curKey==NONE) 
			return i;

		kstart+=L_OFFSET;
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
	//find key location
	BTLeafNode::locate(key, pos);

	printf("located key pos = %d\n", pos);

	//find copy location
	char *loc;
	loc = buffer;
	int offset = pos*12; // pos * L_OFFSET// maybe because L_OFFSET is unsigned long
	loc += offset; // that's the problem !! oh offset is 4
	// printf("oh offset is %d\n", offset); // oh offset is 4

	//shift pairs from pos one space to the right
	BTLeafNode::shiftKeysRight(pos);

	//copy in inserted pair
	printf("copy <rid = (%d,%d)> in inserted pair\n", rid.pid, rid.sid);
	memcpy(loc, &rid, RID_SIZE);
	loc += RID_SIZE;
	printf("copy <key = %d> in inserted pair\n", key);
	memcpy(loc, &key, K_SIZE);
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
	//number of keys
	// int N = N_L-1;
	int N = 85;
	//mid_key is the position to split
	int mid_key = N/2;

	int pos;
	BTLeafNode::locate(key, pos);
	
	//pointer to start of right half
	
	int num_copy = N-mid_key;

	if (pos>mid_key) {
		num_copy--;
	}


	// char *sib_start = buffer+P_SIZE-(num_copy*L_OFFSET);
	char *sib_start = buffer+P_SIZE-PID_SIZE-(num_copy*12);


	sibling.initBuffer(sib_start,num_copy*12);
	memset(sib_start, NONE, num_copy*12);

	if (pos>mid_key) {
		//insert in sibling
		sibling.insert(key,rid);
	} else {
		//insert in current node
		insert(key, rid);
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
 * @return 0 if searchKey is found. Otherwise return an error code.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{
	char *kstart=buffer+RID_SIZE;
	char *end = buffer+P_SIZE;

	int curKey;
	int i=0;
	for (int iter = 0; iter < getKeyCount(); iter++) {
	// wrong b/c it can go over the whole node if the node is empty
	// while(kstart < end) {
		curKey=*((int *) kstart);
		// printf("BTLeafNode::locate - currentKey = %d\n", curKey);

		if (curKey==searchKey) {

			eid=i;
		printf("Match! eid = %d\n", eid);
			return 0;
		}
		
		if (curKey > searchKey) {
			eid=max(0,i-1);

		printf("NO Match... eid = %d\n", eid);
			return RC_NO_SUCH_RECORD;
		}

		kstart+=L_OFFSET;
		i++;
	}
	// TODO it can has problem!!
		eid=i;// we've forgot this
		printf("NO Match... eid = %d\n", eid);
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
	char *entryStart;
	entryStart = buffer + (eid * 12);
	// entryStart+=(L_OFFSET*eid); // seems not work

	// printf("BTLeafNode::readEntry -- eid = %d\n", eid);

	memcpy(&rid, entryStart, RID_SIZE);
	// printf("BTLeafNode::readEntry -- rid = (%d,%d)\n", rid.pid, rid.sid);
	entryStart+=RID_SIZE;
	memcpy(&key, entryStart, K_SIZE);
	// printf("BTLeafNode::readEntry -- key = %d\n", key);

	return 0;
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{
	PageId pid;
	char* ptr = buffer+1020;
	memcpy(&pid, ptr, sizeof(PageId));
	// return *((PageId *) buffer+P_SIZE-PID_SIZE);
	return pid;
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{
	// *((PageId *) buffer+P_SIZE-PID_SIZE)=pid;
	// char* ptr = buffer+P_SIZE-PID_SIZE;
	char* ptr = buffer+1020;
	memcpy(ptr, &pid, sizeof(PageId));
	return 0;
}

BTNonLeafNode::BTNonLeafNode()
{
	memset(buffer, NONE, PageFile::PAGE_SIZE);
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
	char *kstart=buffer+PID_SIZE;
	char *end = buffer+P_SIZE;

	int curKey;
	int i=0;
	while(kstart < end) {
		curKey=*((int *) kstart);

		if (curKey==NONE) 
			return i;
		
		kstart+=NL_OFFSET;//integer pointer artihmetric cast to byte
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
	/*
	int kc=getKeyCount();
	if (kc==N_NL-1) {
		return insertAndSplit(key,pid);
	} else if (kc>=N_NL || kc<0) {
		return EC;
	}
	*/

	//find key location
	locate(key,pos);
	//find copy location
	char *loc = buffer+pos*NL_OFFSET;
	//shift pairs from pos one space to the right
	shiftKeysRight(pos);
	//copy in inserted pair
	memcpy(loc, &pid, PID_SIZE);
	memcpy(loc+PID_SIZE, &key, K_SIZE);
	
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
	//number of keys
	int N = N_NL-1;
	//mid_key is the position to split
	int mid_key = N/2;

	int pos;
	locate(key, pos);
	
	//pointer to start of right half
	

	int num_copy = N-mid_key;

	if (pos>mid_key) {
		num_copy--;
	}

	char *sib_start = buffer+P_SIZE-(num_copy*NL_OFFSET);

	sibling.initBuffer(sib_start,num_copy*NL_OFFSET);

	if (pos>mid_key) {
		//insert in sibling
		sibling.insert(key,pid);
	} else {
		//insert in current node
		insert(key, pid);
	}
	
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
	int curKey;
	char *kstart=buffer+PID_SIZE;
	char *end = buffer+P_SIZE;

	while(kstart < end) {
		curKey=*((int *) kstart);

		if (curKey>searchKey) {
			pid=*((int *) (kstart-PID_SIZE));
			return 0;
		}
		kstart+=NL_OFFSET;//integer pointer artihmetric cast to byte
	}

	pid=*((int *) (kstart-PID_SIZE));
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
	char *kstart=buffer+PID_SIZE;
	char *end = buffer+P_SIZE;


	int curKey;
	int i=0;
	while(kstart < end) {
		curKey=*((int *) kstart);

		if (curKey==searchKey) {
			eid=i;
			return 0;
		}
		
		if (curKey > searchKey) {
			eid=i-1;
			return RC_NO_SUCH_RECORD;
		}

		kstart+=NL_OFFSET;
		i++;
	}

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
	char *entryStart=buffer+NL_OFFSET*eid;

	memcpy(&pid, entryStart, PID_SIZE);
	// printf("BTNonLeafNode::readEntry -- pid = %d\n", pid);
	entryStart+=PID_SIZE;
	memcpy(&key, entryStart, K_SIZE);
	// printf("BTNonLeafNode::readEntry -- key = %d\n", key);

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
	memset(buffer,NONE,P_SIZE);
	memcpy(start,&pid1, PID_SIZE);
	start+=PID_SIZE;
	memcpy(start,&key,K_SIZE);
	start+=K_SIZE;
	memcpy(start,&pid2,PID_SIZE);

	return 0;
}
