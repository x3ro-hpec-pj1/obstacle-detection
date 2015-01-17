# Building

Make sure to clone with --recursive or run git submodule update --init after cloning, to pull in the jansson dependency.

Build for your host linux using `make` and for the Zedboard by running

    make -f Makefile.zedboard

Running the project locally:

    RUN_PARAMS=path-to-scannerdevice-or-scannerfile make run

Running the project on the board:

    RUN_PARAMS=path-to-scannerdevice-or-scannerfile make -f Makefile.zedboard run

Note that to run remotely, the file/device that RUN_PARAMS points to must be manually transferred to the boards `TARGET_DIR` as configured in `Makefile.zedboard`.


# Remote debugging

When running `make -f Makefile.zedboard debug`, a gdbserver on the board will be started on port 3333 expecting connections from 192.168.1.1. To connect to the gdbserver, you can run the following command:

    arm-none-eabi-gdb -tui --command=gdb.conf cimpl

Make sure to have `arm-none-eabi-gdb` installed for that :)




# Troubleshooting

> ./lib/jansson/src/.libs/libjansson.a: error adding symbols: File format not recognized

This error indicates that Jansson is still compiled for another architecture. Run `make jansson_clean clean` and then recompile for your architecture.

