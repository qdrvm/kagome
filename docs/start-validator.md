# KAGOME v0.9.6 Setup guide

:::info
**Note:** This guide assumes you are using KAGOME v0.9.6 or later. Please be aware that data from previous versions is not compatible with the latest version. To migrate your data, please add include the `--enable-db-migration` flag when starting the node.
:::

**Table of Contents**
[[toc]]

# Overview
This is a guide to help you set up a KAGOME node using docker-compose or service files. The guide will walk you through the process of setting up a KAGOME node on your local machine or a remote server.

## Support
For technical support, please join the [KAGOME Discussions](https://github.com/qdrvm/kagome/discussions) on GitHub.

## Prerequisites
This guide assumes you have the knowledge and experience to set up a server and run commands in a terminal. You will need the following software installed on your machine:

- [Ubuntu Server v24.04 LTS](https://ubuntu.com/download/server) or newer, installed and running on your local machine or remote server.

## Recommended Hardware Requirements

We refer to Polkadot-SDK's [reference hardware](https://wiki.polkadot.network/docs/maintain-guides-how-to-validate-polkadot#reference-hardware) for running a validator node. The following are the recommended hardware requirements:

**CPU**
* x86-64 compatible
* Intel Ice Lake, or newer (Xeon or Core series); AMD Zen3, or newer (EPYC or Ryzen)
* 8 physical cores @ 3.4GHz
* Simultaneous multithreading disabled (Hyper-Threading on Intel, SMT on AMD)

**Storage**
* An NVMe SSD of 1 TB (As it should be reasonably sized to deal with blockchain growth). An estimation of current chain snapshot sizes can be found here. In general, the latency is more important than the throughput.

* **Memory**
* 32 GB DDR4 ECC

**System**
* Linux Kernel 5.16 or newer

**Network**
* The minimum symmetric networking speed is set to 500 Mbit/s (= 62.5 MB/s). This is required to support a large number of parachains and allow for proper congestion control in busy network situations.

The specs posted above are not a hard requirement to run a validator, but are considered best practice. Running a validator is a responsible task; using professional hardware is a must in any way. 

## Setting up a KAGOME node
To set up a KAGOME node, you need to obtain the latest version of the KAGOME binary. To do so, you can build KAGOME from source, download pre-built binaries from the [KAGOME GitHub repository](https://github.com/qdrvm/kagome), or install from APK packages.
Alternatively, you can use the provided docker-compose files to run a KAGOME node. If you are using docker-compose, you can skip the rest of this section and move to the Running a KAGOME node part of this guide.

### Building from source

> **Note:** By default, the project is built using `gcc`. Ensure that `gcc` is installed on your system.
> To use a different compiler, update the `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER` variables with the path to corresponding compilers in the `scripts/.env` file.
> KAGOME can be built with `gcc` of version 13 or higher, or `clang` of version 16 or higher.

1. Clone the KAGOME repository:

```sh
git clone https://github.com/qdrvm/kagome.git && cd kagome
```

2. Run installation scripts:

```sh
chmod +x scripts/init.sh scripts/build.sh # might require sudo
./scripts/init.sh # might require sudo
./scripts/build.sh
```

This will build the KAGOME binary and place it in the `build/node` directory. Ensure to add the `build/node` directory to your PATH environment variable.


### Installation from APT package

To install KAGOME releases using the provided package repository, follow these steps (tested on Ubuntu 24.04.1 LTS (Noble Numbat)):

Update your package lists and install necessary utilities:

```sh
apt update && apt install -y gpg curl
```

Add the repository’s GPG signing key:

```sh
curl -fsSL https://europe-north1-apt.pkg.dev/doc/repo-signing-key.gpg | gpg --dearmor -o /usr/share/keyrings/europe-north-1-apt-archive-keyring.gpg
```

Add the KAGOME package repository to your sources list:
```sh
echo "deb [signed-by=/usr/share/keyrings/europe-north-1-apt-archive-keyring.gpg] https://europe-north1-apt.pkg.dev/projects/kagome-408211 kagome main" > /etc/apt/sources.list.d/kagome.list
```

Update the package lists and install KAGOME:

```sh
apt update && apt install -y kagome
```

## Running a KAGOME node

Once you have the KAGOME binary installed on your machine, you can start running a KAGOME node. You can run the node using docker-compose or service files.

:::info
**Note:** To run a KAGOME validator node you need to have a node key. You can generate a node key using the ``kagome key generate-node-key`` command.
:::

:::info
**Note:** To run a KAGOME validator node you need to have a publicly accessible IP address.
:::

### Running a KAGOME node using service files

You can run KAGOME as a service using a systemd service file. Below is an example of a service file to launch KAGOME Kusama validator:

```sh
[Unit]
Description=Kagome Node

[Service]
User=kagome
Group=kagome
# LimitCORE=infinity # Uncomment if you want to generate core dumps 
LimitNOFILE=65536
ExecStart=kagome
  --name kagome-validator \
  --base-path /home/kagome/dev/kagome-fun/kusama-node-1 \
  --public-addr=/ip4/212.11.12.32/tcp/30334 \  # Address should be publicly accessible
  --validator \
  --listen-addr=/ip4/0.0.0.0/tcp/30334 \
  --chain kusama \
  --prometheus-port=9615 \
  --prometheus-external \
  --wasm-execution Compiled \
  --telemetry-url 'wss://telemetry.polkadot.io/submit/ 1' \
  --rpc-port=9944 \
  --node-key 63808171009b35fc218f207442e355b0634561c84e0aec2093e3515113475624 \  # replace with your node key
  --sync Warp # recommended for the first start to quickly sync up the chain, remove it if your node is already synced

Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

#### Adding the Service File

1. Copy the service file content into a new file named kagome.service.
2. Move the file to the systemd directory:

`sudo mv kagome.service /etc/systemd/system/`

#### Managing the Kagome Node

To start the Kagome node using systemd:
1. Reload the systemd manager configuration:
```sh
sudo systemctl daemon-reload
```

2. (Optionally) Enable the Kagome service to start on boot:
```sh
sudo systemctl enable kagome
```

3. Start the Kagome service:
```sh
sudo systemctl start kagome
```

4. Check the status of the Kagome service:
```sh
sudo systemctl status kagome
```

5. To stop the Kagome service:
```sh
sudo systemctl stop kagome
```

### Running a KAGOME node using docker-compose

If you prefer running KAGOME using docker, you can use docker-compose file provided stored in KAGOME repo. To start using it navigate to folder with docker-compose.yaml file and start the node.

> **Note:** Make sure you have docker and docker-compose installed on your machine.

> Before starting the node, make sure to update docker-compose.yaml file with your node's key and public address.

```bash
cd housekeeping/docker-compose/kagome/
docker-compose up -d
```

This will start a KAGOME node in the background. 
You can check the logs using 
```bash
docker-compose logs -f
```

You may connect to your local node via polkadot-js apps by using the following address: https://polkadot.js.org/apps/?rpc=ws%3A%2F%2F127.0.0.1%3A9944#/

To connect to graphana monitoring, use the following address: http://127.0.0.1:3000/

To stop the node: 
```bash
docker-compose down
```

## Setting up session keys

Once your node is synced and running, you can set up session keys to start validating. To set up session keys, you need to invoke ``author_rotateKeys`` RPC method. You can do this using the following command:

```sh
curl -H "Content-Type: application/json" -d '{"id":1, "jsonrpc":"2.0", "method": "author_rotateKeys", "params":[]}' http://localhost:9944
{"jsonrpc":"2.0","id":1,"result":"0xfdd8adfa9839d2b73b8a1be2a737ff01d41e6837415b2fcbbcb3b2795eea75877a28e58509e567c352bf074fa8990cf2635d87c897ab39862a4411c6857b900fd416403da10819c02951d9ff66001d8558f031a0ebec7114386b944d1f50c9426624937c34a92ea55c497aee3b08e85a7d5681f157f2e88e8c964a2df4a9c76bb6f48b591a4f950734df8305d493b8fc68a0defd1e31cc9d4d56ca04274e6f05"}%
```

Result field from response json is a session key. You can assign this key to your validator account using Polkadot JS. Use this guide if you are unfamiliar with the process: [link](https://wiki.polkadot.network/docs/maintain-guides-how-to-validate-polkadot#bond-dot)

