#!/bin/bash

[ -d "./tmp" ] || mkdir ./tmp
[ -d "./dist" ] || mkdir ./dist

# README_HTML=`pandoc -f gfm -t html5 ../README.md`

cp -Rf ./src/*.html ./tmp/
tsc ./src/main.ts --outDir ./tmp/ -lib es2019,dom -t es2015

for FILE in ./tmp/*; do
    minify "${FILE}" > "./dist/${FILE/.\/tmp\/}"
done

rm -fr ./tmp