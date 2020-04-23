import requests


def main():
    url = "http://localhost:40363"

    # Example echo method
    get = {
        "method": "state_getStorage",
        "params": [
            "0x7f864e18e3dd8b58386310d2fe0919eef27c6e558564b7f67f22d99d20f587bb"],
        "jsonrpc": "2.0",
        "id": 0,
    }

    response = requests.post(url, json=get)

    res = response.json()['result']
    print("Alice's balance is ", int.from_bytes(res, byteorder='little', signed=False))


if __name__ == "__main__":
    main()
