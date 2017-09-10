#!/bin/bash
gcc $1 -o _${1%.c} -lm -g -Wall -std=gnu99
