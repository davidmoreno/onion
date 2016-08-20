#!/usr/bin/env bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

    # Install some custom requirements on OS X
    # e.g. brew install pyenv-virtualenv

    brew update;
    brew tap homebrew/versions;
    brew install gnutls;
    brew install libgcrypt;
    brew install hiredis;
    brew install bdw-gc;
    brew install libev;
    brew install gettext;

    #FIXME: temporary hack for mac as gettext is not detected otherwise
    ln -s /usr/local/Cellar/gettext/*/lib/libintl.dylib /usr/local/lib/;
    ln -s /usr/local/Cellar/gettext/*/include/libintl.h /usr/local/include;

    if [[ $CXX == 'g++' ]]; then brew install gcc5 --enable-cxx; fi
    if [[ $CXX == 'clang++' ]]; then brew install llvm37 --enable-cxx; fi
fi