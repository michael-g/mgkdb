### A strongly-typed KDB IPC library in C++23

On the basis that perfection is the enemy of done, here's a KDB IPC library in modern C++ (`c++23`). The library supports the deserialisation of compressed messages and up to version 3 of the KDB IPC format, in other words: the one _with_ timestamps but which is limited to `INT32_MAX` elements in its vectors. Support for the "big data" version may follow at some point.

The library is interesting in that it can operate on partial reads of an IPC message, and can make progress deserialising what's been received while the remaining data is still in flight. The same is true of the decompressor: it will decompress as much data as it can and pass this on to the parser without needing the whole image in memory. For those that search for some things, this supports "streaming deserialisation" and "streaming decompression".

I've gone to some lengths to implement a "to-string" function for pretty much every kdb-type, with the intention you can copy the output from the console and paste into your kdb+ REPL. It even does nice things like quote symbols when they contain spaces. I like it.

The library does not (as yet) contain any I/O components and simply reads from memory buffers. It's always been a pain-point when I've used libraries that are opinionated about event-loops and IO calls.

Although the library does not include any networking code, it _does_ include one of those "magic buffer" implementations — where if you write past the end of the buffer the data is wrapped around and written to the start. It's an `mmap`-fest which is documented in a number of places, [Fabien Sanglard](https://fgiesen.wordpress.com/2012/07/21/the-magic-ring-buffer/), [Wikipedia](https://en.wikipedia.org/w/index.php?title=Circular_buffer&oldid=600431497#Optimized_POSIX_implementation), as copied and modified by [`vitalych`](https://github.com/vitalyvch/rng_buf) (another one [here](https://lo.calho.st/posts/black-magic-buffer/)).

### See the examples

Please see the [examples](examples) directory. It includes a [tickerplant subscriber](examples/subscribe_to_tp.cpp), which subscribes (of course) but also replays the tickerplant's log-file while filtering the messages using the on-disk IPC data, in other words, without allocating. In its current form it tries to connect to a vanilla TP with a `trade` table listening on port 30099. It sends a function to the TP so that it can get the `.u.i` and `.u.L` values for the replay.

Please also see the [connect_to_kdb.cpp](examples/connect_to_kdb.cpp) app for how to get connected.

Both apps use blocking reads/writes, because the code is less verbose than the async alternative, and fulfil the task of demonstrating how you can use the library.

There's a demo of the `CircularBuffer`, used when writing data to kdb+ when using an artificially _tiny_ buffer. It takes away the offset-calculations you have to do when potentially compacting unwritten bytes left in the source-buffer. See [demo_circular_buffer.cpp](examples/demo_circular_buffer.cpp). This needs a listening kdb+ on port 30098.

Check [examples/journal_reader.cpp](examples/journal_reader.cpp). You'll need to edit the path to your tickerplant journal-of-choice, but it will happily scan the journal — without materialising any data, literally just looking at the on-disk bytes — and filter the messages for the chosen tables. For example, given a market-data tickerplant log-file, you could tell it _only_ to replay or forward messages of type `trade` but not `quote`.

It's worth just highlighting that when acting in a subscriber-mode, for example, or when issuing multiple queries before awaiting the responses, that there may be additional bytes in your memory buffer which belong to the _next_ message. You should check the `ReadMsgResult` object for the number of bytes read via its `bytes_consumed` value.

### Example usage

```cpp

mg7x::KdbSymbolAtom fun{".u.sub"};
mg7x::KdbSymbolVector tbl{{"trade","quote"}};
mg7x::KdbSymbolAtom sym{}; // the null sym
mg7x::KdbList msg{};

msg.push(fun);
msg.push(tbl);
msg.push(sym);

mg7x::KdbIpcMessageWriter writer{mg7x::KdbMsgType::SYNC, msg};
// create buffer 'ary' with sufficient capacity, see mg7x::KdbUtil::ipcMessageLen
mg7x::WriteResult res = writer.write(ary.data(), ary.capacity());
// check result ..
```

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
