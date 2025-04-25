#!/usr/bin/env python3

import sys
import time

# CGI requires headers first
print("Content-Type: text/plain")
print()  # Blank line to end headers

# Infinite loop
while True:
    print("This is an infinite loop")
    sys.stdout.flush()  # Ensure output is sent immediately
    time.sleep(1)  # Slow it down to simulate processing