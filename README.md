## requirements

* libxml2
* libevent
* libvorbis
* nettle
* tup (for compilation)
* python2.x (for bindings and some misc utils)
* pycrypto (for bindings and some misc utils)

## installation

There is no installation script at this time, so you have to something like this
```sh
$ tup init
$ tup upd
# cp newsoul/newsoul /usr/bin
# cp muscan/muscan /usr/bin
```

## bindings

There are python2.x bindings available inside python-bindings directory. They can be installed with
```sh
$ cd python-bindings
# python2 setup.py install
```
**Note:** They are deprecated and will be removed in the future.

## utils

There are some miscellaneous utils inside python-clients directory. They can be installed with
```sh
$ cd python-utils
# python2 setup.py install
```
**Note:** They require python-bindings to be installed.

**Note:** They are deprecated and will be removed in the future.
