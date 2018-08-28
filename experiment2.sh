#!/bin/bash


echo "Small File" 
echo $(thor.py -r 1 -p 1 http://student02.cse.nd.edu:9080/smallFile.txt | grep "TOTAL" | cut -d ' ' -f 5)
echo $(thor.py -r 1 -p 2 http://student02.cse.nd.edu:9080/smallFile.txt | grep "TOTAL" | cut -d ' ' -f 5)
echo $(thor.py -r 1 -p 4 http://student02.cse.nd.edu:9080/smallFile.txt | grep "TOTAL" | cut -d ' ' -f 5)

echo "Medium File"
echo $(thor.py -r 1 -p 1 http://student02.cse.nd.edu:9080/mediumFile.txt | grep "TOTAL" | cut -d ' ' -f 5)
echo $(thor.py -r 1 -p 2 http://student02.cse.nd.edu:9080/mediumFile.txt | grep "TOTAL" | cut -d ' ' -f 5)
echo $(thor.py -r 1 -p 4 http://student02.cse.nd.edu:9080/mediumFile.txt | grep "TOTAL" | cut -d ' ' -f 5)

echo "Large File"
echo $(thor.py -r 1 -p 1 http://student02.cse.nd.edu:9080/100-0.txt | grep "TOTAL" | cut -d ' ' -f 5)
echo $(thor.py -r 1 -p 2 http://student02.cse.nd.edu:9080/100-0.txt | grep "TOTAL" | cut -d ' ' -f 5)
echo $(thor.py -r 1 -p 4 http://student02.cse.nd.edu:9080/100-0.txt | grep "TOTAL" | cut -d ' ' -f 5)

