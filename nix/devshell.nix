{
  # Interactive Wine build environment: `nix develop`.
  #
  # Inherits Wine's full dependency closure from nixpkgs' WoW64 Wine, then adds the
  # MinGW cross toolchains (builtin PE DLLs like ntdll.dll are cross-compiled) plus
  # the autotools/lex/yacc that ./configure needs. Use this for the fast edit/make
  # loop on ./wine; use `nix build .#wine-proton` for a reproducible build.
  perSystem = { pkgs, ... }: {
    devShells.default = pkgs.mkShell {
      name = "proton-wine-dev";

      inputsFrom = [ pkgs.wineWow64Packages.unstable ];

      packages = with pkgs; [
        pkgsCross.mingwW64.buildPackages.gcc # x86_64 PE builtins
        pkgsCross.mingw32.buildPackages.gcc # i686 PE builtins
        gcc
        autoconf
        automake
        libtool
        bison
        flex
        gettext
        perl
        pkg-config
        gnumake
        python3
        nasm
        git
      ];

      shellHook = ''
        echo "proton-wine-dev :: Wine build toolchain ready"
        echo "  wine source : ./wine   (git submodule @ dc26e6184)"
        echo "  reproducible : nix build .#wine-proton"
        echo "  fast iterate : cd wine && ./configure --enable-archs=i386,x86_64 && make -j\$(nproc)"
      '';
    };
  };
}
