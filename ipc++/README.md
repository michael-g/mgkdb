On the basis that perfection is the enemy of done, here's a KDB IPC library modern C++ (`c++20`). The library supports the deserialisation of compressed messages and up to version 3 (I think) of the KDB IPC format, in other words the one with Timestamps but which is limited to `INT32_MAX` elements in its vectors. Support for the "big data" version may follow at some point. 

The library is interesting in that it can operate on partial reads of the binary IPC message, and so can make progress doing CPU work deserialising the data while data is still pending on the socket. The same is true of the decompressor: it will decompress as much data as it can and pass this on to the parser without having to have the whole image in memory.

The library does not (as yet) contain any I/O components and simply reads from memory buffers.

### Example usage

Create an instance of the `KdbIpcMessageReader`, which you should keep alive until the whole message had been read as it is stateful with regard to counting the number of bytes read, _etc_. Then you would read some data from the socket — possibly the whole message, possibly not — and pass the bytes read to the `KdbIpcMessageReader`. This component will report whether the read is complete via its `bool` return value. 

**NB:** Depending on the data being sent to your application, there may be additional bytes in your memory buffer which belong to the _next_ message. You should check the `ReadMsgResult` object for the number of bytes read via (in this case) the `rmr.bytes_consumed` value.

When the `KdbIpcMessageReader` returns `true`, it will set the `KdbBase` object on the unique-pointer `rmr.message`.

Here's some pseudo code with a particularly unattractive while-loop condition:

```C++
    int fd = // some file descriptor
    ReadMsgResult rmr{};
    KdbIpcMessageReader rdr{};
    std::unique_ptr<KdbObj> obj{nullptr};
    int8_t ipc[1024]; // silly small, just seemed like a nice round number
    size_t len = 0;

    do {
        ssize_t err = read(fd, ipc + len, sizeof(ipc) - len);
        if (err < 0) {
            // check your errors, EINTR etc, handle appropriately
        }
        len += err;

        if (rdr.readMsg(ipc, len, rmr)) {
            obj.swap(rmr.message);
            break;
        }
        len -= rmr.bytes_consumed;
        // I'm not showing it here but you will have to move any bytes remaining
        // in 'ipc' to the front of the buffer before the next read
    }
    while (true);
    // do stuff with obj, typically the first thing you'd do is test obj->m_typ
```

### Building

The system uses CMake. It's not been designed for an "in source" build, so do the usual:

```
mkdir build
cd build
cmake ..
make
```
This should yield the file `lib/libmgkdbipc.q` and a couple of toy applications in `bin/` (_e.g._ one which I used to check on the to-string capabilities of the library).

### Testing

#### Generate the compressed IPC file used during testing

One of the tests needs you to create a compressed IPC file. Use KDB to generate this:
```
$ qq test/KdbIpcMessageReaderTest.q
```

Then generate the compressed IPC file (it's not "ZIP-C", it's "z-IPC") to the `test/` directory: 
```
q).unz.writeZipcFiles `:test
`:test/tenKQa.zipc
```

#### Run a KDB instance 

Another tests requires a running KDB instance to write data to. Run the "receiver" part of the tests script like this:
```
$ qq test/ConnectToKdbTest.q
```

Then in `build/`:
```
make && make test
```

The tests are created into the `build/test/` directory if you want to run them individually. The `KdbIpcMessageReaderTest` expects its current working directory to be `build/test` (because it uses a poor-man's way of locating the `tenKQa.zipc` file).
