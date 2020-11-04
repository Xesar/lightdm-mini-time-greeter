# Mini-Greeter

Forked from prikhi/lightdm-mini-greeter, added a clock displayed under the password input/ label

Long format:

![Image of long time format](https://raw.githubusercontent.com/Xesar/lightdm-mini-greeter/master/data/ss1_long.png)

Short format:

![Image of short time format](https://raw.githubusercontent.com/Xesar/lightdm-mini-greeter/master/data/ss2_short.png)


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
