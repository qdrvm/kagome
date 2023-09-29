// run (option 1): npm start
// run (option 2): node -r @swc-node/register kusama-para-code.ts

// choose url from https://polkadot.js.org/apps
const provider = new WsProvider("wss://kusama-rpc.dwellir.com");

const DIR = "kusama-para-code";

import { ApiPromise, WsProvider } from "@polkadot/api";
import { hexStripPrefix, hexToU8a as unhex } from "@polkadot/util/hex";
import type { Bytes, Option } from "@polkadot/types";
import * as Fs from "fs";
import * as Path from "path";

async function main() {
  const api = await ApiPromise.create({ provider });
  const hashes = (await api.query.paras.codeByHash.keys())
    .map((x) => x.args[0].toJSON() as string);
  Fs.mkdirSync(DIR, { recursive: true });
  for (const hash of hashes) {
    const path = Path.join(DIR, `${hexStripPrefix(hash)}.wasm`);
    if (Fs.existsSync(path)) {
      continue;
    }
    console.info("fetch %s", hash);
    const code = (await api.query.paras.codeByHash(hash) as Option<Bytes>)
      .unwrap();
    const tmp = path + ".tmp";
    Fs.writeFileSync(tmp, code);
    Fs.renameSync(tmp, path);
  }
}

main().catch(console.error).finally(() => process.exit());
