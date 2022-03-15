:: Script to compile DiskRead
:: by kenan238

@echo off

:: modify this incase something needs to change
SET src=diskread.c
SET out=diskread

g++ %src% -o %out% -O2