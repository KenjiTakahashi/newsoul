**newsoul** is a daemon/server based [Soulseek](http://www.slsknet.org) client, a successor of Museek+.

It is basicaly my attempt to resurrect Museek+, which development ceased few years ago.

This is *only* the daemon/server part, no clients (besides CLI) are developed here.

## requirements

* libxml2
* libevent
* libvorbis
* nettle
* tup (for compilation)
* python2.x (for bindings and some misc utils)
* pycrypto (for bindings and some misc utils)

## installation

There is no installation script at this time, so you have to something like
```sh
$ ./build
# cp src/newsoul /usr/bin
# cp src/nsscan/nsscan /usr/bin
```
**Note:** There are also manpages you might want to copy over. Some of them might be a bit outdated, though.

## usage

**Museek+ users:** Copy your configuration files (usually stored in `~/.museekd`) into `~/.newsoul` and you should be all set.

* Use nssetup for first time configuration.
* Run newsoul.
* Use your favourite client (museek+ clients like Museeq still work).

**Note:** Unfortunately, you cannot refresh shares from client applications anymore. Use **nsscan** directly instead.

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
