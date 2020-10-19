import sys

from substrateinterface import SubstrateInterface

def main():
    url = "http://" + sys.argv[1]

    substrate = SubstrateInterface(
        url=url,
        type_registry_preset='default'
    )

    balance_info = substrate.get_runtime_state(
        module='System',
        storage_function='Account',
        params=[sys.argv[2]]
    ).get('result')

    if balance_info:
        print("\n\nCurrent free balance: {}".format(
            balance_info.get('data').get('free', 0) / 10**12
        ))



if __name__ == "__main__":
    main()
