{
  description = "dre - paint the terminal background and exit on ctrl-c";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          packages = [
            pkgs.stdenv.cc
            pkgs.gdb
          ]
          ++ nixpkgs.lib.optionals pkgs.stdenv.hostPlatform.isLinux [ pkgs.strace ];
        };
      });

      packages = forAllSystems (pkgs: {
        default = pkgs.stdenv.mkDerivation {
          pname = "dre";
          version = "0.1.0";
          src = nixpkgs.lib.cleanSourceWith {
            src = ./.;
            filter = path: type:
              let lib = nixpkgs.lib; name = baseNameOf (toString path); in
              name == "main.c" || lib.hasSuffix ".c" name || lib.hasSuffix ".h" name;
          };
          strictDeps = true;
          dontConfigure = true;
          buildPhase = ''
            runHook preBuild
            $CC -O2 -Wall -Wextra -o main main.c text_buffer.c input.c paint.c
            runHook postBuild
          '';
          installPhase = ''
            runHook preInstall
            mkdir -p $out/bin
            install -m 0755 main $out/bin/dre
            runHook postInstall
          '';
          meta = {
            description = "dre terminal painter";
            mainProgram = "dre";
          };
        };
      });

      
    };
}