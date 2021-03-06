# PDP-6 Simulator

This project is an attempt to simulate the PDP-6 computer
with front panel and blinkenlights on the logic level.
The maintenance manual has flow charts, schematics and explanations:
[Volume1](http://bitsavers.trailing-edge.com/pdf/dec/pdp6/F-67_166instrManVol1_Sep65.pdf)
[Volume2](http://bitsavers.trailing-edge.com/pdf/dec/pdp6/F-67_166instrManVol2_Sep65.pdf)

## Code

The code is more or less a transcription of the schematics into C.
This means you will not understand it unless you're familiar with the maintenance manual.
Pulses are represented as functions, when a pulse triggers another pulse
it does so by the `nextpulse` function which adds a pulse to the list of next pulses.
In the main cpu loop the list of current pulses is iterated and each pulse is called,
then (after checking some external signals) the current and next pulse lists are swapped
and the process begins anew.
The timing was not accurately modeled and there is room for improvement.
Due to the inexact timing the hardware connections (through the memory and IO bus)
were not implemented too accurately. This may change in the future.

## Building

The supplied makefile assumes gcc (there are flags to silence some stupid warnings).
Otherwise you need SDL and pthread.

## Running

The cpu and the console tty are implemented.
There are no other external devices yet.
The only things missing from the cpu are the clock to generate interrupts
and the repeat key mechanism.
The simulator reads `fmem` and `mem` to initialise the memory and fast memory.

## To do

- clock and repeat
- test thoroughly!
- devices
- timing
