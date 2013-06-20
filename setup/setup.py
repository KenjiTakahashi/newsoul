version = "1.0.0"

from distutils.core import setup
from distutils.extension import Extension

setup(name                  = "museek-setup",
      version               = version,
      license               = "GPL",
      description           = "A collection of python setup tools for museekd",
      author                = "daelstorm, hyriand",
      author_email          = "daelstorm@gmail.com",
      url                   = "http://museek-plus.org/",
      long_description      = "musetup config tool",
      scripts               = ['musetup'],
      data_files            = [('man/man1', ['musetup.1'])],

)
