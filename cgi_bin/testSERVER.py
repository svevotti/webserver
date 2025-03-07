#!/usr/bin/python3
import sys
import time
print("Content-Type: text/plain\r")
print("\r")
data = sys.stdin.read()
time.sleep(5)  # Simulate a 5-second delay
print("Received: " + data)