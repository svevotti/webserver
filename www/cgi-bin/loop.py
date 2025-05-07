#!/usr/bin/env python3

import sys
import time

print("Content-Type: text/plain")
print()  # Blank line to end headers

# Infinite loop
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("Process interrupted. Exiting...")
except (BrokenPipeError, IOError):
    print ('BrokenPipeError caught', file = sys.stderr)

print ('Done', file=sys.stderr)
sys.stderr.close()