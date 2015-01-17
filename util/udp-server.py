#!/usr/bin/env python

from socket import *
import sys
import select
address = ('', 6005)
server_socket = socket(AF_INET, SOCK_DGRAM)
server_socket.bind(address)

while(1):
    print "Listening"
    recv_data, addr = server_socket.recvfrom(2048)
    print recv_data
