#!/usr/bin/env python3
import cgi
import os
form = cgi.FieldStorage()
file_item = form['file']
if file_item.filename:
    file_path = os.path.join('./www/uploads', os.path.basename(file_item.filename))
    with open(file_path, 'wb') as f:
        f.write(file_item.file.read())
    print("Status: 303 See Other")
    print("Location: /success.html")
    print()
else:
    print("Status: 400 Bad Request")
    print("Content-Type: text/plain")
    print()
    print("No file uploaded")