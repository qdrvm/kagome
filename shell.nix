{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  buildInputs = [
    pkgs.gcc
    # pkgs.clang
    # pkgs.libclang
    pkgs.cmake
    pkgs.boost
    pkgs.boost-build
    pkgs.zlib
    pkgs.python3
    pkgs.python3Packages.pip
    pkgs.curl
    pkgs.cacert # Provide SSL certificates
    # pkgs.openssl
    # pkgs.wget
    pkgs.gmp.dev
    pkgs.ncurses
    pkgs.libnsl
    pkgs.libseccomp
    # pkgs.elfutils
    # pkgs.libbfd
    # pkgs.libdwarf
    pkgs.which
    # pkgs.pkgsCross.mingw32.buildPackages.gcc
  ];
  configureFlags = [ "--with-boost-libdir=${pkgs.boost}/lib" ];
  # Ensure the include paths for the standard C++ library are available.
  shellHook = ''
    # export CXX=clang
    unset CROSS_COMPILE
    # unset CC
    # unset CXX

    # export CXXFLAGS="-I${pkgs.stdenv.cc.cc.lib}/include -I${pkgs.pkgsCross.mingw32.boost}/include"
    # export LDFLAGS="-L${pkgs.stdenv.cc.cc.lib}/lib -L${pkgs.libdwarf}/lib -L${pkgs.pkgsCross.mingw32.boost}/lib"
    # export LDFLAGS="-L${pkgs.stdenv.cc.cc.lib}/lib -L${pkgs.libdwarf}/lib -L${pkgs.boost}/include -L${pkgs.boost-build}/include"
    # export CXXFLAGS="-I${pkgs.stdenv.cc.cc.lib}/include -I${pkgs.libdwarf}/include -I${pkgs.boost}/include -I${pkgs.boost-build}/include"
    export LDFLAGS="-L${pkgs.stdenv.cc.cc.lib}/lib -L${pkgs.boost}/include -L${pkgs.boost-build}/include"
    export CXXFLAGS="-I${pkgs.stdenv.cc.cc.lib}/include -I${pkgs.boost}/include -I${pkgs.boost-build}/include"
  '';
}
