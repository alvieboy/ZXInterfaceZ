# ZX Spectrum ROMs

## Prerequisites

- pasmo 0.6.0 preview

### Building Pasmo 0.6.0-preview from Source

**Debian/Ubuntu**

The pasmo package from apt is too old, you need to install from source. And
pasmo must be patched to build with a current version of Debian/Ubuntu.

```
wget http://pasmo.speccy.org/bin/pasmo-0.6.0.20070113.0.tgz
tar -xzf pasmo-0.6.0.20070113.0.tgz
cd pasmo-0.6.0.20070113.0/
pathc -p1 < ../pasmo-0.6.0.20070113.0.patch

./configure
make
sudo make install
```

