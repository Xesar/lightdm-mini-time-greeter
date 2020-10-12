# Mini-Greeter

Forked from prikhi/lightdm-mini-greeter, added a clock displayed under the password input/ label


## Install

### Manual

You will need `automake`, `pkg-config`, `gtk+`, & `liblightdm-gobject` to build
the project.

Grab the source, build the greeter, & install it manually:

```sh
./autogen.sh
./configure --datadir /usr/share --bindir /usr/bin --sysconfdir /etc
make
sudo make install
```

Run `sudo make uninstall` to remove the greeter.


## License

GPL-3
