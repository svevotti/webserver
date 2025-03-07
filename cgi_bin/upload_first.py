#!/usr/bin/env python3

import cgi
import os

# Set the directory where uploaded files will be saved
UPLOAD_DIR = "uploads"

# Ensure the upload directory exists
if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR)

# Parse the form data
form = cgi.FieldStorage()

# Get the file from the form
file_item = form['file']

# Check if a file was uploaded
if file_item.filename:
    # Save the file to the upload directory
    file_path = os.path.join(UPLOAD_DIR, os.path.basename(file_item.filename))
    with open(file_path, 'wb') as f:
        f.write(file_item.file.read())
    message = f"File '{file_item.filename}' uploaded successfully!"
else:
    message = "No file uploaded."

# Output the response
print("Content-Type: text/html\n")
print(f"<html><body><h1>{message}</h1></body></html>")