This is a small utility which can selectively replay messages for a set of tables from a tickerplant
log file. There should be a speedup over filtering using `-11!` as it does not materialise a message
unless it for one of the tables of interest. The library therefore needs to parse the bytes itself.

### Bulding

The library uses CMake (yes, to build a one-file project, I know ... I just had the template lying
around). Use it as follows:

```
mkdir build && cd build
cmake ..
make
```

If the build is successful, it writes the file `libmglogfilter.so` into the `lib` directory, which
it creates if necessary.

### Usage

There's an example of how this can be bootstrapped and run in `test/replay.q`. The script can write
a sample journal for you just to see how it works. Load the script into `kdb+` and run `.rpl.wrjnl`,
passing the `hsym` path to a destination file and the number of messages to write, _e.g.:
```
q).rpl.wrjnl[`:test/journal] 50
```

The journal will contain messages for two tables, `trade` and `quote`.

You've probably by this point already noticed that the script's `.rpl.init` function has run, which
loads the shared library and maps its only exported function to `.rpl.replay`. Invoke that function
passing two arguments: first, the `hsym` path to the journal to be replayed, and second a
symbol-vector containing the names of the tables of interest. For example:
```
q).rpl.replay[`:test/journal] (),`trade
```

There's no facility to pass the number of messages to replay, but it's something you could quite
easily add. The idea is that this provides a reasonable starting point for customisation. It should
be safe to use on a live journal, as it runs `stat` over the file before starting to scan, and
doesn't read past that size; you'd hope and expect by the time it got that far that KDB would have
finished writing a partial message to disk.

