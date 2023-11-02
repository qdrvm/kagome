[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

## Runing Kagome in docker container

In this tutorial you will learn how to execute Kagome-based Polkadot-host chain which can be used as a cryptocurrency, and interact with it by sending extrinsics and executing queries.

### Prerequisites

1. Docker container runtime must be installed in your OS. 
   Manual of that can be found in [official site](https://docs.docker.com/engine/install/).

2. Image of kagome docker container is soramitsu/kagome 

   ```bash
   # In Debian-like OS (Debian, Ubuntu, Mint, etc.)
   docker image pull soramitsu/kagome
   ```
   
3. Directory containing configs and keys 

### Launch Kagome applications in docker

#### Brief

```bash
docker run \                        # Run docker container
  --interactive \                   # Interactive mode
  --tty \                           # Allocate pseudo-TTY
  --rm \                            # Remove container (not image) after stop
  --user `id -u`:`id -g` \          # Delegate current user permitions in container 
  --volume `pwd`:/kagome \          # Map current directory to directory in container 
  --publish DOCKER_PORT:HOST_PORT \ # Bind ports
  soramitsu/kagome \                # Name of docker image of Kagome
  APPLICATION \                     # Startup application...  
  ARGUMENTS                         # ...and then its arguments
```

#### Example

For example we want to run syncing node in docker container with configs from [here](first_kagome_chain.md).

Run the following commands: 

```bash
cd examples/first_kagome_chain

docker run \                   
  --interactive \
  --tty \                        
  --rm \                       
  --user `id -u`:`id -g` \          
  --volume `pwd`:/kagome \            
  --publish 30363:30363 \             
  --publish 9933:9933 \
  --publish 9944:9944 \
  soramitsu/kagome \           
  kagome \   
  --chain localchain.json \    
  --base-path base_path \ 
  --port 30363 \
  --rpc-port 9933 \ 
  --ws-port 9944
```

The similar way to run other kagome applications.
