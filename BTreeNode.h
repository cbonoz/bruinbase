/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 5/28/2008
 */

#ifndef BTREENODE_H
#define BTREENODE_H

#include "RecordFile.h"
#include "PageFile.h"

#include <stdio.h>
#include <stdlib.h>

#define NONE -1

// #define RID_SIZE sizeof(RecordId)
// #define K_SIZE sizeof(int)
// #define V_SIZE sizeof(int)
// #define PID_SIZE sizeof(PageId)
// #define P_SIZE PageFile::PAGE_SIZE

// #define N_NL (int) P_SIZE / (K_SIZE + PID_SIZE) + 1
// #define N_L (int) (P_SIZE - PID_SIZE) / (RID_SIZE+K_SIZE) + 1

// #define L_OFFSET RID_SIZE+K_SIZE
// #define NL_OFFSET PID_SIZE+K_SIZE

#define RID_SIZE 8
#define K_SIZE 4
#define V_SIZE 4
#define PID_SIZE 4
#define P_SIZE 1024

#define N_L 86
#define N_NL 129

#define L_OFFSET 12
#define NL_OFFSET 8


#define EC -100 //ERROR CODE


/**
 * BTLeafNode: The class representing a B+tree leaf node.
 */
class BTLeafNode {
  public:

    BTLeafNode();

   /**
    * Insert the (key, rid) pair to the node.
    * Remember that all keys inside a B+tree node should be kept sorted.
    * @param key[IN] the key to insert
    * @param rid[IN] the RecordId to insert
    * @return 0 if successful. Return an error code if the node is full.
    */
    RC insert(int key, const RecordId& rid);

   /**
    * Insert the (key, rid) pair to the node
    * and split the node half and half with sibling.
    * The first key of the sibling node is returned in siblingKey.
    * Remember that all keys inside a B+tree node should be kept sorted.
    * @param key[IN] the key to insert.
    * @param rid[IN] the RecordId to insert.
    * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
    * @param siblingKey[OUT] the first key in the sibling node after split.
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC insertAndSplit(int key, const RecordId& rid, BTLeafNode& sibling, int& siblingKey);

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
    RC locate(int searchKey, int& eid);

   /**
    * Read the (key, rid) pair from the eid entry.
    * @param eid[IN] the entry number to read the (key, rid) pair from
    * @param key[OUT] the key from the slot
    * @param rid[OUT] the RecordId from the slot
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC readEntry(int eid, int& key, RecordId& rid);

   /**
    * Return the pid of the next slibling node.
    * @return the PageId of the next sibling node 
    */
    PageId getNextNodePtr();


   /**
    * Set the next slibling node PageId.
    * @param pid[IN] the PageId of the next sibling node 
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC setNextNodePtr(PageId pid);

   /**
    * Return the number of keys stored in the node.
    * @return the number of keys in the node
    */
    int getKeyCount();
 
   /**
    * Read the content of the node from the page pid in the PageFile pf.
    * @param pid[IN] the PageId to read
    * @param pf[IN] PageFile to read from
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC read(PageId pid, const PageFile& pf);
    
   /**
    * Write the content of the node to the page pid in the PageFile pf.
    * @param pid[IN] the PageId to write to
    * @param pf[IN] PageFile to write to
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC write(PageId pid, PageFile& pf);

    void shiftKeysRight(int pos) {
        //shift element right by one
        char *loc = buffer+pos*L_OFFSET;
        // char *loc = buffer+pos*12;

        char* temp = (char*)malloc(P_SIZE);
        memset(temp, NONE, P_SIZE);

        memcpy(temp,loc,(getKeyCount()-pos)*L_OFFSET);
        loc += L_OFFSET;
        memcpy(loc, temp, (getKeyCount()-pos)*L_OFFSET);
        // memcpy(temp,loc,(getKeyCount()-pos)*12);
        // loc += 12;
        // memcpy(loc, temp, (getKeyCount()-pos)*12);
        free(temp);
        // memcpy(loc+L_OFFSET,loc,(getKeyCount()-pos)*L_OFFSET);
    }

    void printKeys();
    //  {
    //     char *kstart = buffer+RID_SIZE;
    //     int kc=getKeyCount();
    //     printf("Printing keys for Leaf Node\n");
    //     for (int i =0; i < kc;i++) {
    //         printf("%d ", *((int *) kstart));
    //         kstart+=L_OFFSET;
    //     }
    // }

    void initBuffer(char *b, size_t n) {
        memset(buffer,NONE,P_SIZE);
        memcpy(buffer,b,n);
    }

   

  private:
   /**
    * The main memory buffer for loading the content of the disk page 
    * that contains the node.
    */
    char buffer[PageFile::PAGE_SIZE];
}; 


