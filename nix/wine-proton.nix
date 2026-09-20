{ inputs, ... }:
{
  # Valve Wine (proton_11.0 @ dc26e6184) + Elytra compatibility patches.
  #
  # We borrow nixpkgs' WoW64 Wine build recipe (toolchain, configureFlags, library
  # inputs) but swap in Valve's source and our patch series. These patches make the
  # REAL Elytra driver run under Wine's kernel emulation — they are not an anti-cheat
  # bypass (see AGENTS.md § Ethics). Expect to iterate configureFlags/patches here as
  # the build and the running game surface further gaps.
  perSystem = { pkgs, ... }:
    let
      # Ordered series — 0002/0003 build on 0001's ntoskrnl changes.
      winePatches = [
        ./wine-patches/0001-ntdll-Add-Wine-driver-support-needed-by-Elytra.patch
        ./wine-patches/0002-bcrypt-Fix-padded-in-place-symmetric-decryption.patch
        ./wine-patches/0003-ntoskrnl-Implement-MmMapLockedPagesSpecifyCache.patch
      ];

      wine-proton = pkgs.wineWow64Packages.unstable.overrideAttrs (old: {
        pname = "wine-proton-elytra";
        version = "proton11-dc26e61-elytra";
        src = inputs.wine-proton-src;
        # Drop nixpkgs' vanilla-Wine patch set — it targets a different tree.
        # Valve's fork carries its own; we add only the Elytra series.
        patches = winePatches;
        # winedmo vendors FFmpeg (libavcodec) source that doesn't compile against
        # nixpkgs' FFmpeg 7.x headers (AVBSFInternal / AVCodecParameters.channels
        # removed upstream). It's in-game media playback, irrelevant to the Elytra
        # driver/session path we care about — disable it.
        configureFlags = (old.configureFlags or [ ]) ++ [ "--without-ffmpeg" ];
        # nixpkgs builds from a release tarball with all generated files present;
        # Valve's git source strips them, so regenerate after patching:
        #   make_vulkan -> include/wine/vulkan.h (referenced by include/Makefile.in,
        #                  makedep fails without it)
        #   autoconf    -> configure
        #   autoheader  -> include/config.h.in
        # aclocal.m4 is in-tree, so no aclocal step is needed.
        nativeBuildInputs = (old.nativeBuildInputs or [ ]) ++ [ pkgs.autoconf pkgs.python3 pkgs.perl ];
        postPatch = (old.postPatch or "") + ''
          # make_requests -> include/wine/server_protocol.h, server/request_*.h
          # make_specfiles -> dlls/ntdll/ntsyscalls.h, dlls/win32u/win32syscalls.h
          # (invoke via perl; the scripts' exec bit/shebang isn't reliable here).
          perl tools/make_requests
          perl tools/make_specfiles
          # make_vulkan defaults to downloading the Khronos registry (no network in
          # the Nix sandbox) and caches under $HOME (read-only /homeless-shelter).
          # Point it at the in-tree vk.xml/video.xml (-x/-X) and give it a writable
          # HOME for its unconditional cache makedirs.
          ( cd dlls/winevulkan && HOME="$TMPDIR" python3 make_vulkan -x vk.xml -X video.xml )
          autoconf
          autoheader
        '';
      });
    in
    {
      packages.wine-proton = wine-proton;
      packages.default = wine-proton;
    };
}
