# Parachain Runtimes Test

The test is implemented at `wavm_module_init_test` target.

It relies on a set of `.wasm` files to be executed during the run.

All you need to run the test is to download these parachain runtimes for the test.

This could be done via small helper script placed here `para-code.ts`.

In order to use it, you need `npm` installed on your system.

The first launch should be preceded with execution of `npm install` within the script directory.

### Downloading Network's Parachains

Let's take for example Polkadot as a target network.

To download all polkadot's parachains you need to figure out RPC accessible address of one of the network nodes.

For example, it may be `wss://polkadot-rpc.dwellir.com`.

One of the way to do it is to use network explorer at https://polkadot.js.org/apps

Then you have to put the address at line 5 of `para-code.ts` and change the `DIR` constant on line 7 if you'd like to.

The last step is to just execute `npm start`.

As the result, you'll have a set of wasm files downloaded, which you may put then to `../runtime_paths.hpp` and use in
`wavm_module_init_test.cpp`.