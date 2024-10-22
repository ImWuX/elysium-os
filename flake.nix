{
    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
        flake-utils.url = "github:numtide/flake-utils";
    };

    outputs = { nixpkgs, flake-utils, ... } @ inputs: flake-utils.lib.eachDefaultSystem (system:
        let
            pkgs = import nixpkgs { inherit system; };
            inherit (pkgs) lib stdenv mkShell fetchFromGitLab buildGoModule;
        in {
            devShells.default = mkShell {
                shellHook = "export DEVSHELL_PS1_PREFIX='elysium-os'; export PATH=\"$PATH:../chariot\";";
                nativeBuildInputs = with pkgs; [
                    gnumake
                    gdb
                    gcc14
                    qemu_full
                ];
            };
        }
    );
}
