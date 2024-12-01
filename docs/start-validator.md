# KAGOME v0.9.6 Setup guide

:::info
**Note:** This guide assumes you are using KAGOME v0.9.6 or later. Please be aware that data from previous versions is compatible with the latest version. To migrate your data, please add include the `--enable-db-migration` flag when starting the node.
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

We refer to Polkadot-SDK's [https://wiki.polkadot.network/docs/maintain-guides-how-to-validate-polkadot#reference-hardware) for running a validator node. The following are the recommended hardware requirements:

**CPU**
* x86-64 compatible
* Intel Ice Lake, or newer (Xeon or Core series); AMD Zen3, or newer (EPYC or Ryzen)
* 8 physical cores @ 3.4GHz
* Simultaneous multithreading disabled (Hyper-Threading on Intel, SMT on AMD)
**Storage**
* An NVMe SSD of 1 TB (As it should be reasonably sized to deal with blockchain growth). An estimation of current chain snapshot sizes can be found here. In general, the latency is more important than the throughput.
**Memory**
* 32 GB DDR4 ECC
**System**
* Linux Kernel 5.16 or newer
**Network**
* The minimum symmetric networking speed is set to 500 Mbit/s (= 62.5 MB/s). This is required to support a large number of parachains and allow for proper congestion control in busy network situations.

The specs posted above are not a hard requirement to run a validator, but are considered best practice. Running a validator is a responsible task; using professional hardware is a must in any way. 

## Setting up a KAGOME node
To set up a KAGOME node, you need to obtain the latest version of the KAGOME binary. To do so, you can build KAGOME from source, download pre-built binaries from the [KAGOME GitHub repository](https://github.com/qdrvm/kagome), or install from APK packages.

### Building from source

Follow the guide: [Building from source](https://github.com/qdrvm/kagome?tab=readme-ov-file#build)

### Installation from APT package

Follow the guide: [Installation from APT package](https://github.com/qdrvm/kagome?tab=readme-ov-file#installation-from-apt-package)

## Running a KAGOME node

Once you have the KAGOME binary installed on your machine, you can start running a KAGOME node. You can run the node using docker-compose or service files.

:::info
**Note:** To run a KAGOME validator node you need to have a node key. You can generate a node key using the ``kagome key generate-node-key`` command.
:::

:::info
**Note:** To run a KAGOME validator node you need to have a publicly accessible IP address.
:::

### Running a KAGOME node using docker-compose

[//]: # (TODO)

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
ExecStart=kagome \  # should be in path
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
  --node-key 63808171009b35fc218f207442e355b0634561c84e0aec2093e3515113475624 # replace with your node key

Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```



