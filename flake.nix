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
      {
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
          setup-client = stdenv.mkDerivation {
            name = "setup-client";
            dontUnpack = true;
            nativeBuildInputs = [ makeWrapper ];
            installPhase = ''
              makeWrapper ${writeShellScript "-inner" ''
                cmake -S client -B build
              ''} $out/bin/setup-client \
                --prefix PATH : ${
                  lib.makeBinPath [
                    cmake
                    gnumake
                    llvmPackages_20.clang-unwrapped
                  ]
                } \
                --prefix CC ${llvmPackages_20.clang-unwrapped}/bin/clang
            '';
          };
        };
        devShells = {
          default =
            mkShell.override
              {
                stdenv = llvmPackages_20.libcxxStdenv;
              }
rec              {
                buildInputs = build.dependencies ++ [
                  inetutils
                  pkgs.fenix.complete.toolchain
                  openssl
                  pkg-config
                  libxkbcommon
                  libGL
                  udev
                  openssl
                  pkg-config
                  fontconfig

                  # WINIT_UNIX_BACKEND=wayland
                  wayland

                  # WINIT_UNIX_BACKEND=x11
                  libxcursor
                  libxrandr
                  libxi
                  libx11
                ];
                LD_LIBRARY_PATH = lib.makeLibraryPath (buildInputs ++[
                  llvmPackages_20.clang-unwrapped.lib
                  fontconfig.lib
                ]);
                NIX_CC_WRAPPER_SUPPRESS_TARGET_WARNING = true;

              };
        };
      }
    );
}
