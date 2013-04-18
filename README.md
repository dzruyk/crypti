Crypti
=====

Description
-----------

Crypti is simple language for crypto researchers.

Installation
------------
For installing crypti you need to install libmpl lib,
accessible at https://github.com/dzruyk/libmpl

after downloading build it with

    make

then install with
    make install
IMPORTANT: installation need root rights.

After installing libmpl you need to run
    make
    make install
at crypti root dirrectory.

After this you can run crypti interpreter with

    ./bin/crypti

Using
------

./bin/crypti [OPTIONS] [file]

./bin/crypti is interpreter that executes commands from standard input or from file

Examples
-------

NOTE: Full reference accessible at ./doc/specification.asciidoc (unfortunately it only on russian now).

Hello, world example:

    print("hello, world")


