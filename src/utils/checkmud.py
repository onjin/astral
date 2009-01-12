#!/usr/bin/env python
# -*- encoding: utf-8 -*-
#

import socket

port = 4000
host = 'onjin.net'

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    s.connect((host, port))
    s.shutdown(2)
    print "UP"
except:
    print "DOWN"

