# tiny
A shader based demo framework

## Motivation
I want a framework around GLSL shaders that I can use to contruct demos. It should take care of all of the optimizations and produce a single binary that can be executed. It should also be compatible with GNU Rocket out of the box and provide the ability to create keyframes and pass the current values of these variables as uniforms into the shader. This allows the basics such as moving the camera/world around, as well as changing the position and attributes of objects.

Currently it outputs a base executable of around 22KB which is *huuge* but I haven't yet found the Linux ELF equivilent of [crinker](http://crinkler.net/). The next step in size-optimization would be stripping out the default libc for something much smaller but I still want the binary to run on a fresh Ubuntu install. There are a few ASM hacks to bootstrap the ELF without all the libc crap still to explore.

## Pre-requisites
* binutils
* gcc
* mono
* upx

## Special thanks
* [Inigo Quilez](https://www.iquilezles.org/) for learning material
* [Ctrl-Alt-Test](https://ctrl-alt-test.fr/) for shader minifier 
