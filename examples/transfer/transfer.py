import requests
import sys
from substrateinterface import SubstrateInterface, SubstrateRequestException, Keypair


def main():
    url = "http://" + sys.argv[1]

    substrate = SubstrateInterface(
        url=url,
        type_registry_preset='kusama'
    )

    keypair = Keypair.create_from_seed(sys.argv[2])

    call = substrate.compose_call(
        call_module='Balances',
        call_function='transfer',
        call_params={
            'dest': sys.argv[3],
            'value': int(sys.argv[4]) * 10**12
        }
    )

    extrinsic = substrate.create_signed_extrinsic(call=call, keypair=keypair)

    try:
        result = substrate.submit_extrinsic(extrinsic, wait_for_inclusion=False)
        print("Extrinsic '{}' sent".format(result['extrinsic_hash']))

    except SubstrateRequestException as e:
        print("Failed to send: {}".format(e))


if __name__ == "__main__":
    main()
