{
  description = "Nix (flake-parts, dendritic) devshell + patched-Wine derivation for WARDOGS/Elytra Proton work";

  # Additive to Proton's own build system — does NOT replace their Makefile,
  # build/ tooling, or Docker container. Every file under ./nix is a flake-parts
  # module, auto-imported via import-tree. See AGENTS.md.

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    import-tree.url = "github:vic/import-tree";

    # Valve Wine pinned to the exact commit this repo's `wine` submodule points at
    # (dc26e6184). AstralDrift's three Elytra patches were authored on this commit,
    # so they apply with no rebase.
    wine-proton-src = {
      url = "github:mikeBoterf/wine/dc26e61847081a1b5cb0733dc30feba6ee575482";
      flake = false;
    };
  };

  outputs = inputs@{ flake-parts, import-tree, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } (import-tree ./nix);
}
