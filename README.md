# win32-c-http
Simple C HTTP server for Windows

This is a fork of Simple C HTTP server for windows that supports multithreading.

## What can this do

It can currently serve a select set of files to roughly 5000 clients simultaneously. I got this number by using JMeter against the server on my own machine, and kept on raising the number of "users" until the server started behaving weirdly. On my computer, the bottleneck was processing bandwidth. If you have a beefier machine, this might handle more simultaneous clients better. 

## Should I use this in production

Absolutely not.

## What issues does it have.

A lot, namely

* There is no real routing functionality right now
* There are wonky debug printf statements everywhere
* There's a lot of code that, while it kinda works, is not bulletproof enough for a production environment (my current implementation of a ring buffer leaves a lot to be desired).

## Why does this exist then

Because I wanted to see if I could get this to multithread. Also, I'm curious about how web servers work at a low level, as well as threading.


