import requests
import sys

def main():
    url = "http://" + sys.argv[1]

    # Example echo method
    get = {
        "method": "state_getStorage",
        "params": [
            sys.argv[2]],
        "jsonrpc": "2.0",
        "id": 0,
    }

    response = requests.post(url, json=get)

    res = response.json()['result']
    print("Balance is", int.from_bytes(res, byteorder='little', signed=False))


if __name__ == "__main__":
    main()
