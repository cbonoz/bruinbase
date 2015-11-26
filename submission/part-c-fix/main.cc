/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "Bruinbase.h"
#include "SqlEngine.h"
#include <cstdio>


int main()
{
  // run the SQL engine taking user commands from standard input (console).
  SqlEngine::run(stdin);

  return 0;
}

// #include "Bruinbase.h"
// #include "PageFile.h"
// #include "BTreeIndex.h"
// #include <string>
// #include <iostream>
// #include <algorithm>    // std::random_shuffle
// #include <vector>       // std::vector
// #include <ctime>        // std::time
// #include <cstdlib>      // std::rand, std::srand
// using namespace std;

// void testSaveAndOpen() {
//     BTreeIndex index;
//     index.open("abc.tbl", 'w');

//     IndexCursor cursor;
//     RC search_status;

//     index.insertKey(1);
//     index.insertKey(3);
//     index.insertKey(5);
//     index.insertKey(2);
//     index.insertKey(6);
//     index.insertKey(4);
//     index.insertKey(8);
//     index.insertKey(7);
//     index.insertKey(12);
//     index.insertKey(30);
// 	search_status = index.locate(100, cursor);
//     search_status = index.locate(300, cursor);
//     search_status = index.locate(500, cursor);
//     search_status = index.locate(800, cursor);
//     search_status = index.locate(1500, cursor);
//     index.insertKey(15);
//     index.insertKey(14);
//     search_status = index.locate(1400, cursor);
//     search_status = index.locate(1600, cursor);

//     index.insertKey(10);
//     index.insertKey(9);
//     index.insertKey(11);
//     index.insertKey(13);
//     index.insertKey(16);
//     index.insertKey(19);
//     index.insertKey(17);
//     index.insertKey(18);
//     index.insertKey(20);
//     for (int i = 21; i < 320; i++) {
// 	    index.insertKey(i);
//     }

//     printf("\n\n\ntest(): INSERTION COMPLETE, TEST LOCATE\n");
//     search_status = index.locate(0, cursor);
//     search_status = index.locate(100, cursor);
//     search_status = index.locate(900, cursor);
//     search_status = index.locate(1500, cursor);
//     search_status = index.locate(2800, cursor);
//     search_status = index.locate(8500, cursor);


//     printf("\n\n\ntest(): SPECIAL ATTEN\n");
//     search_status = index.locate(1500, cursor);

//     printf("\n\n\ntest(): we are done inserting. Print tree info now:\n");
//     index.BTreeInfo();
//     printf("test(): close the tree now:\n");
//     index.close();

//     BTreeIndex index1;
//     printf("\n\ntest(): hello, we open you again, but in read mode\n");
//     index1.open("abc.tbl",'r');
//     printf("test(): so we want to retrive tree info again\n");
//     index1.BTreeInfo();

//     printf("\n\ntest(): we are locating key = 0 now\n");

//     search_status = index1.locate(0, cursor);
//     //attempt to read the values we entered
//     for (int i = 1; i <= 20; i++) {   
//         printf("test(): Cursor at pageId = %d, eid = %d \n", cursor.pid, cursor.eid);
//         int key;
//         RecordId rid;
//         // key = i;
//         RC read_status = index1.readForward(cursor, key, rid);
//         printf("test() readForward for i = %d, key = %d record pid = %d, record sid = %d.\n\n", i, key, rid.pid, rid.sid);
//     }
//     index1.BTreeInfo();
//     index1.close();
//     //return 0;
// }
// int main() {
//    // cout<<random(10)<<endl;
//     testSaveAndOpen();
//    // testRandomOrder();
//    // testInOrder();
//    // testReverseOrder();

//     // Test on small index


//     return 0;
// }
