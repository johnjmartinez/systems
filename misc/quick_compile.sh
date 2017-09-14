#!/bin/bash
gcc $1 -o _${1%.c}  -g -std=gnu99
