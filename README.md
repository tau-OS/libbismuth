<img align="left" style="vertical-align: middle" width="120" height="120" src="assets/Bismuth.svg">

# libbismuth

Libadwaita's responsive widgets, without all the baggage.

###

[![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v2.1-blue.svg)](https://www.gnu.org/licenses/old-licenses/lgpl-2.1)

## 🚀 Getting Started

Welcome! Please see the various resources below. If you have any questions, our [Discord](https://discord.gg/BHNfGewTXX) is always open :)

- [libbismuth's gidoc](https://docs.developers.tauos.co/libbismuth/)
- [libbismuth's rustdoc](https://docs.developers.tauos.co/rust/libbismuth/index.html)

## 🛠️ Dependencies

Please make sure you have these dependencies first before building.

```bash
gtk4
meson
fribidi
```

Please note that generating documentation requires the following dependencies.

```bash
gi-docgen
```

## 🏗️ Building

Simply clone this repo, then:

```bash
meson _build --prefix=/usr && cd _build
sudo ninja install
```

For additional build options, please see [meson_options.txt](./meson_options.txt). For example, to enable documentation:

```sh
meson _build -Ddocumentation=true --prefix=/usr && cd _build
sudo ninja install
```
