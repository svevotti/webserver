#!/usr/bin/python3
import sys
import time
import os
import signal

# Get CGI timeout from environment variable or use default from config
def get_cgi_timeout():
    try:
        if 'CGI_PROCESSING_TIMEOUT' in os.environ:
            timeout = float(os.environ['CGI_PROCESSING_TIMEOUT'])
            # sys.stderr.write(f"[CGI Debug] Read CGI_PROCESSING_TIMEOUT: {timeout} seconds\n")
            # sys.stderr.write(f"[CGI Debug] Server context: SERVER_NAME={os.environ.get('SERVER_NAME', 'unknown')}, SERVER_PORT={os.environ.get('SERVER_PORT', 'unknown')}\n")
            return timeout
        # sys.stderr.write(f"[CGI Debug] CGI_PROCESSING_TIMEOUT not found in environment. Defaulting to 7.0 seconds\n")
        # sys.stderr.write(f"[CGI Debug] Environment variables: {str(os.environ)}\n")
        return 7.0
    except (ValueError, TypeError) as e:
        # sys.stderr.write(f"Error parsing timeout: {str(e)}\n")
        return 7.0

CGI_TIMEOUT = get_cgi_timeout()

def timeout_handler(signum, frame):
    sys.stdout.write("503 - Service Unavailable: This took too long!\n")
    sys.stdout.flush()
    sys.exit(0)

def send_error(status_code, status, message):
    sys.stdout.write(f"{status_code} - {status}: {message}\n")
    sys.stdout.flush()
    sys.exit(0)

try:
    sys.stdout.write("Content-Type: text/plain\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.flush()
    signal.signal(signal.SIGALRM, timeout_handler)
    signal.alarm(int(CGI_TIMEOUT))
    while True:
        sys.stdout.write("This is an infinite loop\n")
        sys.stdout.flush()
        time.sleep(0.1)

except Exception as e:
    send_error(500, "Internal Server Error", "Unexpected error")





