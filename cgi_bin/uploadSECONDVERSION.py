#!/usr/bin/python3
import os, sys
data = sys.stdin.read()
filename = os.environ.get("HTTP_CONTENT_DISPOSITION", "uploaded_file").split('filename="')[1].strip('"')
with open("./uploads/" + filename, "wb") as f:
    f.write(data.encode() if isinstance(data, str) else data)
print("Status: 303 See Other")
print("Location: /uploads")
print("")
