fn main() {
    let wasm = std::fs::read("test-wasm.wasm").unwrap();
    let compile = |bulk| {
        let mut config = wasmtime::Config::new();
        config.wasm_bulk_memory(bulk);
        config.wasm_reference_types(false);
        let engine = wasmtime::Engine::new(&config).unwrap();
        let module = wasmtime::Module::from_binary(&engine, &wasm);
        if bulk {
            assert!(module.is_ok());
            Some(module.unwrap())
        } else {
            assert!(module.is_err());
            None
        }
    };
    compile(false);
    let module = compile(true).unwrap();
    let mut store = wasmtime::Store::new(module.engine(), ());
    let instance = wasmtime::Instance::new(&mut store, &module, &[]).unwrap();
    let f = instance
        .get_typed_func::<(i32, i32), (i64,)>(&mut store, "test")
        .unwrap();
    let memory = instance.get_memory(&mut store, "memory").unwrap();
    assert_eq!(memory.data(&mut store)[..4], [0, 0, 0, 0]);
    f.call(&mut store, (0, 0)).unwrap();
    assert_eq!(memory.data(&mut store)[..4], [1, 1, 1, 1]);
    println!("done");
}
