# LearnOpenGl Examples 

**Unofficial** cross platform examples for [learnopengl.com](https://learnopengl.com/)

[Live Demos](https://www.geertarien.com/learnopengl-examples-html5/)

- written in C.
- shader dialect GLSL v450
- runs on OSX, Linux, Windows and web (emscripten) from the same source
- uses [Sokol libraries](https://github.com/floooh/sokol) for cross platform support


## Building 

[Fips](http://floooh.github.io/fips/index.html) is used as build system to support multiple platforms.

#### Requirements

* a **C development environment**:
    - OSX: Xcode + command line tools
    - Linux: make/ninja + gcc/clang
    - Windows: Visual Studio
* [CMake](https://cmake.org/)
* [Python](https://www.python.org/)
* [Git](https://git-scm.com/)
* **Linux only:** libgl-dev libx11-dev libxi-dev libxcursor-dev

#### How to Build

```bash
> mkdir fips-workspace
> cd fips-workspace
> git clone https://github.com/GeertArien/learnopengl-examples.git
> cd learnopengl-examples
> ./fips build
> ./fips run 1-3-1-rendering
```

#### Targets

To get a list of targets:

```bash
> ./fips list targets
```

To run a specific target:

```bash
> ./fips run 3-1-2-backpack-lights
```


#### Web Builds

To enable web builds you need to setup the [emscripten](https://emscripten.org/index.html) SDK and select
one of the webgl2 configs: _webgl2-wasm-ninja-debug_ or _webgl2-wasm-ninja-release_.

```bash
> ./fips setup emscripten
> ./fips set config webgl2-wasm-ninja-debug
> ./fips build
```


## IDE Integration

Fips supports CLion, Visual Studio, Visual Studio Code and XCode as provided by cmake:

```bash
> ./fips set config linux-clion-debug
> ./fips gen
> ./fips open
```

```bash
> ./fips set config win64-vstudio-debug
> ./fips gen
> ./fips open
```

```bash
> ./fips set config win64-vscode-debug
> ./fips gen
> ./fips open
```

```bash
> ./fips set config osx-xcode-debug
> ./fips gen
> ./fips open
```
