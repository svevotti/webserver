#!/usr/bin/python3
import sys
print("Content-Type: text/plain\r")
print("\r")
data = sys.stdin.read()
print("Received: " + data)

