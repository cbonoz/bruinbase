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

void printnode(BTNonLeafNode node) {
  int key;
  PageId pid;
  cout<<"<key, pid>"<<endl;
  for(int i=0; i<node.getKeyCount(); i++) {
	  node.readEntry(i, key, pid);
	  cout<<"["<<key<<", "<<pid<<"]\t";
  }

  cout<<endl<<"---------"<<endl;
  return;	
}

int main()
{
  BTLeafNode leafnode;
  printnode(leafnode);
  // leafnode.BTLeafNode();
  cout<<"Testing leaf node:"<<endl;
  // leafnode.initBuffer();
  leafnode.printKeys();


  cout<<"===use another method to print leaf node==="<<endl;
  printnode(leafnode);
  cout<<"currently there are getKeyCount() = " <<leafnode.getKeyCount()<<" keys"<<endl;
  cout<<"-------------"<<endl;
  RecordId rid = {1,2};
  cout<<"set a rid at {1,2}"<<endl;
  
  cout<<"Inserting (3, rid) pair to the node."<<endl;
  leafnode.insert(3, rid);
  cout<<"currently there are getKeyCount() = " <<leafnode.getKeyCount()<<" keys"<<endl;
  leafnode.printKeys();printnode(leafnode);
  cout<<"Inserting (2, rid) pair to the node."<<endl;
  leafnode.insert(2, rid);
  cout<<"currently there are getKeyCount() = " <<leafnode.getKeyCount()<<" keys"<<endl;
  leafnode.printKeys();printnode(leafnode);
  cout<<"Inserting (5, rid) pair to the node."<<endl;
  leafnode.insert(4, rid);
  cout<<"currently there are getKeyCount() = " <<leafnode.getKeyCount()<<" keys"<<endl;
  leafnode.printKeys();printnode(leafnode);
  cout<<"Inserting (7, rid) pair to the node."<<endl;
  leafnode.insert(1, rid);
  cout<<"currently there are getKeyCount() = " <<leafnode.getKeyCount()<<" keys"<<endl;
  leafnode.printKeys();printnode(leafnode);
  // leafnode.printKeys();
  cout<<"===set next node ptr to 123==="<<endl;
  leafnode.setNextNodePtr(123);
  cout<<"Next node ptr is: "<<leafnode.getNextNodePtr()<<endl;
  
  for(int i=6; i<N_NL+1; i++) {
  	leafnode.insert(i, rid);
  }
  leafnode.printKeys();printnode(leafnode);

  BTLeafNode sibling;
  int siblingkey;
  cout<<"*Inserting (5, rid) pair to split the node."<<endl;
  leafnode.insertAndSplit(5, rid, sibling, siblingkey);

  cout<<"===After split, the leaf node has==="<<endl;
  printnode(leafnode);
  cout<<"===After split, the sibling node has==="<<endl;
  printnode(sibling);
  cout<<"*Sibling Key = "<<siblingkey<<endl;

  BTLeafNode fullleafnode;
  cout<<"===set next node ptr to 888==="<<endl;
  fullleafnode.setNextNodePtr(888);
  cout<<"Full node's next node ptr is: "<<fullleafnode.getNextNodePtr()<<endl;



  cout<<"***\n\n===Zengwen: My test and debug is up this point===\n\n***"<<endl;

  


    PageFile pf;
    pf.open("test.pf", 'w');
    fullleafnode.write(pf.endPid(), pf);


  BTNonLeafNode nnode;
  // nnode.BTNonLeafNode();
  cout<<"Testing non leaf node:"<<endl;
  cout<<nnode.getKeyCount()<<endl;
  cout<<"-------------"<<endl;
  nnode.initializeRoot(1, 4, 4);
  cout<<"initialized:"<<endl;
  printnode(nnode);

  nnode.insert(3, 3);
  nnode.insert(2, 2);
  nnode.insert(5, 5);
  nnode.insert(7, 7);
  printnode(nnode);
  cout<<nnode.getKeyCount()<<endl;
  cout<<"----------------"<<endl;
  PageId pid_loc;
  nnode.locateChildPtr(0, pid_loc);
  cout<<"pid_loc: "<<pid_loc<<endl;

  BTNonLeafNode sibling2;
  // nnode.BTNonLeafNode();
  int midkey;
  nnode.insertAndSplit(4, 1, sibling2, midkey);
  printnode(nnode);
  printnode(sibling2);
  cout<<midkey<<endl;


  return 0;
}

// int main()
// {
//   // run the SQL engine taking user commands from standard input (console).
//   SqlEngine::run(stdin);

//   return 0;
// }
