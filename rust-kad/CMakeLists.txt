
set(KAGOME_RUST_KAD ${CMAKE_BINARY_DIR}/node/kagome-rust-kad)
add_custom_command(
  OUTPUT ${KAGOME_RUST_KAD}
  DEPENDS
    Cargo.lock
    Cargo.toml
    src/main.rs
    src/net.rs
    src/stdio.rs
  COMMAND cargo build --release
  COMMAND cp target/release/rust-kad ${KAGOME_RUST_KAD}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
add_custom_target(kagome_rust_kad DEPENDS ${KAGOME_RUST_KAD})
