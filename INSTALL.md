# Installation

**Note**: For Windows and macOS, installation is not required to use Porymap. You can download the latest release to begin using Porymap immediately.

 - [Download Porymap for Windows](https://github.com/huderlem/porymap/releases/latest/download/porymap-windows.zip).
 - [Download Porymap for macOS latest (arm)](https://github.com/huderlem/porymap/releases/latest/download/porymap-macos-latest.zip).
 - [Download Porymap for macOS 13 (intel)](https://github.com/huderlem/porymap/releases/latest/download/porymap-macos-13.zip).


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

## Windows 10+

Install [Qt development tools](https://www.qt.io/download-qt-installer), and use Qt Creator, the official Qt IDE, for development purposes. (for qt6)

## Debian /Linux derivatives (Non Ubuntu with an exception to Mint)
Although porymap can be made on other platforms with Qt 5.14.2, Debian requires qt6

```bash
git clone https://github.com/huderlem/porymap
cd porymap

apt install qt6-declarative-dev 

qmake6
make
./porymap
```


## Gentoo Linux
You need to install Qt dev tools. You can set version of your Qt packages with flags such as `qt6`.
25.04.2 is porymap's minimum supported dev-util/kdevelop version.
https://wiki.gentoo.org/wiki/Qt

```bash
git clone https://github.com/huderlem/porymap
cd porymap

emerge dev-util/kdevelop
emerge --depclean

qmake6
make
./porymap
```

## Ubuntu derivatives (Non Mint)

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

## Arch Linux derivatives

You need to install Qt. You can check the version of your Qt packages with `qtdiag` or `qmake --version`.

```bash
sudo pacman -S qt6-declarative qt6-charts

git clone https://github.com/huderlem/porymap
cd porymap

qmake
make
./porymap
```
