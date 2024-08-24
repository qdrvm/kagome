use libp2p::identify;
use libp2p::kad;
use libp2p::kad::store::MemoryStore;
use libp2p::noise;
use libp2p::ping;
use libp2p::swarm::NetworkBehaviour;
use libp2p::tcp;
use libp2p::yamux;
use libp2p::Multiaddr;
use libp2p::PeerId;
use libp2p::StreamProtocol;
use libp2p::Swarm;
use std::time::Duration;

pub fn get_addr_peer(addr: &Multiaddr) -> Option<PeerId> {
    addr.iter().find_map(|x| match x {
        libp2p::multiaddr::Protocol::P2p(x) => Some(x),
        _ => None,
    })
}

#[derive(NetworkBehaviour)]
pub struct Behaviour {
    id: identify::Behaviour,
    pub kad: kad::Behaviour<MemoryStore>,
    ping: ping::Behaviour,
}

pub async fn swarm(config: &super::Config) -> anyhow::Result<Swarm<Behaviour>> {
    let kad_protocols = config
        .kad_protocols
        .iter()
        .cloned()
        .map(StreamProtocol::try_from_owned)
        .collect::<Result<_, _>>()?;
    let mut swarm = libp2p::SwarmBuilder::with_new_identity()
        .with_tokio()
        .with_tcp(
            tcp::Config::default(),
            noise::Config::new,
            yamux::Config::default,
        )?
        .with_dns()?
        .with_websocket(noise::Config::new, yamux::Config::default)
        .await?
        .with_behaviour(|key| {
            let id = identify::Behaviour::new(identify::Config::new(
                identify::PROTOCOL_NAME.to_string(),
                key.public(),
            ));
            let mut kad_cfg = kad::Config::new(kad::PROTOCOL_NAME);
            #[allow(deprecated)]
            kad_cfg.set_protocol_names(kad_protocols);
            let kad = kad::Behaviour::with_config(
                key.public().to_peer_id(),
                MemoryStore::new(key.public().to_peer_id()),
                kad_cfg,
            );
            let ping = ping::Behaviour::new(ping::Config::new());
            Behaviour { kad, id, ping }
        })?
        .with_swarm_config(|c| {
            c.with_idle_connection_timeout(Duration::from_secs(config.connection_idle as _))
        })
        .build();
    for addr_str in &config.bootstrap {
        let addr: Multiaddr = addr_str.parse()?;
        let peer =
            get_addr_peer(&addr).ok_or_else(|| anyhow::anyhow!("config.bootstrap {addr_str:?}"))?;
        swarm.behaviour_mut().kad.add_address(&peer, addr);
    }
    swarm.behaviour_mut().kad.bootstrap()?;
    Ok(swarm)
}
