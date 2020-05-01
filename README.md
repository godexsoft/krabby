# Krabby

### Compile
```
    mkdir build
    cd build
    cmake -DENABLE_LOG=YES
    make
```

### Usage:
See `examples/scripts` directory for Lua code.

Krabby accepts a data root directory as first (optional) argument. It will load all the `.lua` scripts it can find recursively.

```
    $ krabby -h
    $ krabby path/to/data/root
```

*NOTE:* Krabby will use current directory as data root if no path is specified.

Proper documentation will be written eventually.

### Powered by:
* https://github.com/hrissan/crablib
* https://github.com/GVNG/SQLCPPBridgeFramework
* https://github.com/ThePhD/sol2
* https://github.com/pantor/inja.git
* https://github.com/jarro2783/cxxopts.git
* https://github.com/fmtlib/fmt  
