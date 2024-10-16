### A More Fully-Featured Javascript Library For KDB IPC 

This library is designed to allow JS authors to be more precise in the way they convert between the Javascript and KDB object models. 

Let me explain.

Kx Systems makes `c.js` available (via Github, [here](https://github.com/KxSystems/kdb/blob/28d14cde9840ddb1d98613560cf5e051ae108a4d/c/c.js)) to us and it does a good job of getting us productive when converting between the two domains. We can see from the comments in the file that it dates at least to May 2015, and I get the feeling that Javascript has come a long way since then. 

However, it does have some rough edges. When serialising, JS `Date` objects are encoded as KDB `datetime` objects, which have been deprecated for quite a while. I suspect that's because of the precision it _does_ support: since `Date` objects are only precise to the millisecond the `datetime` is probably entirely adequate. Moving on, numbers are all sent as floating-point values of type `9h`. Encoding JS `String` values as symbols requires the somewhat inelegant prefixing of the business-logic values with a leading backtick.

In contrast, this library allows you to encode values as any* of the KDB types. It's possible to represent a JS `Date` object as any of a timestamp, month, date, timespan, minute, second or time value. I might get to supporting date-time values ... but I'm not keen. At the time of writing there's a blank where the `KdbGuidAtom` should be, again because it's a bit of an investment and I'm not sure of the size of their audience. But anyway: what you put into the pipe at one end now comes out the other in the format you specified. 

_\[* except GUID and date-time values, for now\]_

#### 64-bit Integer Support

The library uses `BitInt64Array` and `BigInt` values for the types where a 64-bit integer is required. This adds its own set of ergonomics issues as you can't mix `Number` and `BitInt` values in mathematical expressions. Nonetheless, there's now no fear of truncated values, you just have to code appropriately.

### Standalone

The `ipc.js` library is intended to be framework agnostic, and can be used entirely standalone. 

### Sample React App

I've packaged a sample React App with the library that shows how you might encode values, and it also packages a minimalist version of a column-oriented HTML table. 

### Column-Oriented HTML Tables

I've often wondered why pretty much all open-source HTML `table` implementations all seem to want to work over rows of objects, rather than sets of columns. A KDB table is composed of a list of column-values. It's efficient to store the values in a few typed-arrays in JS rather than a large number of individual objects with repeating properties _etc._. To that end I have put together a React `Component` to which you can pass the library's KDB table object, and with a bit of config it'll render the values as you'd expect. It sorts values naturally and lets you drag column headers to change their widths. It's really basic, but it shows the `toString` formatting that each of the types has, and hopefully that we don't need an HTML `TABLE`  to get something on-screen. I suspect that this approach will perform better at higher row-counts just because the DOM is far less dense, and far less deep.

![table-header](https://github.com/user-attachments/assets/684101bf-3ad8-460c-a2e7-87d39929abbc)


