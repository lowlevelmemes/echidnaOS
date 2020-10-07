# echidnaOS

## What is echidnaOS?

echidnaOS is a pointless exercise in futility, using x86 segmentation.

# Building Instructions

## Requirements

* nasm
* gnu make
* gcc
* g++
* xbstrap
* qemu (for testing the image, non essential)

## Step by step:

Build the toolchain and everything else
```bash
mkdir build && cd build
xbstrap init ..
xbstrap install --all
cd ..
```

Build the kernel and image
```bash
make clean all
```

Run it
```bash
make run
```
