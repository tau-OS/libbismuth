<img align="left" style="vertical-align: middle" width="120" height="120" src="Bismuth.png">

# Bismuth

Based on libadwaita responsive widgets, without all the baggage.

## License

Bismuth is licensed under the LGPL-2.1+.

## Building

```sh
meson . _build
ninja -C _build
ninja -C _build install
```

For build options see [meson_options.txt](./meson_options.txt).
E.g. to enable documentation:

```sh
meson . _build -Dgtk_doc=true
ninja -C _build
```
