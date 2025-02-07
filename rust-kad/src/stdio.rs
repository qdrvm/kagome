use futures::SinkExt;
use futures::StreamExt;
use parity_scale_codec::Decode;
use parity_scale_codec::Encode;
use tokio::io::BufReader;
use tokio::io::BufWriter;
use tokio::sync::mpsc::unbounded_channel;
use tokio::sync::mpsc::UnboundedSender;
use tokio_fd::AsyncFd;
use tokio_util::codec::FramedRead;
use tokio_util::codec::FramedWrite;
use tokio_util::codec::LengthDelimitedCodec;

fn codec() -> LengthDelimitedCodec {
    LengthDelimitedCodec::builder().little_endian().new_codec()
}

pub struct Stdin(FramedRead<BufReader<AsyncFd>, LengthDelimitedCodec>);
impl Stdin {
    pub fn new() -> anyhow::Result<Self> {
        let fd = AsyncFd::try_from(libc::STDIN_FILENO)?;
        Ok(Self(FramedRead::new(BufReader::new(fd), codec())))
    }
    pub async fn read<T: Decode>(&mut self) -> anyhow::Result<T> {
        let buf = self
            .0
            .next()
            .await
            .ok_or_else(|| anyhow::anyhow!("stdin eof"))??;
        Ok(T::decode(&mut &buf[..])?)
    }
}

pub struct Stdout(UnboundedSender<Vec<u8>>);
impl Stdout {
    pub fn new() -> anyhow::Result<Self> {
        let fd = AsyncFd::try_from(libc::STDOUT_FILENO)?;
        let (tx, mut rx) = unbounded_channel::<Vec<u8>>();
        tokio::spawn(async move {
            let mut framed = FramedWrite::new(BufWriter::new(fd), codec());
            while let Some(v) = rx.recv().await {
                framed.send(v.into()).await?;
            }
            Ok::<(), anyhow::Error>(())
        });
        Ok(Self(tx))
    }
    pub fn write<T: Encode>(&self, v: T) -> anyhow::Result<()> {
        Ok(self.0.send(v.encode())?)
    }
}
