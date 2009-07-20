#!/usr/bin/env python
import sys
import os
import time

intPort=4000
if len(sys.argv) > 1:
    intPort = int(sys.argv[1])

while 1:
    intIndex = 1000
    print "start main loop"

    while 1:
        strLogfile = "../log/%d.log" % intIndex
        if os.path.isfile( strLogfile):
            print "searching: %s" % strLogfile
            intIndex += 1
            continue
        else:
            break

    print "logfile: %s" % strLogfile
    os.system("date > %s" % strLogfile)
    os.system("date > ../area/boot.txt")
    strCommand = "cd ../area/;"
    strCommand += "../src/smaug %d 2>&1 >>  %s" % ( intPort, strLogfile )
    os.system( strCommand )

    if os.path.isfile( 'shutdown.txt' ):
        os.system('rm -f shutdown.txt')
        sys.exit()
    time.sleep(5)

