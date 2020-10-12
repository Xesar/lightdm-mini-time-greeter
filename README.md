# Mini-Greeter

A minimal but highly configurable single-user GTK3 greeter for LightDM.

Inspired by the SLiM Display Manager & LightDM GTK3 Greeter.


## Current Status

Right now you can:

* log in
* hide the `Password:` label & customize the text
* hide the password input's cursor
* set the size of the login window, the font & every color.
* set a background image.
* use modifiable hotkeys to trigger a shutdown, restart, hibernate or suspend.

![A screen with a dark background and a single password input box in the center](http://bugs.sleepanarchy.com/projects/mini-greeter/repository/revisions/master/entry/screenshot.png "Mini Greeter Screenshot")


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


## Configure

Once installed, you should specify `lightdm-mini-greeter` as your
`greeter-session` in `/etc/lightdm/lightdm.conf`. If you have multiple Desktop
Environments or Window Managers installed, you can specify the one to start by
changing the `user-session` option as well(look in `/usr/share/xsession` for
possible values).

Modify `/etc/lightdm/lightdm-mini-greeter.conf` to customize the greeter. At
the very least, you will need to set the `user`.

You can test it out using LightDM's `test-mode`:

    lightdm --test-mode -d

Or with `dm-tool`:

    dm-tool add-nested-seat

Note: If you've added a `background-image` it will appear in this preview, but
it may not appear during normal use if the file is not in directory which
lightdm has permission to read(like `/etc/lightdm/`). A symlink into this
location won't work.

### Keyboard layout

If your keyboard layout is loaded from your shell configuration files (`.bashrc`
for example) then it might not be possible to type certain characters after
installing lightdm-mini-greeter. You should consider modifying your 
[Xorg keyboard configuration](https://wiki.archlinux.org/index.php/Xorg/Keyboard_configuration#Using_X_configuration_files).

For example for a french keyboard layout (azerty) you should edit/create 
`/etc/X11/xorg.conf.d/00-keyboard.conf` with at least the following options:

```
Section "InputClass"
        Identifier "system-keyboard"
        MatchIsKeyboard "on"
        Option "XkbModel" "pc104"
        Option "XkbLayout" "fr"
EndSection
```


### Style

* Use indentation and braces, 4 spaces - no tabs, no trailing whitespace.
* Declare pointers like this: `char *p1, *p2;`, avoid: `char* p1;`.
* Function braces should be on their own line.
* If/else/while/do should always use braces and indentation.
* Use `g_critical` for irrecoverable user errors, `g_error` for programming
  errors.

When in doubt, check surrounding code.


## License

GPL-3


[aur-package]: https://aur.archlinux.org/packages/lightdm-mini-greeter/
[gentoo-package]: https://packages.gentoo.org/packages/x11-misc/lightdm-mini-greeter
[releases]: https://github.com/prikhi/lightdm-mini-greeter/releases
