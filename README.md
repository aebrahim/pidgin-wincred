# pidgin-wincred

Pidgin usually stores passwords as plaintext, which is generally considered
a poor idea. This plugin uses the Windows Credential Manger to log passwords,
which in theory only allows the user who wrote the credentials to read
them back. Many would argue this is a more secure form of password storage.

This plugin has primarily been tested on Windows 7 and 8, but some users have
also reported success with Windows XP. Feedback (by posting issues to the
[tracker](https://github.com/aebrahim/pidgin-wincred/issues)) would be helpful.

## Installation Instructions

Download the zip file for the latest version from the
[releases page](https://github.com/aebrahim/pidgin-wincred/releases).
Unzip the file, and copy the pidgin-wincred.dll file to the pidgin
plugin directly (usually ```%APPDATA%\.purple\plugins```).


## Build-it-yourself Instructions

To build the plugin from source manually, you will need to use the 32 bit
version of the mingw-w64 compiler and follow the instructions for [building
pidgin on windows](http://developer.pidgin.im/wiki/BuildingWinPidgin). The
easiest way to do this is to copy and paste from .travis.yml to set up your
build environment.

The [travis build](https://travis-ci.org/aebrahim/pidgin-wincred) should 
also cross-compile a working plugin. It is set to deploy on tagged releases 
on my repository with my key, so these values should be modified in your 
repository's .travis.yml file if you want to use this.
