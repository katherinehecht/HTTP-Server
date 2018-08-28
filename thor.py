#!/usr/bin/env python3

import multiprocessing
import os
import requests
import sys
import time
import urllib

# Globals

PROCESSES = 1
REQUESTS  = 1
VERBOSE   = False
URL       = None

# Functions

def usage(status=0):
    print('''Usage: {} [-p PROCESSES -r REQUESTS -v] URL
    -h              Display help message
    -v              Display verbose output

    -p  PROCESSES   Number of processes to utilize (1)
    -r  REQUESTS    Number of requests per process (1)
    '''.format(os.path.basename(sys.argv[0])))
    sys.exit(status)

def do_request(pid):
    ''' Perform REQUESTS HTTP requests and return the average elapsed time. '''
    total_time = 0
    for i in range(REQUESTS):
        time_before = time.time()
        response = requests.get(URL)
        time_after = time.time()
        curr_time = time_after - time_before;
        if VERBOSE:
            print(urllib.request.urlopen(URL).read().decode('utf-8'))
        print('Process: {}, Request: {}, Elapsed Time: {:.02}\n'.format(pid, i, curr_time))
        total_time = total_time + curr_time

    ave_time = total_time / REQUESTS
    print('Process: {}, AVERAGE   , Elapsed Time: {:.02}\n'.format(pid, ave_time))
    return ave_time
# Main execution

if __name__ == '__main__':
    # Parse command line arguments
    args = sys.argv[1:]
    while len(args) and args[0].startswith('-') and len(args[0]) > 1:
        arg = args.pop(0)
        if arg == '-h':
            usage(0)
        elif arg == '-v':
            VERBOSE = True
        elif arg == '-p':
            arg = args.pop(0)
            PROCESSES = int(arg)
        elif arg == '-r':
            arg = args.pop(0)
            REQUESTS = int(arg)
        else:
            usage(1)

    if len(args) != 1:
        usage(1)

    URL = args[0]


    # Create pool of workers and perform requests
    pool = multiprocessing.Pool(PROCESSES)
    final_time = sum(pool.map(do_request, range(PROCESSES))) / PROCESSES
    print('TOTAL AVERAGE ELAPSED TIME: {:.02}\n'.format(final_time))




    pass

# vim: set sts=4 sw=4 ts=8 expandtab ft=python:
