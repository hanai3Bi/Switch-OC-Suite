#!/bin/bash

[ -d "./dist" ] || mkdir ./dist
cp -Rf ./src/*.html ./dist/
# README_HTML=`pandoc -f gfm -t html5 ../README.md`
tsc ./src/main.ts --outDir ./dist/ -lib es2015,dom -t es2015
