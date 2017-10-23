Init
====
```
git submodule update --init
```

Build
=====

We need to use brew openssl -- this will let cmake find it.
 
```
OPENSSL_ROOT_DIR=/usr/local/opt/openssl/ cmake ..
```

