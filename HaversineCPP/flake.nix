{
  description = "Flake for my haversine project";

    inputs = {
      nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
      flake-utils.url = "github:numtide/flake-utils";
    };

    outputs = { self, nixpkgs, flake-utils }:
      flake-utils.lib.eachDefaultSystem (system:
        let
          pkgs = import nixpkgs {
            inherit system;
            config.allowUnfree = true; # needed for CLion
          };

          llvm = pkgs.llvmPackages_latest;

          commonBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config

            llvm.clang
            llvm.lldb
            llvm.lld
            gdb

            ccache
            git

            # optional but commonly useful for CMake projects
            python3
          ];
        in
        {
          packages.default = pkgs.stdenv.mkDerivation {
            pname = "haversine-cpp";
            version = "0.1.0";
            src = ./.;

            nativeBuildInputs = commonBuildInputs;

            # Add library deps here when you want them supplied by Nix, e.g.
            # buildInputs = [ pkgs.fmt pkgs.spdlog ];
            buildInputs = [
             pkgs.argparse
             pkgs.glaze
             ];

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
            ];
          };

          devShells.default = pkgs.mkShell {
            packages = commonBuildInputs ++ [
              pkgs.jetbrains.clion
              pkgs.clang-tools
            ];

            # Good defaults for CLion/CMake/clangd on NixOS
            shellHook = ''
              export CC=${llvm.clang}/bin/clang
              export CXX=${llvm.clang}/bin/clang++
              export CMAKE_GENERATOR=Ninja
              export LD=${llvm.lld}/bin/ld.lld

              # Helps clangd / CLion resolve standard library paths consistently
              export CMAKE_EXPORT_COMPILE_COMMANDS=1

              echo "Dev shell ready."
              echo "Run: clion ."
            '';
          };
        });
  }
