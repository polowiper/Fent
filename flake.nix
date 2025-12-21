{
  description = "A basic C++ devshell for aoc";

  inputs.nixpkgs.url = "nixpkgs/nixpkgs-unstable";

  outputs = {
    self,
    nixpkgs,
    ...
  }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs {
      inherit system;
    };

    clang-tools-wrapped = pkgs.symlinkJoin {
      name = "clang-tools-wrapped";
      paths = [pkgs.clang-tools];
      buildInputs = [pkgs.makeWrapper];
      postBuild = ''
        # Remove all symlinks and the original clangd binary
        rm $out/bin/*

        # Create wrapper for clangd with custom environment
        makeWrapper ${pkgs.lib.getExe' pkgs.clang-tools "clangd"} $out/bin/clangd \
          --set CPATH "${pkgs.lib.makeSearchPathOutput "dev" "include" [pkgs.libgcc]}" \
          --set CPLUS_INCLUDE_PATH "${pkgs.stdenv.cc.cc}/include/c++/${pkgs.stdenv.cc.cc.version}:${pkgs.stdenv.cc.cc}/include/c++/${pkgs.stdenv.cc.cc.version}/x86_64-unknown-linux-gnu:${pkgs.lib.getDev pkgs.stdenv.cc.libc}/include" \
          --prefix PATH : ${pkgs.lib.makeBinPath [pkgs.clang-tools]}

        # Recreate all other symlinks pointing to the original clangd binary
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-format
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-tidy
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-apply-replacements
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-change-namespace
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-check
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-doc
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-extdef-mapping
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-include-cleaner
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-include-fixer
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-installapi
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-linker-wrapper
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-move
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-nvlink-wrapper
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-offload-bundler
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-offload-packager
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-query
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-refactor
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-reorder-fields
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-repl
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-scan-deps
        ln -s ${pkgs.clang-tools}/bin/clangd $out/bin/clang-sycl-linker
      '';
    };
  in {
    devShells.${system}.default =
      pkgs.mkShell.override
      {
        stdenv = pkgs.clangStdenv;
      }
      {
        packages = with pkgs; [
          libgcc
          cmake
          clang-tools-wrapped
          nasm
          asmrepl
          bear
          gnumake
          gdb
        ];
        shellHook = ''
          echo "C++ devShell"
        '';
      };
  };
}
