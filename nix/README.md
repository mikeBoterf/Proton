# Building this Proton fork on NixOS

This `nix/` layer is **tooling only** — it does not change how Proton builds. The
shippable Proton is produced by Proton's own `make redist`; Nix just provides a
reproducible toolchain and a fast way to verify the Wine patches compile.

See `nix/wine-patches/` for the Elytra compatibility patch series and its
provenance, and `backlog/` (Backlog.md) for tasks/milestones and status.

## One-time setup

```sh
git submodule update --init wine        # Valve wine @ dc26e6184
```

## Build the real Proton (what ships)

Proton's build runs host-side orchestration scripts with `#!/bin/bash`, which
NixOS lacks, and it drives Docker. Run the whole thing inside an FHS shell
(`steam-run`) so `/bin/bash` exists and Docker stays reachable:

```sh
# 1. apply the Elytra Wine patches into the wine/ submodule tree
( cd wine && git apply ../nix/wine-patches/000*.patch )

# 2. (optional but recommended) pre-fetch the fixed runtime blobs so the build
#    never depends on network mid-way (steam-run's sandbox can hiccup on DNS).
#    Versions come from Makefile.in (GECKO_VER / WINEMONO_VER / XALIA_VER).
( cd contrib
  wget -nc https://dl.winehq.org/wine/wine-gecko/2.47.4/wine-gecko-2.47.4-x86_64.tar.xz
  wget -nc https://dl.winehq.org/wine/wine-gecko/2.47.4/wine-gecko-2.47.4-x86.tar.xz
  wget -nc https://github.com/madewokherd/wine-mono/releases/download/wine-mono-11.2.0/wine-mono-11.2.0-x86.tar.xz
  wget -nc https://github.com/madewokherd/xalia/releases/download/xalia-0.4.9/xalia-0.4.9-net48-mono.zip )

# 3. build a redistributable Proton (pulls the steamrt SDK image, then compiles
#    wine + dxvk + vkd3d + lsteamclient). Long; Docker must be usable (docker group).
#    --docker-opts DNS is REQUIRED on NixOS: containers inherit systemd-resolved's
#    127.0.0.53 and cannot resolve hostnames, so in-container downloads (piper's
#    fmt/spdlog/piper-phonemize) fail without it. configure once with the opts,
#    then `make redist` reuses them (build/<name>/Makefile persists).
( cd build/build-wardogs-elytra 2>/dev/null || { mkdir -p build/build-wardogs-elytra && cd build/build-wardogs-elytra; }
  [ -e Makefile ] || steam-run bash ../../configure.sh --build-name=wardogs-elytra \
      --enable-ccache --docker-opts="--dns 1.1.1.1 --dns 8.8.8.8" )
steam-run make redist build_name=wardogs-elytra

# 4. install for Steam
cp -r build/wardogs-elytra ~/.steam/root/compatibilitytools.d/
#    (restart Steam; select "wardogs-elytra" under the title's Compatibility)
```

`build/` is gitignored. To reset the wine tree: `cd wine && git checkout -- . && git clean -fdq`.

## Fast patch verification (optional, tooling)

`nix build .#wine-proton` builds just the patched Wine via nixpkgs' recipe to
confirm the patches compile and land in the binaries. It is a **WoW64 standalone
Wine**, not a drop-in for Proton's split-arch runtime — do not try to overlay it
onto a base Proton. Use it only as a compile check; ship via `make redist` above.

## Enter the toolchain shell

```sh
nix develop        # mingw x64+x86, autotools, flex/bison, nasm, git, make
```
