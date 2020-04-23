import requests


def main():
    url = "http://localhost:40363"

    send = {"id": 1, "jsonrpc": "2.0", "method": "author_submitExtrinsic", "params": [
        "0x2d0284ffd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d0136954fd254651d3ca3cafaa9644f80aa4d17b326101b1e646d7e94424c98d4195306df88c156cb184995f6e0dea02775c56cba98b14ecdfb3fc5b9e4dd28b88a0000000600ff8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a10f"]}
    response = requests.post(url, json=send)

    print("Extrinsic submitted. Response: ", response.json())


if __name__ == "__main__":
    main()
