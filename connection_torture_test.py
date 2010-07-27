#!/usr/bin/python

import socket
import math
import random

HOST="127.0.0.1"
PORT=8000

counter = 2000
socketlist = [ ]

while counter > 0:
    counter = counter - 1
    print("Torture test: counter %d" % counter)
    
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(0.05)
    try:
        s.connect( (HOST, PORT) )
        socketlist.append(s)
    except:
        print("Could not connect")

    if (random.random() < 0.1 and len(socketlist) > 0):
        index = random.randint(0, len(socketlist)-1)
        socketlist[index].close()
        socketlist.pop(index)

    for s in socketlist:
        for i in range(0, random.randint(50, 100)):
            try:
                s.send(chr(random.randint(0, 255)))
            except:
                socketlist.remove(s)
                break

