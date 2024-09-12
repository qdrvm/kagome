use futures::StreamExt;
use libp2p::kad;
use libp2p::swarm::SwarmEvent;
use net::BehaviourEvent;
use parity_scale_codec::Decode;
use parity_scale_codec::Encode;
use std::collections::hash_map::Entry;
use std::collections::HashMap;

mod net;
mod stdio;

#[derive(Decode, Encode)]
struct Config {
    kad_protocols: Vec<String>,
    bootstrap: Vec<String>,
    quorum: u32,
    connection_idle: u32,
}
#[derive(Decode, Encode)]
struct Request {
    key: Vec<u8>,
    put_value: Option<Vec<u8>>,
}
#[derive(Decode, Encode)]
struct Response {
    key: Vec<u8>,
    values: Vec<Vec<u8>>,
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> anyhow::Result<()> {
    _ = tracing_subscriber::fmt()
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
        .try_init();
    let mut stdin = stdio::Stdin::new()?;
    let stdout = stdio::Stdout::new()?;
    let config: Config = stdin.read().await?;
    let mut swarm = net::swarm(&config).await?;
    let mut responses: HashMap<kad::QueryId, Response> = Default::default();
    let mut queries: HashMap<Vec<u8>, kad::QueryId> = Default::default();
    loop {
        tokio::select! {
            request = stdin.read::<Request>() => {
                let Request { key, put_value } = request?;
                if let Some(value) = put_value {
                    _ = swarm.behaviour_mut().kad.put_record(kad::Record::new(key.clone(),value.clone()), kad::Quorum::All);
                } else if let Entry::Vacant(entry) = queries.entry(key.clone()) {
                    let id = swarm.behaviour_mut().kad.get_record(key.clone().into());
                    entry.insert(id);
                    responses.insert(
                        id,
                        Response {
                            key,
                            values: vec![],
                        },
                    );
                }
            },
            event = swarm.select_next_some() => match event {
                SwarmEvent::Behaviour(BehaviourEvent::Kad(
                    kad::Event::OutboundQueryProgressed {
                        id,
                        result: kad::QueryResult::GetRecord(result),
                        step,
                        ..
                    },
                )) => {
                    if let Ok(kad::GetRecordOk::FoundRecord(record)) = result {
                        let mut response = responses.get_mut(&id);
                        if let Some(response) = &mut response {
                            if !response.values.contains(&record.record.value) {
                                response.values.push(record.record.value);
                            }
                        }
                        if response.is_none() || step.count.get() >= config.quorum as _ {
                            if let Some(mut query) = swarm.behaviour_mut().kad.query_mut(&id) {
                                query.finish();
                            }
                        }
                    }
                    if step.last {
                        if let Some(response) = responses.remove(&id) {
                            queries.remove(&response.key);
                            stdout.write(response)?;
                        }
                    }
                }
                _ => {}
            },
        }
    }
}
