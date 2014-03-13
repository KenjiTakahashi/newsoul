[![Build Status](https://travis-ci.org/KenjiTakahashi/newsoul.png?branch=master)](https://travis-ci.org/KenjiTakahashi/newsoul)

**newsoul** is a daemon/server based [Soulseek](http://www.slsknet.org) client, a successor of Museek+.

It is basically my attempt to resurrect Museek+, which development ceased few years ago.

This is *only* the daemon/server part, no clients (besides CLI) are developed here.

## requirements

* json-c >= 0.10
* libevent
* taglib
* nettle
* sqlite3 >= 3.7.11
* pcre
* google-glog
* C++11 capable compiler (for compilation)
* premake4 (for compilation)
* CppUTest (for test-suite)
* python2.x (for bindings and some misc utils)
* pycrypto (for bindings and some misc utils)

## installation

```sh
$ cd build
$ premake4 gmake  # see premake4 --help for other options
$ make newsoul
# premake4 install
```

## usage

* Use `$ newsoul set` command for first time configuration. See `$ newsoul set help` for detailed information.
* Run `$ newsoul`.
* Use your favourite client (Museek+ clients like Museeq should still work).

**Note:** Unfortunately, it is not possible to manage shares from client applications anymore. Please use `$ newsoul database` command instead.

**Note:** Version 0.2 is meant as a transitional release and suffers from a few known bugs:

* Some configuration options might not get applied before the server is restarted.
* Application startup might become slow when the shares database grows bigger. This also affects using `newsoul` as a CLI utility.

Both of these issues require significant changes to be properly fixed. It is the main course of the 0.3 release.

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
