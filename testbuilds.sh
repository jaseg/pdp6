#!/bin/sh

make clean
CC=gcc make all clean
CC=clang make all clean
