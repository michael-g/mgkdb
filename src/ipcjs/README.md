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

### Roughly, how it all works

I'm sure if you've found your way here you already know how a browser client can request a page from a server, load its resources (like Javascript files) and then open a web-socket back to its origin. It turns out that KDB is great at serving static content and also at being the remote endpoint for web-socket traffic. And if it's a KDB instance that's both acting as web-server for the content and endpoint for the web-socket, then you avoid all the nastiness with having to do the CORS dance ... which is possible to do with KDB, with a bit of effort. 

Here's that workflow:

<img src="https://github.com/user-attachments/assets/9976353b-ec24-49b6-8900-07a2de5e6b06" width="256"/>

### Specifially, how to use it

A quick narrative might be helpful, then code examples.

First of all you need to create the web-socket. You can of course roll your own as the library doesn't require that you use the (slightly bare-bones) one that I've put together. You serialise some RPC or message that will be received in KDB's `.z.ws` handler, where it must be deserialised with `-9!`. Something in there will need to evaluate the message by inspecting its content or being really wild and just using `value`. The browser will have moved on with its life as the web-socket call is non-blocking: these are not request-response interactions.

KDB will serialise one or more messages to the browser and send each asynchronously (`neg .z.w -8! reply`). The web-socket listeners in the browser must deserialise each message and decide what to do next. And that will need a wrapper around the web-socket that does request-tracking so it can marry responses to callbacks. 

Error handling is a separate concern: the response-route needs to be able to specify that whatever's in the payload should be considered an error-signal. 

### Code Examples

Let's assume you're using the library and not using your own web-socket implementation. 

#### Set up KDB

I've included a toy `q` script called `q/serve.q`. This wires up the `.z.ws` function and checks that incoming messages' first argument matches `` `.web.request``. If so, it blithey evaluates the message payload. At this point you'd want to pause and be sure your KDB endpoint is only going to be exposed inside a highly trusted environment (_i.e._ inside your network at $Job). It provides a function `.web.getTable` which returns a table containing columns for all the types I was motivated to include. Our example will invoke this and return a `KdbTable` instance to the client's response-handler. 

Run the instance as
```bash
q q/serve.q
```
It'll bind to the port via its `.web.run` function.

#### Establish the connection

```javascript
import { MgKdb, C, } from '[..]/kdb/ipc'
const NS = {}
function onKdbConnected(conn) { /* ... */ }
function onKdbDisconnected(conn) { /* ... */ }
function onLoad() {
  const listener = {
    onConnected: onKdbConnected,
    onDisconnected: onKdbDisconnected,
  }
  NS.conn = new MgKdb.Endpoint('ws://localhost:30098', listener)
}
```
The above code is just about enough to open a connection to a KDB instance listening on the same machine on port 30098. We're passing an object as the listener which has two properties named `onConnected` and `onDisconnected` respectively. These reference the function stubs I've added by way of illustration. Once the function exits the `Endpoint` instance will be assigned to `NS.conn`.

#### Send the request

Via some event-handler, perhaps on a button click in your UI, or even in your `onConnected` implementation, serialise a message that will call the `.web.getTable` function.

There are two approaches available to you: you could either construct each element's instance via its constructor, a bit like this:
```javascript
  var func = new KdbSymbolAtom(".web.getTable")
  var timestamp = new KdbTimestampAtom(new Date())
  var request = new KdbList([func, timestamp])
```
It's a bit verbose and the `import` line will have to get bigger, but it lets you be explicit about how you compose your messages. The alternative is to use a C-like API I've cobbled together in an object called `C` in `ipc.js`. This maps construction of many of the types to simple function names, for example `C.ki(3)` will create a new `KdbIntAtom` with the value 3. The same object-graph as above can be composed as follows:
```javascript
  var func = C.ks(".web.getTable")
  var timestamp = C.kp(new Date())
  var request = C.kva(func, timestamp)
```
Once you're finished, pass `request` to the `conn.sendRequest` function, together with the callback you want invoked with the response:
```javascript
function onData(isError, data) {
  console.log("onData: ", isError, data)
}
NS.conn.sendRequest(request, onData)
```
If it's all wired-up correctly, you should see the message echoed at the KDB console, and the table of data returned into the `onData` function.

#### What's going on under the covers

The `KdbEndpoint` assigns each request sent through `sendRequest` the next integer in a sequence. It inspects the query you are sending, and inserts the `KdbIntAtom` representing the tracking ID as the first argument to the function: thus, although you've only provided one argument to `.web.getTable`, it is in fact encoding two. The reason for this is that when replying asynchronously, the library needs to know which response-handler to invoke. The response handler is added to a map keyed on the ID. When KDB encodes its reply in `serve.q`, it includes the tracking ID as its first parameter, whether the response is an error as its second, and finally the response object as the third.

The `KdbEndpoint` code strips the tracking ID away as it's meaningless to your application code, and passes the remaining two arguments to the response-handler, in this case our `onData` function.

I've extemporised most of the foregoing but I hope you get the idea; for a working implementation — albeit in React, which may not be to everyone's taste — see `app/src/AppRoot.js`. Again: you don't need the React gubbins to use the `ipc.js` library or its `KdbEndpoint` implementation. You are of course free to customise the way the request tracking works to your heart's content, but it's a fair starter for ten.


