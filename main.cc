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

#include "PageFile.h"
#include "BTreeNode.h"
#include <iostream>
using namespace std;


void printnode(BTLeafNode node) {
  int key;
  RecordId rid;
  cout<<"[rid(pid, sid), key]"<<endl;
  for(int i=0; i<node.getKeyCount(); i++) {
	  node.readEntry(i, key, rid);
	  cout<<"[("<<rid.pid<<", "<<rid.sid<<"), "<<key<<"]\t";
  }
  cout<<endl<<"---------"<<endl;
  return;
}


int main()
{
  // BTLeafNode leafnode;

  // cout<<"Testing leaf node:"<<endl;
  // cout<<"===after initialized node, print node==="<<endl;
  // printnode(leafnode);
  // leafnode.printKeys();
  // cout<<"currently there are getKeyCount() = " <<leafnode.getKeyCount()<<" keys"<<endl;
  // cout<<"-------------"<<endl;


  // cout<<"set a rid at {1,2}"<<endl;
  // RecordId rid = {1,2};
  // cout<<"Inserting (3, rid) pair to the node."<<endl;
  // leafnode.insert(3, rid);
  // cout<<"currently there are getKeyCount() = " <<leafnode.getKeyCount()<<" keys"<<endl;
  // leafnode.printKeys();printnode(leafnode);
  // cout<<"Inserting (2, rid) pair to the node."<<endl;
  // leafnode.insert(2, rid);
  // cout<<"currently there are getKeyCount() = " <<leafnode.getKeyCount()<<" keys"<<endl;
  // leafnode.printKeys();printnode(leafnode);
  // cout<<"Inserting (4, rid) pair to the node."<<endl;
  // leafnode.insert(4, rid);
  // cout<<"currently there are getKeyCount() = " <<leafnode.getKeyCount()<<" keys"<<endl;
  // leafnode.printKeys();printnode(leafnode);
  // cout<<"Inserting (1, rid) pair to the node."<<endl;
  // leafnode.insert(1, rid);
  // cout<<"currently there are getKeyCount() = " <<leafnode.getKeyCount()<<" keys"<<endl;
  // leafnode.printKeys();printnode(leafnode);

  // cout<<"===set next node ptr to 123==="<<endl;
  // leafnode.setNextNodePtr(123);
  // cout<<"Next node ptr is: "<<leafnode.getNextNodePtr()<<endl;
  
  // for(int i=6; i<N_L+1; i++) {
  // 	leafnode.insert(i, rid);
  // }
  // leafnode.printKeys();printnode(leafnode);

  // BTLeafNode sibling;
  // int siblingkey;
  // cout<<"*Inserting (5, rid) pair to split the node."<<endl;
  // leafnode.insertAndSplit(5, rid, sibling, siblingkey);

  // cout<<"===After split, the leaf node has==="<<endl;
  // printnode(leafnode);
  // cout<<"===After split, the sibling node has==="<<endl;
  // printnode(sibling);
  // cout<<"*Sibling Key = "<<siblingkey<<endl;



  // // BTLeafNode fullleafnode;
  // // for(int i=0; i<N_L; i++) {
  // //   fullleafnode.insert(i*10, rid);
  // // }
  // // fullleafnode.printKeys();printnode(fullleafnode);
  // // cout<<"===set next node ptr to 888==="<<endl;
  // // fullleafnode.setNextNodePtr(888);
  // // cout<<"Full node's next node ptr is: "<<fullleafnode.getNextNodePtr()<<endl;
  // // PageFile pf;
  // // pf.open("test.pf", 'w');
  // // fullleafnode.write(pf.endPid(), pf);

  // cout<<"***\n\n===Zengwen: My test and debug is up this point===\n\n***"<<endl;


  BTNonLeafNode nnode;
  cout<<"Testing non leaf node:"<<endl;
  cout<<"currently there are getKeyCount() = " <<nnode.getKeyCount()<<" keys"<<endl;
  cout<<"-------------"<<endl;
  nnode.initializeRoot(1, 4, 4); // 
  cout<<"initialized root: <pid1, Key, pid2> = <1, 4, 4>"<<endl;
  nnode.printKeys();

  cout<<"Inserting (pid = 3, key = 3) pair to the node."<<endl;
  nnode.insert(3, 3);nnode.printKeys();
  cout<<"Inserting (pid = 2, key = 2) pair to the node."<<endl;
  nnode.insert(2, 2);nnode.printKeys();
  cout<<"Inserting (pid = 5, key = 5) pair to the node."<<endl;
  nnode.insert(5, 5);nnode.printKeys();
  cout<<"Inserting (pid = 7, key = 7) pair to the node."<<endl;
  nnode.insert(7, 7);nnode.printKeys();

  cout<<"currently there are getKeyCount() = " <<nnode.getKeyCount()<<" keys"<<endl;

  PageId pid_loc;
  cout<<"looking for pid location for key == 0"<<endl;
  nnode.locateChildPtr(0, pid_loc);
  cout<<"pid_loc is: "<<pid_loc<<endl;
  cout<<"looking for pid location for key == 3"<<endl;
  nnode.locateChildPtr(3, pid_loc);
  cout<<"pid_loc is: "<<pid_loc<<endl;
  cout<<"looking for pid location for key == 6"<<endl;
  nnode.locateChildPtr(6, pid_loc);
  cout<<"pid_loc is: "<<pid_loc<<endl;
  cout<<"looking for pid location for key == 7"<<endl;
  nnode.locateChildPtr(7, pid_loc);
  cout<<"pid_loc is: "<<pid_loc<<endl;
  cout<<"looking for pid location for key == 9"<<endl;
  nnode.locateChildPtr(9, pid_loc);
  cout<<"pid_loc is: "<<pid_loc<<endl;


  cout<<"Testing full non leaf node:"<<endl;
  BTNonLeafNode fullnnode;
  cout<<"currently there are getKeyCount() = " <<fullnnode.getKeyCount()<<" keys"<<endl;
  fullnnode.printKeys();
  // for(int i=0; i<5; i++) {
  for(int i=0; i<N_L-1; i++) {
   fullnnode.insert(i, i);
   // fullnnode.printKeys();
  }
  cout<<"currently there are getKeyCount() = " <<fullnnode.getKeyCount()<<" keys"<<endl;
  fullnnode.printKeys();

  BTNonLeafNode nonsibling;
  int midkey;
  fullnnode.insertAndSplit(100, 1, nonsibling, midkey);
  cout<<"===After split, the non leaf node has==="<<endl;
  fullnnode.printKeys();
  cout<<"===After split, the sibling non leaf node has==="<<endl;
  nonsibling.printKeys();
  cout<<"the returned midkey is = " <<midkey<<endl;

  return 0;
}

// int main()
// {
//   // run the SQL engine taking user commands from standard input (console).
//   SqlEngine::run(stdin);

//   return 0;
// }
