{
  description = "A Nix-flake for building Kagome";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/master";

  outputs =
    {
      self,
      nixpkgs,
    }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forEachSupportedSystem =
        f:
        nixpkgs.lib.genAttrs supportedSystems (
          system:
          f {
            pkgs = import nixpkgs { inherit system; };
          }
        );
    in
    {
      devShells = forEachSupportedSystem (
        { pkgs }:
        {
          default =
            pkgs.mkShell.override
              {
                # Override stdenv in order to change compiler:
                # stdenv = pkgs.clangStdenv;
              }
              {
                packages = with pkgs; [
                  clang-tools
                  cmake
                  libseccomp
                  ncurses.dev
                  python3

                  # Rust tools
                  rustup
                  cargo
                  rust-analyzer
                ];

                shellHook = ''
                  export RUSTUP_HOME="$PWD/.rustup"
                  export CARGO_HOME="$PWD/.cargo"
                  export PATH="$CARGO_HOME/bin:$PATH"
                  export FLAKE_INITIATED=1

                  echo "Using RUSTUP_HOME: $RUSTUP_HOME"
                  echo "Using CARGO_HOME: $CARGO_HOME"
                  echo "Current PATH: $PATH"

                  export RUST_VERSION=1.81.0

                  if ! rustup show | grep -q "$RUST_VERSION"; then
                      echo "Installing Rust version $RUST_VERSION"
                          rustup toolchain install $RUST_VERSION
                          rustup default $RUST_VERSION
                  else
                      echo "Rust version $RUST_VERSION already installed and set as default."
                          fi

                          rustup show
                '';

              };
        }
      );
    };
}
