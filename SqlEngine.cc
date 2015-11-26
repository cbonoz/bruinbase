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
#include <climits>
#include "Bruinbase.h"
#include "SqlEngine.h"

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

RC SqlEngine::selectOnIndex(int attr, const std::string& table, BTreeIndex &btree, const vector<SelCond>& cond) {
  //  { EQ, NE, LT, GT, LE, GE }
  
  //general (for rf reading)
  IndexCursor cursor; // index cursor for locating tuples 
  IndexCursor end_cursor; // index cursor for last tuple (in range)
  RecordFile rf;   // RecordFile containing the table
  RecordId rid;
  rid.pid=0;
  rid.sid=0;

  string value;
  int key;
  int rc=0;
  //rid.pid=0;//page of record
  //rid.sid=0;//slot of record
  
  //key condition variables
  int key_min=INT_MIN, key_max=INT_MAX, key_exact=0;
  bool EQ_active=false,NEQ_active=false;
  vector<int> key_neq_vals;
  

  //value condition variables (only EQ and NEQ apply)
  bool val_NEQ_active=false, val_EQ_active=false;
  vector<string> val_neq_vals;
  string val_exact="";
  bool val_cond=false;


  //indicates whether non-null value could be returned from query
  bool logic_error = false;

  // open the table file
  if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
    fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
    return rc;
  }


  //loop over the specified conditions (assuming AND logic between conditions)
  int cond_length = cond.size();
  for (int i=0;i<cond_length;i++) {
    if (cond[i].attr==1) {
      int cur_key = atoi(cond[i].value);
      switch (cond[i].comp) {
        case SelCond::EQ:
          if (EQ_active && key_exact != cur_key) // key = val1 AND key = val2
            logic_error=true;
          else if (NEQ_active && key_exact == cur_key) // key = val and key <> val
            logic_error=true;

          key_exact = cur_key;    
          EQ_active=true;

          break; // break right away? that means you do not handle the logic error..
        case SelCond::NE:
          if (EQ_active && key_exact == cur_key)
            logic_error=true;

          key_neq_vals.push_back(cur_key);
          NEQ_active=true;

          break; // break right away? that means you do not handle the logic error..
        case SelCond::GT: //keep greatest key min value
          if (key_min<cur_key)
            key_min=cur_key;

          break;
        case SelCond::GE:
          cur_key--; //turn GE into GT (since keys are integers)
          if (key_min<cur_key)
            key_min=cur_key;
          
          break;
        case SelCond::LT:  //keep lowest key max value
          if (key_max>cur_key)
            key_max=cur_key;
          
          break;
        case SelCond::LE:
          cur_key++;
          if (key_max>cur_key)
            key_max=cur_key;
          
          break;
        }

    } else { //attr == 2
      val_cond=true;
      //only considered == and != right now
      string cur_value(cond[i].value);
      switch (cond[i].comp) {

        case SelCond::EQ:
          if (val_EQ_active && val_exact != cur_value) {
            // values are not equal and eq_active
            logic_error=true;
          } else if (val_NEQ_active && val_exact == cur_value) {
            //values are equal and neq_active
            logic_error=true;
          }

          val_exact.assign(cond[i].value);
          val_EQ_active=true;

          break;
        case SelCond::NE:
        if (val_EQ_active && val_exact == cur_value) //logic error
            logic_error=true;

          val_neq_vals.push_back(cur_value);
          val_NEQ_active=true;

          break;
        default:
          printf("\nInvalid condition %d (which is not EQ or NEQ) detected with attr=2",cond[i].comp);
          logic_error=true;
          break;

      }
    }
  }

  /***********************************************************/
  /* Conditions have been determined - time to run the query */
  /***********************************************************/

  //run the query only if no logic error
  if (!logic_error) {
    //note that equality of a key takes precedence over all other comparators
    if (!EQ_active) {
      btree.locate(key_min,cursor, 0);
      btree.locate(key_max,end_cursor, 0);
    } else { //equality is active
      btree.locate(key_exact,cursor, 0);
      btree.readForward(cursor, key, rid);

      if (key==key_exact){
        if ((rc = rf.read(rid, key, value)) < 0) {
          fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
          rf.close();
          return rc;
        }

        //determine type of print
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
            case 4:
              fprintf(stdout, "%d\n", 1);
              break;
            
        }
      }

      rf.close();
      return 0;

    }


    const int neq_val_len = val_neq_vals.size();
    const int neq_key_len = key_neq_vals.size();

    int count=0;
    //need to condider how this is handled if eq is active
    //right now assuming eq will not be active (only neq here)
    while(btree.readForward(cursor, key, rid)==0) {
      if ((cursor.pid == end_cursor.pid) && (cursor.eid==end_cursor.eid))
        return 0; //done

      if ((rc = rf.read(rid, key, value)) < 0) {
        fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
        rf.close();
        return rc;
      }

      //need to check eq and neq on key and value
      bool violation=false;

      //check if key violates conditions
      if (NEQ_active) {
        for (int i=0;i<neq_key_len;i++) {
            if (key_neq_vals[i]==key) //if keys equal
              violation=true; 
        }
      }
      //check if value violates conditions
      if (val_NEQ_active) {
        for (int i=0;i<neq_val_len;i++) {
            if (val_neq_vals[i]==value)//if strings equal
              violation=true;
        }
      }

      if (!violation) {

        //determine type of print
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
        count++;
      } else {
        continue; //violation of tuple, continue read forward
      }


    }

    if (attr==4) {
      fprintf(stdout, "%d\n", count);
    }


  } else {
    printf("\nLogic Error detected in select conditions");
  }

  rf.close();
  return 0;


}

RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
  RecordFile rf;   // RecordFile containing the table
  RecordId   rid;  // record cursor for table scanning
  BTreeIndex btree; // BTree index
  

  RC     rc;
  int    key;     
  string value;
  int    count;
  int    diff;

  string index_file = table + ".idx";
  rc = btree.open((index_file).c_str(), 'r');

  //if index file successfully opens - use index select method
  if (rc == 0) { 
    rc=selectOnIndex(attr, table, btree, cond);
    return rc;
  }


  // open the table file
  if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
    fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
    return rc;
  }
  // TODO : Finish index search
  int searchkey = 0;

  // scan the table file from the beginning
  rid.pid = rid.sid = 0;
  count = 0;
  
  while (rid < rf.endRid()) {
    // read the tuple
    if ((rc = rf.read(rid, key, value)) < 0) {
      fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
      goto exit_select;
    }

    // check the conditions one by one on the tuple to see if they hold
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
