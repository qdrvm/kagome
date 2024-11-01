{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  buildInputs = [
    pkgs.gcc
    # pkgs.clang
    # pkgs.libclang
    pkgs.cmake
    pkgs.cmake-language-server
    pkgs.boost
    pkgs.boost-build
    pkgs.git
    pkgs.zlib
    pkgs.python3
    pkgs.python3Packages.pip
    pkgs.curl
    pkgs.cacert # Provide SSL certificates
    pkgs.boringssl
    # pkgs.openssl
    # pkgs.wget
    pkgs.gmp.dev
    pkgs.ncurses
    pkgs.libnsl
    pkgs.libseccomp
    pkgs.elfutils
    pkgs.perl
    # NOTE: cross-compiling seems unsupported by windres for aarch64, so disabling
    # pkgs.libbfd
    pkgs.libdwarf
    pkgs.libcxx
    pkgs.which
    # pkgs.pkgsCross.mingw32.buildPackages.gcc
    # TODO: remove!
    pkgs.libnotify

    pkgs.cargo
    pkgs.rustc
  ];
  configureFlags = [ "--with-boost-libdir=${pkgs.boost}/lib" ];
  # Ensure the include paths for the standard C++ library are available.
  shellHook = ''
        export CXX=g++
        export CC=gcc
        # export CXX=clang
        # export CC=clang
        # unset CROSS_COMPILE
        # unset CC
        # unset CXX

        # export CXXFLAGS="-I${pkgs.stdenv.cc.cc.lib}/include -I${pkgs.pkgsCross.mingw32.boost}/include"
        # export LDFLAGS="-L${pkgs.stdenv.cc.cc.lib}/lib -L${pkgs.libdwarf}/lib -L${pkgs.pkgsCross.mingw32.boost}/lib"
        export LDFLAGS="-L${pkgs.stdenv.cc.cc.lib}/lib -L${pkgs.libdwarf}/lib -L${pkgs.boost}/lib -L${pkgs.boost-build}/lib -L${pkgs.boringssl}/lib"
        export CXXFLAGS="-I${pkgs.stdenv.cc.cc.lib}/include -I${pkgs.libdwarf}/include -I${pkgs.boost}/include -I${pkgs.boost-build}/include -I${pkgs.boringssl}/include -Wno-error=stringop-overflow"
    export CCFLAGS=$CXXFLAGS
        # export LDFLAGS="-L${pkgs.stdenv.cc.cc.lib}/lib -L${pkgs.boost}/include -L${pkgs.boost-build}/include"
        # export CXXFLAGS="-I${pkgs.stdenv.cc.cc.lib}/include -I${pkgs.boost}/include -I${pkgs.boost-build}/include"
  '';
}
