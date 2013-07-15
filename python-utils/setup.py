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
    long_description="nslog (chat logging), nscontrol (commandline client/tool), nschat (curses chat client), nssirc (irc gateway)",
    scripts=['nslog', 'nschat', 'nscontrol', 'nssirc.py'],
    data_files=[('man/man1', ['nslog.1', 'nscontrol.1'])]
)
