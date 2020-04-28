import requests
import sys


def main():
    url = "http://" + sys.argv[1]

    send = {"id": 1, "jsonrpc": "2.0", "method": "author_submitExtrinsic", "params": [
        sys.argv[2]]}
    response = requests.post(url, json=send)

    print("Extrinsic submitted. Response: ", response.json())


if __name__ == "__main__":
    main()
