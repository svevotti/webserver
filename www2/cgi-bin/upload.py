#!/usr/bin/python3
import os
import sys
import io
import json
import logging

# Set up logging to a file
logging.basicConfig(filename='/Users/hdorado/Webserv_final/upload.py.log', level=logging.DEBUG,
                    format='%(asctime)s - %(message)s')
# These are for backup
ERROR_MESSAGES = {
    400: "400 - Bad Request: Yo, Your Upload's Got No Vibe!",
    405: "405 - Method Not Allowed: Wrong Dance Move. Today You Cannot Groove!",
    409: "409 - Conflict: File Already Exists, Pick a New Groove!",
    413: "413 - Payload Too Large: That File's Too Chunky for Our Dance Floor!",
    #415: "415 - Unsupported Media Type: Gimme Another Tune!"
    500: "500 - Internal Server Error: Server Lost Its Groove, Ouch!",
    501: "501 - Not Implemented: That Trick Ain't in Our Playlist Yet!",
    503: "503 - Service Unavailable: Party's Too Packed, Can't Handle More!",
    504: "504 - Gateway Timeout: The Groove Timed Out, Baby!",
    505: "505 - HTTP Version Not Supported: Your Protocol's Outta Sync!"
}

def send_json_response(status_code, status, message):
    print(f"Status: {status_code} {status}")
    print("Content-Type: application/json")
    print()
    response = {"status": status.lower(), "message": message}
    print(json.dumps(response))
    sys.exit(0)

def save_uploaded_file(upload_dir):
    logging.debug("All environment variables: %s" % os.environ)
    logging.debug("CONTENT_LENGTH: %s" % os.environ.get("CONTENT_LENGTH", "unset"))
    logging.debug("CONTENT_TYPE: %s" % os.environ.get("CONTENT_TYPE", "unset"))
    logging.debug("REQUEST_METHOD: %s" % os.environ.get("REQUEST_METHOD", "unset"))

    # Check content length against client_max_body_size from environment
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    # get and check content type to either read string or
    raw_input = sys.stdin.buffer.read(content_length)
    logging.debug("Raw stdin length: %d" % len(raw_input))

    # Parse form data
    # try:
    #     buffer = io.BytesIO(raw_input)
    #     # form = cgi.FieldStorage(fp=buffer, environ=os.environ)
    #     environ = dict(os.environ)
    #     logging.debug(environ)
    #     # environ['CONTENT_LENGTH'] = str(len(raw_input))
    #     form = cgi.FieldStorage(fp=buffer, environ=environ, keep_blank_values=True)
    #     # logging.debug("Form parsed, keys: %s" % list(form.keys()))
    # except Exception as e:
    #     logging.debug("FieldStorage error: %s" % str(e))
    #     send_json_response(500, "Internal Server Error", ERROR_MESSAGES[500])

    # # Validate file field
    # if "file" not in form or not form["file"].filename:
    #     send_json_response(400, "Bad Request", ERROR_MESSAGES[400])
    # file_item = form["file"]

    # Prepare upload directory
    abs_upload_dir = os.path.abspath(upload_dir)
    logging.debug("Absolute upload directory: %s" % abs_upload_dir)

    if not os.path.exists(abs_upload_dir):
        try:
            os.makedirs(abs_upload_dir, 0o755)
            logging.debug("Created upload directory: %s" % abs_upload_dir)
        except OSError as e:
            logging.debug("Error creating upload directory %s: %s" % (abs_upload_dir, str(e)))
            send_json_response(500, "Internal Server Error", ERROR_MESSAGES[500])

    # Check directory writability
    if not os.access(abs_upload_dir, os.W_OK):
        send_json_response(403, "Forbidden", "Forbidden: No Write Vibes Here!")

    # Set target filename and check for conflict
    filename = os.environ.get("FILE_NAME")
    full_filename = os.path.join(abs_upload_dir, os.path.basename(filename))
    logging.debug("Target filename: %s" % full_filename)
    if os.path.exists(full_filename):
        send_json_response(409, "Conflict", ERROR_MESSAGES[409])

    # Simulate server overload (example condition) -- to take out before submission
    # if len(os.listdir(abs_upload_dir)) > 100:  # Adjustable threshold 4 testing
    #     send_json_response(503, "Service Unavailable", ERROR_MESSAGES[503])

    # Process and save the file
    try:
        # file_data = file_item.file.read()
        # logging.debug("File data size: %d bytes" % len(file_data))
        with open(full_filename, "wb") as f:
            bytes_written = f.write(raw_input)
            logging.debug("Bytes written: %d" % bytes_written)
            f.flush()
            os.fsync(f.fileno())
        logging.debug("File exists after write: %s" % os.path.exists(filename))

        # Success response
        send_json_response(200, "OK", "Upload Grooved to Perfection, Baby!")
    except Exception as e:
        logging.debug("Error processing file: %s" % str(e))
        send_json_response(500, "Internal Server Error", ERROR_MESSAGES[500])

if __name__ == "__main__":
    logging.debug("sys.argv: %s" % sys.argv)
    upload_dir = sys.argv[1] if len(sys.argv) > 1 else "./www2/upload"
    logging.debug("Upload directory: %s" % upload_dir)

    # 501 Not Implemented for non-upload actions (example)
    if len(sys.argv) > 2 and sys.argv[2] == "not_implemented":
        send_json_response(501, "Not Implemented", ERROR_MESSAGES[501])

    # 504 Gateway Timeout (simulated)
    if len(sys.argv) > 2 and sys.argv[2] == "timeout":
        send_json_response(504, "Gateway Timeout", ERROR_MESSAGES[504])

    save_uploaded_file(upload_dir)
