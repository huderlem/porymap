This directory holds the sources that build the porymap documentation website. It uses Sphinx to build a static website, and copy the results to the `docs/` directory for GitHub Pages.

## Setup
Sphinx uses Python, so you can use `pip` to install the dependencies:
```
pip install -r requirements.txt
```

## Build
This will build the static site and copy the files to the root-level `docs/` directory.  The GitHub Pages site will automatically update when the commit is merged to porymap's `master` branch.
```
make github
```
