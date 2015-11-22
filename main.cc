/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
// #include "Bruinbase.h"
// #include "SqlEngine.h"
// #include <cstdio>


// int main()
// {
//   // run the SQL engine taking user commands from standard input (console).
//   SqlEngine::run(stdin);

//   return 0;
// }

#include "Bruinbase.h"
#include "PageFile.h"
#include "BTreeIndex.h"
#include <string>
#include <iostream>
#include <algorithm>    // std::random_shuffle
#include <vector>       // std::vector
#include <ctime>        // std::time
#include <cstdlib>      // std::rand, std::srand
using namespace std;

int NUM_NODES =300;

void testInOrder() {
    BTreeIndex index;
    index.open("abc.tbl", 'w');

    int numNodes = NUM_NODES;
    RC rc;
    //add 1-100 to our b+tree
    for (int i = 1; i <= numNodes; i++) {
        RecordId insertMe;
        insertMe.pid = i;
        insertMe.sid = i;

        int key_to_insert = i;
        printf("TreeTester:main() Inserting key: %d with record pid = %d, sid = %d\n", key_to_insert, insertMe.pid, insertMe.sid);
        rc = index.insert(key_to_insert, insertMe);
    }
    IndexCursor cursor;
    RC search_status = index.locate(1, cursor);
    //attempt to read the values we entered
    for (int j = 1; j <= numNodes; j++) {   
        printf("TreeTester:main()  Cursor contents pageId: %d eid: %d \n", cursor.pid, cursor.eid);
        int key;
        RecordId rid;
        RC read_status = index.readForward(cursor, key, rid);
        printf("TreeTester:main() readForward for j = %d, key = %d record pid = %d, record sid = %d.\n\n", j, key, rid.pid, rid.sid);
    }
    //return 0;
}

void testReverseOrder() {
    BTreeIndex index;
    index.open("cba.tbl", 'w');

    int numNodes = NUM_NODES;
    RC rc;
    //add 100-1 to our b+tree
    for (int i = numNodes; i > 0; i--) {
        RecordId insertMe;
        insertMe.pid = i;
        insertMe.sid = i;

        int key_to_insert = i;
        printf("TreeTester:main() Inserting key: %d with record pid = %d, sid = %d\n", key_to_insert, insertMe.pid, insertMe.sid);
        rc = index.insert(key_to_insert, insertMe);
    }
    IndexCursor cursor;
    RC search_status = index.locate(1, cursor);
    if (search_status) {
        printf("Error:%d\n", search_status);
    }
    //attempt to read the values we entered
    for (int j = 1; j <= numNodes; j++) {   
        printf("TreeTester:main()  Cursor contents pageId: %d eid: %d \n", cursor.pid, cursor.eid);
        int key;
        RecordId rid;
        RC read_status = index.readForward(cursor, key, rid);
        printf("TreeTester:main() readForward for j = %d, key = %d record pid = %d, record sid = %d.\n\n", j, key, rid.pid, rid.sid);
    }
    //return 0;
}
int myrandom (int i) { return std::rand()%i;}   
void testRandomOrder() {
    BTreeIndex index;
    RC rc;
    index.open("bca.tbl", 'w');
    int numNodes = NUM_NODES;
    //following code is borrowed from http://www.cplusplus.com/reference/algorithm/random_shuffle/
    // to generate 100 random unique integers for random key insertion
    srand ( unsigned ( std::time(0) ) );
    vector<int> myvector;
    for (int i=1; i<=numNodes; ++i) myvector.push_back(i); // 1 2 3 4 5 6 7 8 9
    random_shuffle ( myvector.begin(), myvector.end() );
    random_shuffle ( myvector.begin(), myvector.end(), myrandom);
    cout << "Random Keys are:";
    int i;
    for (std::vector<int>::iterator it=myvector.begin(); it!=myvector.end(); ++it)
    {
        std::cout << ' ' << *it;
    }
    std::cout << '\n';  
    //end of borrowed code
    for (std::vector<int>::iterator it=myvector.begin(); it!=myvector.end(); ++it)
    {
        i=*it;
        RecordId insertMe;
        insertMe.pid = i;
        insertMe.sid = i;
        int key_to_insert = i;
        printf("TreeTester:main() Inserting key: %d with record pid = %d, sid = %d\n", key_to_insert, insertMe.pid, insertMe.sid);
        rc = index.insert(key_to_insert, insertMe);
    }
          
    IndexCursor cursor;
    RC search_status = index.locate(1, cursor);
    if (search_status) {
        printf("Error:%d\n", search_status);
    }
    //attempt to read the values we entered
    for (int j = 1; j <= numNodes; j++) {   
        printf("TreeTester:main()  Cursor contents pageId: %d eid: %d \n", cursor.pid, cursor.eid);
        int key;
        RecordId rid;
        RC read_status = index.readForward(cursor, key, rid);
        printf("TreeTester:main() readForward for j = %d, key = %d record pid = %d, record sid = %d.\n\n", j, key, rid.pid, rid.sid);
    }
    //return 0;*/
}
void testSaveAndOpen() {
    BTreeIndex index;
    index.open("abc.tbl", 'w');

    int numNodes = NUM_NODES;
    RC rc;
    //add 1-100 to our b+tree
    for (int i = 25; i <= numNodes; i++) {
        RecordId insertMe;
        insertMe.pid = i;
        insertMe.sid = i;

        int key_to_insert = i;
        printf("TreeTester:main() Inserting key: %d with record pid = %d, sid = %d\n", key_to_insert, insertMe.pid, insertMe.sid);
        rc = index.insert(key_to_insert, insertMe);
    }
    index.BTreeInfo();
    index.close();
    BTreeIndex index1;
    index1.open("abc.tbl",'r');
    index1.BTreeInfo();
    IndexCursor cursor;
    RC search_status = index1.locate(0, cursor);
    //attempt to read the values we entered
    for (int j = 1; j <= numNodes; j++) {   
        printf("TreeTester:main()  Cursor contents pageId: %d eid: %d \n", cursor.pid, cursor.eid);
        int key;
        RecordId rid;
        RC read_status = index1.readForward(cursor, key, rid);
        printf("TreeTester:main() readForward for j = %d, key = %d record pid = %d, record sid = %d.\n\n", j, key, rid.pid, rid.sid);
    }
    //return 0;
}
int main() {
   // cout<<random(10)<<endl;
    testSaveAndOpen();
   // testRandomOrder();
   // testInOrder();
   // testReverseOrder();

    // Test on small index


    return 0;
}


