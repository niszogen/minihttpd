# minihttpd
**minihttpd** is a simple multithreaded http/1.1 server, written in C.

## Instalation

```bash
git clone https://github.com/niszogen/minihttpd
meson setup build
ninja -C build
sudo ninja -C build install
```

## License

This project is licensed under the [GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html) license, with the exception of `libminihttpd`, which is licensed under [LGPLv3](https://www.gnu.org/licenses/lgpl-3.0.html).
