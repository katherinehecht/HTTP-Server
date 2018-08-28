#!/bin/bash


echo "Directory Listing" 
echo $(thor.py -r 100 http://student02.cse.nd.edu:9081/html | grep "TOTAL" | cut -d ' ' -f 5)

echo "Static Text"
echo $(thor.py -r 100 http://student02.cse.nd.edu:9081/songs.txt | grep "TOTAL" | cut -d ' ' -f 5)

echo "CGI Scripts"
echo $(thor.py -r 100 http://student02.cse.nd.edu:9081/scripts/cowsay.sh | grep "TOTAL" | cut -d ' ' -f 5)

