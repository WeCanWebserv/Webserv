#! /usr/bin/env python3

import cgi
import cgitb
import os

def error():
    print("Content-Type: application/json", end="\r\n")
    print("\r\n")
    print("""
    {
      "error": "filename not found"
    }
    """)

def main():
    cgitb.enable()

    form = cgi.FieldStorage()
    if "file" in form:
        file = form["file"]
        if file.filename:
            filename = "./" + os.path.basename(file.filename)
            open(filename, "wb").write(file.file.read())

            print("Content-Type: application/json", end="\r\n")
            print("\r\n")
            print(f"""
            {'{'}
            "message": "{filename} file uploaded success"
            {'}'}
            """)
            return
    error()
    return


if __name__ == "__main__":
    main()
