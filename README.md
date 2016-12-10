# Atmel SAM4L program to gather sensor data, transmit data and receive data wirelessly

## This project requires the following:
## Atmel SAM4L Xplained Pro board x2 -- http://www.atmel.com/tools/atsam4l-xstk.aspx
## AT86RF233 Amplified Zigbit Radio Extension x2 -- http://www.atmel.com/tools/atzb-a-233-xpro.aspx
## Must have Atmel Studio installed

Program to grab sensor data (temperature and light) from one Atmel Xplained Pro board and wirelessly transmit that data to a receiving Atmel Xplained Pro board

###-----------------------------------

There are two different files. Each once contains the source code for each board. "tx_demonstration" is the transmit code to be installed on the board doing the transmitting, "our_rx_demonstration" is the recieve code to be installed on the board receiving data.

###-----------------------------------

This source code requires the user to set up a new project in Atmel studio for the Sam4LC8C Xplained pro, while also connecting the following peripherals:

the I/O1 Xplained Pro should be connected to ext1 of the SAM4LC8C for gathering sensor data

the AT86RF233 Amplified Zigbit Radio should be conencted to ext2 of the SAM4LC8C for transmitting and receiving data.

###-----------------------------------

Once hardware is set up, user needs to copy source code into corresponding files.
main.c and asf.h need to be copied. 
config folder in the project should contain exact copies of files in my config folder.
ASF folder in project should contain exact copies of sub-folders and files inside my ASF folder.

The source code for the the tx program also contains files "at30tse75x.c / .h". These files need to reside along side main.c and asf.h in the transmit program.


