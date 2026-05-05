# Installation

**Note**: For Windows and macOS, installation is not required to use Porymap. You can download the latest release to begin using Porymap immediately.

 - [Download Porymap for Windows](https://github.com/huderlem/porymap/releases/latest/download/porymap-windows.zip).
 - [Download Porymap for macOS latest (arm)](https://github.com/huderlem/porymap/releases/latest/download/porymap-macos-latest.zip).
 - [Download Porymap for macOS 15 (intel)](https://github.com/huderlem/porymap/releases/latest/download/porymap-macos-15-intel.zip).


For installation, Porymap requires Qt 5.14.2 & C++11.

## macOS

The easiest way to get Qt is through [homebrew](https://brew.sh/). 
Once homebrew is installed, run these commands in Terminal:

```bash
xcode-select --install

brew update
brew upgrade
brew install qt

git clone https://github.com/huderlem/porymap
cd porymap

qmake
make

./porymap.app/Contents/MacOS/porymap
```

## Windows

Install [Qt development tools](https://www.qt.io/download-qt-installer), and use Qt Creator, the official Qt IDE, for development purposes.

## Ubuntu

You need to install Qt. The minimum supported version is currently Qt 5.14.2. You can check your Qt version
with `qtdiag` or `qmake --version`.

```bash
sudo apt-get install qt6-declarative-dev
# if your distro does not have qt6-declarative-dev, try sudo apt-get install qtdeclarative5-dev

git clone https://github.com/huderlem/porymap
cd porymap

qmake
make
./porymap
```

## Arch Linux

You need to install Qt. You can check the version of your Qt packages with `qtdiag` or `qmake --version`.

```bash
sudo pacman -S qt6-declarative qt6-charts

git clone https://github.com/huderlem/porymap
cd porymap

qmake
make
./porymap
```

## NixOS

You need to create an instructions file for compiling the package with Qt and wrapping it as a Qt app. This instructions file can then be used for nix-build or referenced within the shell.nix file for your project.

Example `porymap.nix` instructions file:
```nix
{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation rec {
  pname = "porymap";
  version = "6.3.1";

  src = pkgs.fetchFromGitHub {
    owner = "huderlem";
    repo = "porymap";
    rev = version;
    sha256 = "sha256-EG09aOgJrIe5X+e/SKcZn+mxkZ2N4mBmRxlEV3LYvgo=";
  };

  nativeBuildInputs = with pkgs; [
    git
    qt6.qtdeclarative
    qt6.qtcharts
    qt6.wrapQtAppsHook
  ];

  buildInputs = with pkgs; [
    qt6.qtdeclarative
    qt6.qtcharts
  ];

  installPhase = ''
    mkdir -p $out/bin
    cp porymap $out/bin/
    wrapQtApp $out/bin/porymap  # Wrap the binary to include Qt plugins
  '';

  buildPhase = ''
    qmake
    make -j$NIX_BUILD_CORES
  '';
}
```
