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



