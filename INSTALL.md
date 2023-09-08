# Installation

porymap requires Qt 5.14.2 & C++11.

## macOS

The easiest way to get Qt is through [homebrew](https://brew.sh/). 
Once homebrew is installed, run these commands in Terminal:

```
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
with `qtdiag`.

```
sudo apt-get install qt6-declarative-dev
# if your distro does not have qt6-declarative-dev, try sudo apt-get install qtdeclarative5-dev
qmake
make
./porymap
```
