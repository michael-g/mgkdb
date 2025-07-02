On the basis that perfection is the enemy of done, here's a KDB IPC library in modern C++ (`c++23`). The library supports the deserialisation of compressed messages and up to version 3 of the KDB IPC format, in other words: the one _with_ timestamps but which is limited to `INT32_MAX` elements in its vectors. Support for the "big data" version may follow at some point.

The library is interesting in that it can operate on partial reads of an IPC message, and can make progress deserialising what's been received while the remaining data is still in flight. The same is true of the decompressor: it will decompress as much data as it can and pass this on to the parser without needing the whole image in memory. For those that search for some things, this supports "streaming deserialisation" and "streaming decompression".

The library does not (as yet) contain any I/O components and simply reads from memory buffers. It's always been a pain-point when I've used libraries that are opinionated about event-loops and IO calls.

### Example usage

Please see the [examples](examples) directory for the [connect_to_kdb.cpp](examples/connect_to_kdb.cpp) sample. It's deliberately simple and uses blocking reads/writes, just because you end up with less code. Think about it for a second: once you've serialised a message to a buffer, how _you_ choose to write those bytes to a socket should be up to you.

It's worth just highlighting that when acting in a subscriber-mode, for example, or when issuing multiple queries before awaiting the responses, that there may be additional bytes in your memory buffer which belong to the _next_ message. You should check the `ReadMsgResult` object for the number of bytes read via its `bytes_consumed` value.

Here's some pseudo code with a particularly unattractive while-loop condition:

### Building

The system uses CMake. It's not been designed for an "in source" build, so do the usual:

```
mkdir build
cd build
cmake ..
make
```
This should build the `lib/libmgkdbipc.a` archive and the example and a couple of research apps in `bin/`.

### Testing

#### Run a KDB instance 

One of the tests requires a running KDB instance to write data to. Run the "receiver" part of the tests script like this:
```
$ qq test/ConnectToKdbTest.q
```

Then in `build/`:
```
make && make test
```

The tests are created into the `build/test/` directory if you want to run them individually. The `KdbIpcMessageReaderTest` expects its current working directory to be `build/test` (because it uses a poor-man's way of locating the `tenKQa.zipc` file).
