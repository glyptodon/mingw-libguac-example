mingw-libguac-example
=====================

This example demonstrates basic use of libguac within Windows applications,
leveraging recent upstream Windows compatibility at the libguac level. The
example itself is a reimplementation of the [ball protocol
tutorial](http://guacamole.incubator.apache.org/doc/gug/guacamole-protocol.html)
in the Guacamole manual.

Building with MinGW
-------------------

Building the example using [MinGW](http://www.mingw.org/) should be as simple
as running `make`:

    $ make

This will produce a new executable, `ball.exe`, within the `bin/` directory.
The DLLs of all dependencies are already included in that directory, including
a Windows build of libguac.

For convenience, the build will also produce a zip file, `ball.zip`, containing
the executable and all dependencies.

