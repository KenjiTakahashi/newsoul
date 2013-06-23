version = "1.0.0"

from distutils.core import setup

setup(
    name="newsoul-python-utils",
    version=version,
    license="GPL",
    description="A small collection of python utils for newsoul",
    author="daelstorm",
    author_email="daelstorm@gmail.com",
    url="http://museek-plus.org/",
    long_description="mulog (chat logging), museekcontrol (commandline client/tool), museekchat (curses chat client), musirc (irc gateway), musetup",
    scripts=['mulog', 'museekchat', 'museekcontrol', 'musirc.py', 'musetup'],
    data_files=[('man/man1', ['mulog.1', 'museekcontrol.1', 'musetup.1'])]
)