/**
 * BTNonLeafNode: The class representing a B+tree nonleaf node.
 */
class BTNonLeafNode {
  public:

    BTNonLeafNode();

   /**
    * Insert a (key, pid) pair to the node.
    * Remember that all keys inside a B+tree node should be kept sorted.
    * @param key[IN] the key to insert
    * @param pid[IN] the PageId to insert
    * @return 0 if successful. Return an error code if the node is full.
    */
    RC insert(int key, PageId pid);

   /**
    * Insert the (key, pid) pair to the node
    * and split the node half and half with sibling.
    * The sibling node MUST be empty when this function is called.
    * The middle key after the split is returned in midKey.
    * Remember that all keys inside a B+tree node should be kept sorted.
    * @param key[IN] the key to insert
    * @param pid[IN] the PageId to insert
    * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
    * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey);

   /**
    * Given the searchKey, find the child-node pointer to follow and
    * output it in pid.
    * Remember that the keys inside a B+tree node are sorted.
    * @param searchKey[IN] the searchKey that is being looked up.
    * @param pid[OUT] the pointer to the child node to follow.
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC locateChildPtr(int searchKey, PageId& pid);

   /**
    * Initialize the root node with (pid1, key, pid2).
    * @param pid1[IN] the first PageId to insert
    * @param key[IN] the key that should be inserted between the two PageIds
    * @param pid2[IN] the PageId to insert behind the key
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC initializeRoot(PageId pid1, int key, PageId pid2);

   /**
    * Return the number of keys stored in the node.
    * @return the number of keys in the node
    */
    int getKeyCount();

   /**
    * Read the content of the node from the page pid in the PageFile pf.
    * @param pid[IN] the PageId to read
    * @param pf[IN] PageFile to read from
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC read(PageId pid, const PageFile& pf);
    
   /**
    * Write the content of the node to the page pid in the PageFile pf.
    * @param pid[IN] the PageId to write to
    * @param pf[IN] PageFile to write to
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC write(PageId pid, PageFile& pf);

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
    RC locate(int searchKey, int &eid);

    /*
     * Read the (key, pid) pair from the eid entry.
     * @param eid[IN] the entry number to read the (key, rid) pair from
     * @param key[OUT] the key from the entry
     * @param pid[OUT] the PageId from the entry
     * @return 0 if successful. Return an error code if there is an error.
     */
    RC readEntry(int eid, int& key, PageId& pid);

    void shiftKeysRight(int pos) {
        //shift element right by one
        // char *loc = buffer+pos*NL_OFFSET;
        char *loc = buffer+pos*8;

        char* temp = (char*)malloc(P_SIZE);
        memset(temp, NONE, P_SIZE);

        // memcpy(temp,loc,(getKeyCount()-pos)*NL_OFFSET);
        // loc += NL_OFFSET;
        // memcpy(loc, temp, (getKeyCount()-pos)*NL_OFFSET);
        memcpy(temp,loc,(getKeyCount()+1-pos)*8);
        loc += 8;
        memcpy(loc, temp, (getKeyCount()+1-pos)*8);
        free(temp);
        // memcpy(loc+NL_OFFSET,loc,(getKeyCount()-pos)*NL_OFFSET);
    }


    void printKeys();
    // {
    //     char *kstart = buffer+PID_SIZE;
    //     int kc=getKeyCount();
    //     printf("Printing keys for NoNLeaf Node\n");
    //     for (int i =0; i < kc;i++) {
    //         printf("%d ", *((int *) kstart));
    //         kstart+=NL_OFFSET;
    //     }
    // }

    void initBuffer(char *b, size_t n) {
        memset(buffer,NONE,P_SIZE);
        // memset(buffer,0,P_SIZE);
        memcpy(buffer,b,n);
    }

  private:
   /**
    * The main memory buffer for loading the content of the disk page 
    * that contains the node.
    */
    char buffer[PageFile::PAGE_SIZE];
}; 

#endif /* BTREENODE_H */
