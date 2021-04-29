#!/bin/sh

find -E "$@" -maxdepth 3 -regex ".*\.(c|cc|cpp|cxx)$"
