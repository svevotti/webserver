#!/usr/bin/python3
import sys
print ("Content-Type: text/plain\r")
print ("\r")
data = sys.stdin.read()  # Will be empty for GET
print ("Received: " + data)