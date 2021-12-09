# **nt535-freebsd-sleepd**
Daemon handle between sleep button and ACPI for Foxconn NettopNT535 machines
on FreeBSD. Fixes the issue with the sleep button not causing the machine to
sleep.

## **Installation**
The following steps are required to setup `nt535sleepd`.

*Note: don't confuse the `nt535sleepd` file in the root project directory which is a 
`rc.d` script with the `nt535sleepd` binary that is dumped in `build/` after running 
`make`. The former will be referred as **`rc.d nt535sleepd` script** and the latter 
will be referred as **`nt535sleepd` binary***.
1. After cloning and entering the project's root directory, install libinput with `pkg 
install libinput`.
2. Move the `rc.d nt535sleepd` script into `/etc/rc.d/` directory by running
`mv nt535sleepd /etc/rc.d/`. Then, change the owner and group to `root:wheel` by running
`chown root:wheel /etc/rc.d/nt535sleepd`.
3. Build the `nt535sleepd` binary by running `make`.
4. Move the `rc.d nt535sleepd` binary into `/usr/sbin/` directory by running
`mv build/nt535sleepd /usr/sbin/`. Then, change the owner and group to root:wheel by 
running `chown root:wheel /usr/sbin/nt535sleepd`. Afterwards, grant exec privileges 
by running `chmod +x /usr/sbin/nt535sleepd`.
5. Enable the daemon by adding the `nt535sleepd_enable="YES"` entry into `/etc/rc.conf`.

*All testing done on FreeBSD 13.0*.
