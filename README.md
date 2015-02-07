# pidgin-wincred

Pidgin usually stores passwords as plaintext, which is generally considered
a poor idea. This plugin uses the Windows Credential Manger to log passwords,
which in theory only allows the user who wrote the credentials to read
them back. Many would argue this is a more secure form of password storage.

This plugin has primarily been tested on Windows 7, but some users have also
reported success with Windows XP. Feedback (by posting issues to the tracker)
would be helpful.

## Installation Instructions

Download the zip file for the latest version. Unzip the file, and copy the
pidgin-wincred.dll file to ```%APPDATA%\.purple\plugins```


## Build-it-yourself Instructions

To build the plugin from source, follow the instructions for [building pidgin
on windows](http://developer.pidgin.im/wiki/BuildingWinPidgin). Put the source
into the libpurple subdirectory. You will need to use the 32 bit version of
the mingw-w64 compiler. The easiest possible way is to copy the build procedure
used in .travis.yml to set up the environment.
