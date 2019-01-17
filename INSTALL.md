# Installation

porymap requires Qt 5 & C++11.

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

## Ubuntu

You need to install Qt 5. Qt 5.5 should be enough. You can check you Qt version
with `qtdiag`.

```
sudo apt-get install qt5-default
qmake
make
./porymap
```
