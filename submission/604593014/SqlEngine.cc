/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "Bruinbase.h"
#include "SqlEngine.h"
#include "BTreeIndex.h"

using namespace std;

// external functions and variables for load file and sql command parsing 
extern FILE* sqlin;
int sqlparse(void);


RC SqlEngine::run(FILE* commandline)
{
  fprintf(stdout, "Bruinbase> ");

  // set the command line input and start parsing user input
  sqlin = commandline;
  sqlparse();  // sqlparse() is defined in SqlParser.tab.c generated from
               // SqlParser.y by bison (bison is GNU equivalent of yacc)

  return 0;
}

RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
  RecordFile rf;   // RecordFile containing the table
  RecordId   rid;  // record cursor for table scanning
  // BTreeIndex index; // BTree index
  // IndexCursor cursor; // index cursor for locating tuples

  RC     rc;
  int    key;     
  string value;
  int    count;
  int    diff;

  // open the table file
  if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
    fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
    return rc;
  }
  // TODO : Finish index search
  // int searchkey = 0;

  // scan the table file from the beginning
  rid.pid = rid.sid = 0;
  count = 0;

  while (rid < rf.endRid()) {
    // read the tuple
    if ((rc = rf.read(rid, key, value)) < 0) {
      fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
      goto exit_select;
    }

    // check the conditions on the tuple
    for (unsigned i = 0; i < cond.size(); i++) {
      // compute the difference between the tuple value and the condition value
      switch (cond[i].attr) {
      case 1:
        diff = key - atoi(cond[i].value);
        break;
      case 2:
        diff = strcmp(value.c_str(), cond[i].value);
        break;
      }

      // skip the tuple if any condition is not met
      switch (cond[i].comp) {
      case SelCond::EQ:
        if (diff != 0) goto next_tuple;
        break;
      case SelCond::NE:
        if (diff == 0) goto next_tuple;
        break;
      case SelCond::GT:
        if (diff <= 0) goto next_tuple;
        break;
      case SelCond::LT:
        if (diff >= 0) goto next_tuple;
        break;
      case SelCond::GE:
        if (diff < 0) goto next_tuple;
        break;
      case SelCond::LE:
        if (diff > 0) goto next_tuple;
        break;
      }
    }

    // the condition is met for the tuple. 
    // increase matching tuple counter
    count++;

    // print the tuple 
    switch (attr) {
      case 1:  // SELECT key
        fprintf(stdout, "%d\n", key);
        break;
      case 2:  // SELECT value
        fprintf(stdout, "%s\n", value.c_str());
        break;
      case 3:  // SELECT *
        fprintf(stdout, "%d '%s'\n", key, value.c_str());
        break;
    }

    // move to the next tuple
    next_tuple:
    ++rid;
  }

  // print matching tuple count if "select count(*)"
  if (attr == 4) {
    fprintf(stdout, "%d\n", count);
  }
  rc = 0;

  // close the table file and return
  exit_select:
  rf.close();
  return rc;
}

/**
 * load a table from a load file.
 * @param table[IN] the table name in the LOAD command
 * @param loadfile[IN] the file name of the load file
 * @param index[IN] true if "WITH INDEX" option was specified
 * @return error code. 0 if no error
 */
RC SqlEngine::load(const string& table, const string& loadfile, bool index)
{
  RecordFile wf; // RecordFile to store the table
  // The ifstream constructor expects a const char*, so we need to do ifstream file(filename.c_str()); to make it work.
  ifstream rf(loadfile.c_str()); // RecordFile for the load file
  RecordId rid;  // record cursor for table scanning
  // ifstream idxf;
  BTreeIndex btree; // B+ tree for index

  RC rc = 0;

  string line;
  string value;

  int    key;  
  int    count;
  int    diff;

  // open the table file
  if ((rc = wf.open(table + ".tbl", 'w') < 0)) {
    fprintf(stderr, "Error: table %s file could not be opened for writing\n", table.c_str());
    wf.close();
    return rc;
  }

  if (!rf.is_open()) {
    fprintf(stderr, "Error: record %s file could not be opening for reading\n", loadfile.c_str());
    wf.close();
    rf.close();
    return RC_FILE_OPEN_FAILED;
  }

  if (index) {
    // idxf.open((table + ".idx").c_str());
    // if (!idxf.is_open()) {
    //   fprintf(stderr, "Error: index %s file could not be opened for writing\n", loadfile.c_str());
    //   return RC_FILE_OPEN_FAILED;
    // }
    rc = btree.open((table + ".idx").c_str(), 'w');
    if (rc != 0) {
      fprintf(stderr, "Error: index %s file could not be opened for writing\n", loadfile.c_str());
      wf.close();
      rf.close();
      return rc; //RC_FILE_OPEN_FAILED;
    }
  }

  // initialize the record ID
  rid.pid = rid.sid = 0;

  while (!rf.eof()) {
    getline(rf, line);
    if (!line.empty()) {
      rc = parseLoadLine(line, key, value);
      if (rc != 0) {
        fprintf(stderr, "Error: cannot parse load line\n");
        wf.close();
        rf.close();
        return rc;
      } else { // parseLoadLine failed
        rc = wf.append(key, value, rid);
        if (rc != 0) { // append line failed
          fprintf(stderr, "Error: cannot append value to record file\n");
          wf.close();
          rf.close();
          return rc;
        }

        // insert corresponding (key, RecordId) to 
        // idxf for each tuple of the inserted file
        if (index) { // inserting key into index
          rc = btree.insert(key, rid);
          if (rc != 0) { // insert into index failed
            fprintf(stderr, "Error: cannot insert key into index %s file\n", loadfile.c_str());
            wf.close();
            rf.close();
            return rc;
          }
        }
      }
    }
  }

  wf.close();
  rf.close();
  if (index) {
    btree.close();
  }

  return 0;
}

RC SqlEngine::parseLoadLine(const string& line, int& key, string& value)
{
    const char *s;
    char        c;
    string::size_type loc;
    
    // ignore beginning white spaces
    c = *(s = line.c_str());
    while (c == ' ' || c == '\t') { c = *++s; }

    // get the integer key value
    key = atoi(s);

    // look for comma
    s = strchr(s, ',');
    if (s == NULL) { return RC_INVALID_FILE_FORMAT; }

    // ignore white spaces
    do { c = *++s; } while (c == ' ' || c == '\t');
    
    // if there is nothing left, set the value to empty string
    if (c == 0) { 
        value.erase();
        return 0;
    }

    // is the value field delimited by ' or "?
    if (c == '\'' || c == '"') {
        s++;
    } else {
        c = '\n';
    }

    // get the value string
    value.assign(s);
    loc = value.find(c, 0);
    if (loc != string::npos) { value.erase(loc); }

    return 0;
}
