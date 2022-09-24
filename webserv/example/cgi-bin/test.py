#! /usr/bin/env python3

import cgi
import cgitb
import sys

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
        fileitem = form["file"]
        data = sys.stdin.buffer.read()
        open(fileitem.filename, "wb").write(data)

        print("Content-Type: application/json", end="\r\n")
        print("\r\n")
        print(data)
        print("""
        {
          "message": "{fileitem.filename} file uploaded success"
        }
        """)

    else:
        error()
        return


if __name__ == "__main__":
    main()
