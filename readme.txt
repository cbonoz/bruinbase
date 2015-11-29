Chris Buonocore
904587506

Zengwen Yuan
604593014



Part D:
11/29/2015
---

Comments:

This part took the index implementation in Part C and adapted it into the insert and load functionality of the SQL Engine.

The biggest part of this step was getting the SQL select option with index to actually use the index correctly. We mirrored the select/logic structure from the without-index case to use the index for locating records very quickly. Considering a branching factor of roughly 86 in our tree, the amount of random I/O's to find a record is reduced to log N / log 86, where N is the number of tuples in the database (a drastic improvement over the original un-indexed method for locating records).

One issue we encountered was an off-by-one error in our insert comparison with keycount, causing a very subtle overwrite of seemingly-random tuples in the table (for example, the large table was missing 3 records, but everything else was loading correctly). By inserting some debug print statement into our program, we were able to find that the getKeyCount() comparison using > was causing one of the records in an insert to be overwritten, changing this to >= fixed this issue.

Additional optimizations: We do initial Logic analysis that will avoid doing queries if the given conditions in a select query conflict with each other (for example, a condition like key=100 AND key=150 will automatically return NULL and not print anything). We also have the ability to easily extend to != conditions on keys even though not required in spec (note this has been commented out for the submission).

Part D is functional and passes all tests defined in test.sh with acceptable page reads according to the original TA comments.


Part C:
---

Comments:

This part added on an Index to our existing database structure.

To test the BTreeIndex.cc implementation, we expended our test cases to load the index file and monitor how the index was constructed on the disk (Pagefile). Overflow and parent Splitting were challenging components to create precisely; the test cases revealed a few off-by-one errors in our original BTreeNode implementation of insertAndSplit which we were able to fix.

Another difficult component was figuring out the correct way to track the path of parent nodes when the B tree was traversed to find a particular leaf pointer. To accomplish the tracking, we use a dynamically resizable array (vector) to store the pids of the nodes we visit when we locate a particular entry in the table.

We noticed that rootPid and treeHeight are key index variables that are needed to reconstruct the index should the index file be closed and reopened again. We had the idea of appending these values to every page of the pagefile; however, we decided to create a specifical Index/Tree information page for each Index on the disk that would not store any other tree data. This gives us the advantage of scalability in that these values don't need to be repetitively stored for each page, and the data page can easily be extended to hold more configuration settings for the index if needed. For small data sets, this may not be as much of an advantage as it requires a fresh page to store the values.

We started on the implementation of Part D (loading the index), and will use this index to achieve faster range select queries on the underlying dataset.


Part B:
---

Comments:

Added test functions and routines to main.cc for verifying the implementation of BTreeNode. Normal bruinbase can be enabled by simply changing the definition of main in main.cc

Currently limited use of macros - encountered some strange behavior when calculations were included in the #define expression. Will intend to use macros in the future.

We reduced the true N for the tree equal to the maximum amount of pairs containable in either Nonlead or Leafnode. The limiting N value ended up coming from the nonleaf node N=86

The last resolved problem was fixing an off by one error in the locate function when keys appeared at the end of the list. We needed to shift the keycount by one in this case to make sure we were able to retrieve the last pointer value from the node.


Part A
---

# bruinbase
bruinbase proj2 repo for 143

Contains all required base files (any other files are test-related)

######################################
# CS143 Database System              #
# Project 2A, Bruinbase System       #
# Chris Buonocore, SID: 904587506    #
# Zengwen Yuan, SID: 604593014       #
# Ver 1.1, Nov. 5th 2015             #
######################################

This is the readme file for Project 2A of CS143.


##<Overview>

In this part, we implemented the bulk LOAD command, similar to the LOAD DATA command in MySQL for Project 1B. For Bruinbase, the syntax to load data into a table is:

	LOAD tablename FROM 'filename' [ WITH INDEX ]


##<Objective>

The main objective of this project is:
+ implement SqlEngine::load(table, loadfile, index) function with the user-provided table name and the load file name as its parameters


##<Implementation>

Following the functional headers in SqlEngine.h and the instruction on project specification, we are dealing with the command:

	LOAD movie FROM 'movie.del'

thus, we have to open two files using open:
a) the `movie.del' file
b) the `movie.tbl' file we are creating

We first read the content of `movie.del' file using getline(), then call parseLoadLine() to parse the line input and extract the <key, value> pair, and use append(key, value, rid) to add the <key, value> pair to the `movie.tbl' file.

Finally, we close the files to save the results.

One tricky case we successfully handled is the empty line in the `movie.del' file. We used `empty()' function to check whther `getline()' returns an empty line so that we can discard it.

Also, when compiling the code we've got an error [C++ ifstream error using string as opening file path](http://stackoverflow.com/questions/6323619/c-ifstream-error-using-string-as-opening-file-path)

	SqlEngine.cc:144:23: error: no matching function for call to ‘std::basic_ifstream<char>::basic_ifstream(const string&)’
	   ifstream rf(loadfile); // RecordFile for the load file

which can be fixed by using
	ifstream rf(loadfile.c_str());

Because the constructor for an ifstream takes a const char*, not a string pre-C++11. 


## <Test Results>

First, we renamed the original table to `movie_orginal.tbl'.

Then, we entered the bruinbase and issued the load command, and the sample queries:
$ ./bruinbase 
Bruinbase> load movie from 'movie.del'
Bruinbase> select * from movie where key > 1000 and key < 1010
1008 'Death Machine'
1002 'Deadly Voyage'
1004 'Dear God'
1003 'Deal of a Lifetime'
1009 'Death to Smoochy'
  -- 0.000 seconds to run the select command. Read 403 pages
Bruinbase> select count(*) from movie
3616
  -- 0.000 seconds to run the select command. Read 403 pages
Bruinbase> select * from movie where key=3421
3421 'Remember the Titans'
  -- 0.010 seconds to run the select command. Read 403 pages
Bruinbase> select * from movie where value='Die Another Day'
1074 'Die Another Day'
  -- 0.000 
Bruinbase> quit

Last, we compared the binary files generated by our program against the original one:
$ diff movie.tbl movie_original.tbl
$

which the output is empty so that two binary files are exactly the same.

Thus, our implementation is correct.

