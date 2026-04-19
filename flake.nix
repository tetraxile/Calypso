{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    fenix = {
      url = "github:nix-community/fenix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    self.submodules = true;
  };
  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      fenix,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [
            fenix.overlays.default
          ];
        };
        build = import ./client/sys/nix pkgs;
      in
      with pkgs;
      rec {
        inherit self;
        formatter = pkgs.nixfmt-tree;
        packages = {
          build-sail = stdenv.mkDerivation {
            name = "build-sail";
            dontUnpack = true;
            nativeBuildInputs = [ makeWrapper ];
            installPhase = ''
              makeWrapper ${writeShellScript "build-sail-inner" ''
                cmake -S client/sys/hakkun/sail -B client/sys/hakkun/sail/build
                cmake --build client/sys/hakkun/sail/build --parallel
              ''} $out/bin/build-sail \
                --prefix PATH : ${
                  lib.makeBinPath [
                    cmake
                    gnumake
                    llvmPackages_20.clang
                  ]
                }
            '';
          };
          cmake = writeShellScriptBin "cmake" "nix develop .#cpp --command bash -c $@";
        };
        devShells = {
          default = mkShell rec {
            nativeBuildInputs = build.dependencies ++ [
              inetutils
              pkgs.fenix.complete.toolchain
              openssl
              pkg-config
              libxkbcommon
              libGL
              udev
              openssl
              fontconfig
              packages.cmake

              # WINIT_UNIX_BACKEND=wayland
              wayland

              # WINIT_UNIX_BACKEND=x11
              libxcursor
              libxrandr
              libxi
              libx11
            ];
            LD_LIBRARY_PATH = lib.makeLibraryPath (
              nativeBuildInputs
              ++ [
                llvmPackages_20.clang-unwrapped.lib
                fontconfig.lib
                dbus.lib
              ]
            );
          };
          cpp =
            mkShellNoCC.override
              {
                stdenv = pkgsCross.aarch64-embedded.llvmPackages_20.stdenv;
              }
              rec {
                nativeBuildInputs = build.dependencies ++ [
                  cmake
                  pkg-config
                ];
                LD_LIBRARY_PATH = lib.makeLibraryPath (
                  nativeBuildInputs
                  ++ [
                    llvmPackages_20.clang-unwrapped.lib
                  ]
                );
              };
        };
      }
    );
}
