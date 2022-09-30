#! /usr/bin/env python3

import cgi
import cgitb
import os

def error(code, msg):
    print(f"Status: {code}\r")
    print("Content-Type: application/json\r")
    print("\r")
    print(f"""
    {'{'}
      "error": "{msg}"
    {'}'}
    """)

def main():
    cgitb.enable()

    method = os.environ["REQUEST_METHOD"]
    root = os.environ["DOCUMENT_ROOT"]

    form = cgi.FieldStorage()

    if method == "POST":
        if "file" in form:
            file = form["file"]
            if file.filename:
                basename = os.path.basename(file.filename)
                filename = root + "/" + basename
                with open(filename, "wb") as f:
                    f.write(file.file.read())
                    print("Status: 201\r")
                    print(f"Location: /cgi-bin/{basename}\r")
                    print("Content-Type: application/json\r")
                    print("\r")
                    print(f"""
                    {'{'}
                    "message": "{filename} file uploaded success,"
                    "uri": "{os.environ["DOCUMENT_URI"]}"
                    {'}'}
                    """)
                    return
        error(404, "filename not found")
        return
    elif method == "DELETE":
        target = ""
        if "QUERY_STRING" in os.environ:
            target = os.environ["QUERY_STRING"]
        target = target.split('=')[1]
        if target:
            target = os.environ["DOCUMENT_ROOT"] + target
            if os.path.isfile(target):
                try:
                    os.remove(target)
                    print("Status: 204\r")
                    print("\r")
                except BaseException as e:
                    error(403, e)
                    return
            else:
                error(403, "forbidden")
                return
        error(404, f"not found {target}")
        return
    else:
        error(403, f"{method} method forbidden")
        return


if __name__ == "__main__":
    main()
