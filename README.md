# What
This repository contains libraries and utilities I've developed for use with kdb+. Hopefully they're of help! 

### C++ KDB IPC library

The centrepiece of is the [**ipc++**](ipc++) project which provides a strongly-typed C++23 KDB IPC library, with other goodies included. You get things like an IPC-reader and writer, types like `KdbSymbolAtom` and `KdbUnaryPrimitive`, [example](ipc++/examples) applications as well as utilities to scan on-disk KDB tickerplant log-files. 

### Javascript KDB IPC library

I have done quite a bit of $work with Kx Systems' venerable `c.js` and thought that something with stronger typing (in Javascript, ho ho) would be useful. This library provides just that, and has a one-to-one correspondence with the types you'll find in kdb+: there's a [`KdbRealAtom`](ipcjs/src/ipc.js#L482), [`KdbTable`](ipcjs/src/ipc.js#L1220), _etc_. You don't have to prefix your "symbols" with a backtick, nor suffer translations on the server-side from a date-time type to whatever it was you wanted to send. Just create an object graph with a `KdbSymbolAtom` and one of the `KdbDateAtom`, `KdbTimestampAtom`, `KdbMonthAtom`, `KdbDateTimeAtom`, `KdbTimespanAtom` ... and all the rest, and you can be much more specific about how your data is encoded. The library includes several JS `Date` [conversion utilities](ipcjs/src/ipc.js#L115).

Please see the [ipcjs](ipcjs) project and the [ipc.js](ipcjs/src/ipc.js) file in particular. This uses the decompression function from the `c.js` script.

There's a basic React.JS application to showcase its usage in the [react](react) project directory. Build the React application and assign its output directory to `.h.HOME`, then use kdb+ to serve it as a single-page application (SPA).

### Tickerplant Multiplexer

This research project implements a standalone application that will subscribe to a give set of tables from a number of tickerplants, replay and filter their log-files into its own and then proxy subsequent matching TCP messages to its own connected clients. If productionised, it would be useful in producing a sequenced stream of filtered data that could underpin deterministic computation (since there's only one strongly-ordered input stream). 

The "research" nature of the project lies in the fact that it uses C++ coroutines and at present doesn't implement failure-mode recovery strategies. I guess they'd say that this was left as an exercise for the reader. 

### Tickerplant Log Filter

This project implements a pure-C implementation of the tickerplant log-scanner, suitable for loading into a `q` instance and using instead of `-11!`. As it suggests in its README, you can call 
```q
.rpl.replay[`:test/journal] (),`trade
```
to replay just the `trade` messages from the `test/journal` tickerplant log file. No more waiting for hundreds of millions of quote messages!
