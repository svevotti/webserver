#!/usr/bin/env python3

# Import necessary modules
import cgi

# Set the content type to HTML
print("Content-Type: text/html; charset=UTF-8")
print()  # End of headers

# HTML content
html_content = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Simple Page</title>
</head>
<body>
    <h1>Hi from child</h1>
</body>
</html>
"""

# Print the HTML content
print(html_content)