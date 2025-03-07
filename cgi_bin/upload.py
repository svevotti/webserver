#!/usr/bin/python3
import cgi
import os
import sys

def save_uploaded_file(upload_dir):
    form = cgi.FieldStorage()
    print("Form keys: %s" % list(form.keys()), file=sys.stderr)
    if "file" not in form:
        print("HTTP/1.1 400 Bad Request")
        print("Content-Length: 16")
        print()
        print("No file uploaded")
        return
    file_item = form["file"]
    if file_item.filename:
        filename = os.path.join(upload_dir, "uploaded_image.jpg")
        with open(filename, "wb") as f:
            f.write(file_item.file.read())
        print("HTTP/1.1 200 OK")
        print("Content-Length: 43")
        print()
        print("Image saved to %s" % filename)
    else:
        print("HTTP/1.1 400 Bad Request")
        print("Content-Length: 16")
        print()
        print("No file uploaded")

if __name__ == "__main__":
    print("sys.argv: %s" % sys.argv, file=sys.stderr)
    upload_dir = sys.argv[1] if len(sys.argv) > 1 else "./uploads"
    print("Upload directory: %s" % upload_dir, file=sys.stderr)
    save_uploaded_file(upload_dir)