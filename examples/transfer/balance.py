import sys

from substrateinterface import SubstrateInterface


def main():
    url = "http://" + sys.argv[1]

    substrate = SubstrateInterface(
        url=url,
        type_registry_preset='rococo'
    )

    balance_info = substrate.query(
        module='System',
        storage_function='Account',
        params=[sys.argv[2]]).__getattribute__('value')

    if balance_info:
        print("\n\nCurrent free balance: {}".format(
            balance_info.get('data').get('free', 0) / 10 ** 12
        ))


if __name__ == "__main__":
    main()
