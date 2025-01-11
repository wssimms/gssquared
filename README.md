# GSSquared - A Complete Apple II Series Emulator.

GSSquared is a complete emulator for the Apple II series of computers. It is written in C++ and runs on Windows, Linux, and macOS.

It uses the SDL2 library for graphics, sound, and I/O. This is a video game-oriented library and suits an emulator well. And, it's cross-platform.

To date, development has been Mac-only. The code base builds on a Mac M1.

Eventually (hopefully, soon) I will build for Linux. I am hoping I can get someone else to build on Windows. :-)



## Build for Production

### CLI

```
cmake -DCMAKE_BUILD_TYPE=Release .
make
```

### Mac App Bundle

Gets installed into GSSquared.app/

```
cmake -DCMAKE_BUILD_TYPE=Release
make mac-package
```

## Build for Debug

```
cmake -DCMAKE_BUILD_TYPE=Debug .
make
```