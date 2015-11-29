#!/bin/sh

make clean
make
rm -f xsmall.tbl xsmall.idx
rm -f small.tbl small.idx
rm -f medium.tbl medium.idx
rm -f large.tbl large.idx
rm -f xlarge.tbl xlarge.idx
# clear

# ./bruinbase < test.sql
# ./bruinbase < test2d.sql
./bruinbase < test2d.sql > debug2d.txt
