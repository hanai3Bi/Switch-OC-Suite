#!/bin/bash

[ -d "./dist" ] || mkdir ./dist
cp -Rf ./src/*.html ./dist/
tsc ./src/main.ts --outDir ./dist/ -lib es2015,dom -t es2015
npx tailwindcss -i ./src/style.css -o ./dist/output.css
