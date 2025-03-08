#!/usr/bin/python3
import sys
print("Content-Type: text/plain\r")
print("\r")
input_data = sys.stdin.read()
print("Received: " + input_data if input_data else "Hello from test.py")