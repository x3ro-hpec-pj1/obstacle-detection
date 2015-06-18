# Dependencies

If you're on a recent Ubuntu (e.g. 14.10), installing the dependencies can be done by running:

    sudo apt-get install gcc-4.9-arm-linux-gnueabihf minicom sshpass

# Building

Don't forget to run

    git submodule update --init

after cloning!

Make sure to clone with --recursive or run git submodule update --init after cloning, to pull in the jansson dependency.

Build for your host linux using `make` and for the Zedboard by running

    make -f Makefile.zedboard

Running the project locally:

    RUN_PARAMS=path-to-scannerdevice-or-scannerfile make run

Running the project on the board:

    RUN_PARAMS=path-to-scannerdevice-or-scannerfile make -f Makefile.zedboard run

Note that to run remotely, the file/device that RUN_PARAMS points to must be transferred to the boards `TARGET_DIR` as configured in `Makefile.zedboard`. This is achieved by running:

	make -f Makefile.zedboard scannerdata


# Laserscanner on the Zedboard

Disclaimer: The Laserscanner on the Zedboard is not currently working. I'm describing the setup I've tried and the issues I have encountered in this section.

In order to use the Zedboard's USB OTG port in host mode, JP2 and JP3 have to be shorted (closed), according to [the Zedboard Getting Started Guide](http://zedboard.org/support/documentation/1521). Shorting JP2 enables 5V output on the USB port and shorting JP3 sets it to "Host mode".

Since, for me, the laserscanner would not power up even after setting the jumpers correctly, I tried with an intermediate USB hub:

    ┌────────┐                                           
    │        │    OTG to                ┌───────────────┐
    │  Zynq  │◀───USB(A)───▶◀──USB(A)──▶│Powered USB Hub│
    │        │      [1]                 └───────────────┘
    └────────┘                                  ▲        
                                   USB(A) to    │        
                               ┌──USB(mini-B)───┘        
                               │      [2]                
                               ▼                         
                        ┌────────────┐                   
                        │            │                   
                        │ Hokuyo URG │                   
                        │            │                   
                        └────────────┘                   

For [1] I used one of [these](http://www.amazon.com/Insten%C2%AE-Micro-USB-Adapter-Cable/dp/B005QX7KYU) and [2] is one of [these](http://www.amazon.com/Monoprice-1-5-Feet-Mini-B-Ferrite-105446/dp/B003L18RZU/). 

This worked as far as connecting the USB device and being able to see it on the Zedboard with `lsusb`. However, when connecting it, the following error message was displayed:

    usb 1-1.2.4: new full-speed USB device number 7 using zynq-ehci
    usb 1-1.2.4: device descriptor read/64, error -32

I've tried this with [Zynq Linux releases](http://www.wiki.xilinx.com/Zynq+Releases) 2014.3 and 2014.4 but could not figure out a way to fix it. 

# Remote debugging

When running `make -f Makefile.zedboard debug`, a gdbserver on the board will be started on port 3333 expecting connections from 192.168.1.1. To connect to the gdbserver, you can run the following command:

    arm-none-eabi-gdb -tui --command=gdb.conf cimpl

Make sure to have `arm-none-eabi-gdb` installed for that :)




# Troubleshooting

> ./lib/jansson/src/.libs/libjansson.a: error adding symbols: File format not recognized

This error indicates that Jansson is still compiled for another architecture. Run `make jansson_clean clean` and then recompile for your architecture.

> `make run` of the Makefile.zedboard fails silently

Make sure that you can connect to the board using a regualr SSH session:

    ssh root@[insert board ip here]

If this works, check if the correct board IP is set in the Makefile.zedboard.

