#!/usr/bin/env -S deno run -A

import { assertEquals } from "jsr:@std/assert/equals";

// wat2wasm test-wasm.wat
const wasm = Deno.readFileSync("test-wasm.wasm");

const module = await WebAssembly.compile(wasm);
console.info("exports", WebAssembly.Module.exports(module));
const instance: any = await WebAssembly.instantiate(module);
const read = () => new Uint8Array(instance.exports.memory.buffer, 0, 4);
assertEquals(read(), Uint8Array.of(0, 0, 0, 0));
instance.exports.test();
assertEquals(read(), Uint8Array.of(1, 1, 1, 1));
console.info("ok");
