#!/bin/bash
TMP_DIR=/tmp/p2_tmp
ZIP_FILE="P2.zip"
REQUIRED_FILES="readme.txt team.txt Makefile main.cc SqlEngine.cc BTreeIndex.cc BTreeNode.cc RecordFile.cc PageFile.cc Bruinbase.h PageFile.h SqlEngine.h BTreeIndex.h BTreeNode.h RecordFile.h SqlParser.l SqlParser.y"

# usage
if [ $# -ne 1 ]
then
     echo "Usage: $0 Your_UID" 1>&2
     exit
fi

FOLDER_NAME=$1

# clean any existing files
rm -rf ${TMP_DIR}
mkdir ${TMP_DIR}

# unzip the submission zip file 
if [ ! -f ${ZIP_FILE} ]; then
    echo "ERROR: Cannot find $ZIP_FILE, ensure this script is put in the same directory of your P2.zip file. Otherwise check the zip file name" 1>&2
    rm -rf ${TMP_DIR}
    echo "rmd"
    exit 1
fi
unzip -q -d ${TMP_DIR} ${ZIP_FILE}
if [ "$?" -ne "0" ]; then 
    echo "ERROR: Cannot unzip ${ZIP_FILE} to ${TMP_DIR}"
    rm -rf ${TMP_DIR}
    exit 1
fi

# change directory to the grading folder
cd ${TMP_DIR}

if [ ! -d ${FOLDER_NAME} ];
then
echo "Check your folder name is EXACTLY the same as UID you typed"
rm -rf ${TMP_DIR}
exit 1
fi

cd ${FOLDER_NAME}

# check the existence of the required files
for FILE in ${REQUIRED_FILES}
do
    if [ ! -f ${FILE} ]; then
        echo "ERROR: Cannot find ${FILE} in the UID folder of your zip file" 1>&2
	rm -rf ${TMP_DIR}
        exit 1
    fi
done

echo "Checking whether your code compiles without any error..."
make -s clean
make -s
if [[ "$?" -ne "0" ]]; then
    echo "ERROR: Compilation error. Please fix the compilation error(s) first" 1>&2
    rm -rf ${TMP_DIR}
    exit 1
fi

echo "Everything seems OK. Please upload your P2.zip file to CCLE."
rm -rf ${TMP_DIR}
exit 0