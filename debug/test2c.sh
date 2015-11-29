#!/bin/bash

clear
rm abc.tbl
make clean
make
./bruinbase > debug.txt