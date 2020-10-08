#!/bin/bash

set -ex

rm -rf ports/$1
cd build
xbstrap install -u $1
