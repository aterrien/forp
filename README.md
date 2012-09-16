!under development!

on : PHP 5.3.8,  Debian Squeeze

[![Build Status](https://secure.travis-ci.org/aterrien/forp.png)](http://travis-ci.org/aterrien/forp)

# Introduction #

forp is a lightweight PHP extension which provides CPU and memory profiling datas.

## PHP functions ##
- forp_enable() : starts forp
- forp_dump() : Array - forp stack
- forp_print() : displays forp stack (CLI)

## Install ##

* make
```sh
git clone https://github.com/aterrien/forp
cd forp
phpize
./configure
make
make test
make install
```

* php.ini
```ini
extension=forp.so
```

## Win32 Install ##

For PHP 5.3 - Thread Safe (by default) :
https://github.com/downloads/ichiriac/forp/php53_x86_ts_forp.dll

For PHP 5.3 - Non Thread Safe :
https://github.com/downloads/ichiriac/forp/php53_x86_nts_forp.dll