#!/usr/bin/env -S deno run --allow-all

import {
  basename,
  dirname,
  fromFileUrl,
  join,
} from "https://deno.land/std@0.146.0/path/mod.ts";
import { assert } from "https://deno.land/std@0.146.0/testing/asserts.ts";
import { existsSync } from "https://deno.land/std@0.146.0/fs/exists.ts";
import {
  blue,
  cyan,
  magenta,
  yellow,
} from "https://deno.land/std@0.146.0/fmt/colors.ts";
import {
  BufWriterSync,
  readLines,
} from "https://deno.land/std@0.146.0/io/mod.ts";

function rm(path: string) {
  try {
    Deno.removeSync(path, { recursive: true });
  } catch (e) {
    if (!(e instanceof Deno.errors.NotFound)) throw e;
  }
}

const DIR = dirname(fromFileUrl(import.meta.url));
const LOG = "log.log";

const KAGOME = join(DIR, "kagome");
const POLKADOT = join(DIR, "polkadot");
const _JSON = join(DIR, "testchain4v-fast-4.json");
assert(existsSync(_JSON));
const NETWORK = JSON.parse(Deno.readTextFileSync(_JSON)).id;

class Validator {
  dir: string;
  constructor(public name: string) {
    this.dir = join(DIR, name);
    assert(existsSync(this.dir));
  }
  _run(o: Deno.RunOptions) {
    const file = new BufWriterSync(Deno.openSync(join(this.dir, LOG), {
      create: true,
      write: true,
      truncate: true,
    }));
    const p = Deno.run({ ...o, stdout: "piped", stderr: "piped" });
    const utf = new TextEncoder();
    const name = {
      alice: magenta("A"),
      bob: cyan("B"),
      charlie: blue("C"),
      dave: yellow("D"),
    }[this.name]!;
    async function log(r: Deno.Reader) {
      for await (const x of readLines(r)) {
        console.info(`${name}: ${x}`);
        file.writeSync(utf.encode(x + "\n"));
      }
    }
    setInterval(() => file.flush(), 5000);
    log(p.stdout);
    log(p.stderr);
    return p;
  }
  run(i: number, type: "kagome" | "polkadot") {
    const env: Record<string, string> = {};
    const cmd: string[] = [
      type === "kagome" ? KAGOME : POLKADOT,
      "--chain",
      _JSON,
      "--validator",
      "--base-path",
      this.dir,
      "--name",
      this.name,
      "--port",
      `${10001 + i}`,
      "--rpc-port",
      `${11001 + i}`,
      "--ws-port",
      `${12001 + i}`,
      "--prometheus-port",
      `${13001 + i}`,
    ];
    if (type === "kagome") {
      cmd.push("-ldebug");
    } else {
      cmd.push("--no-hardware-benchmarks");

      env.RUST_LOG = [
        "debug",
        ...[
          "wasmtime_cranelift",
          "wasm_overrides",
          "wasm-heap",
          "parachain",
          "offchain-worker",
          "dummy-subsystem",
          "multistream_select",
          "libp2p_ping",
          "substrate_prometheus_endpoint",
          "libp2p_websocket",
          "sc_rpc_server",
          "libp2p_kad",
          "libp2p_mdns",
          "runtime::election-provider",
        ].map((g) => g + "=info"),
      ].join(",");

      const keys = join(this.dir, NETWORK, "keystore");
      function key1(p: string) {
        for (const e of Deno.readDirSync(keys)) {
          if (e.name.startsWith(p)) return join(keys, e.name);
        }
        throw new Error();
      }
      const keys2 = join(this.dir, "chains", NETWORK, "keystore");
      Deno.mkdirSync(keys2, { recursive: true });
      for (const [p, h] of [["babe", "62616265"], ["gran", "6772616e"]]) {
        Deno.writeTextFileSync(
          join(keys2, h + basename(key1(p)).slice(p.length)),
          JSON.stringify(
            "bottom drive obey lake curtain smoke basket hold race lonely fit walk//" +
              this.name[0].toUpperCase() + this.name.slice(1),
          ),
        );
      }
      const keys3 = join(this.dir, "chains", NETWORK, "network");
      Deno.mkdirSync(keys3, { recursive: true });
      Deno.writeTextFileSync(
        join(keys3, "secret_ed25519"),
        Deno.readTextFileSync(key1("lp2p")).trim().replace(/^0x/, ""),
      );
    }
    return this._run({ cmd });
  }
}
const validators = ["alice", "bob", "charlie", "dave"].map((
  name,
) => new Validator(name));

for (const v of validators) {
  rm(join(v.dir, NETWORK, "db"));
  rm(join(v.dir, "chains", NETWORK, "db"));
}
const ps = validators.map((v, i) =>
  v.run(i, v.name === "alice" ? "kagome" : "polkadot")
);
await Promise.all(ps.map(async (p) => {
  try {
    await p.status();
  } catch {}
}));
