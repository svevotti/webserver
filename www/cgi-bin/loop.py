#!/usr/bin/env python3

import sys
import time

# CGI requires headers first
print("Content-Type: text/plain")
print()  # Blank line to end headers

# Infinite loop
try:
    while True:
        time.sleep(1)  # Slow it down to simulate processing
        print("Processing...")
except KeyboardInterrupt:
    print("Process interrupted. Exiting...")
except BrokenPipeError:
    # Handle the broken pipe error if necessary
    pass
finally:
        # Any cleanup code can go here
        print("Cleanup complete.")