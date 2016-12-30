# ARM-STM32F469I-Discovery-with-Sound-
A hacked port from floppes/stm32doom to work on the 469I discovery..with sound!

Right now it works..sort of.  It run the start demo and plays the sounds.  No music yet, I am working on a software synth for it as we speak.

Movement dosn't work right yet.  The discovery supports multi touch so I will get figners to work on it soon.  I "may" move over to the 479 as that also has a network port and it be intresting to get that to work.  (IPX networking for the win!)

Right now its more of a proof of concept of me learning how to use the audio out and to use the built in DSP.  Might move some of the code base to use more of the DSP for preformance reasons but right now it runs at 30 frames a second.

It still runs at 320x240, doubled in size.  Also the back buffer flipping is unoptimized and i need to change it over to a DMA model.

All in all I am proud but still alot of work ot be done and thought I post a working copy of it so I don't screw up and delete it by accident.

