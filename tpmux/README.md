## A Tickerplant Multiplexer in C++

This project implements a _selective_ tickerplant multiplexer. In other words, it can be configured to
subscribe to one or more tables each from a number of tickerplant instances. Upon subscription, it will
scan the tickerplant's log file for messages matching that TPs table-set, and incorporate those into its
own log file. Subsequent matching messages are appended to its own log and forwarded to its subscribers.

## Why would I want one?
This provides a single, sequenced stream of data for subscribers, which permits all sorts of great things.

Imagine you have a mixed trade-and-quote tickerplant feed, and during development need to resubscribe to it,
but are only interested in the trade-data. Later in the day you may as well head off for a coffee while it
chews through those N * 100 million mesasges. Using this, you'll get a trade-only log file and your life
will be immediately better.

This is designed for low-latency operation by scanning the mesasge bytes to see if they're relevant, without
materialising any objects or doing unnecessary parsing. If a message is irrelevant (_e.g._ you want trades
but it's only got quotes), it's simply dropped and never makes it out of the application receive-buffer.

This could be an interesting tool to help build sequenced streams of data for subscribers. If the output
of the Mux (multiplexer) becomes the sole event-source for clients, then not only can they be made to run
in virtual synchrony, but they have a fighting chance of becoming deterministic (especially if you can
stop them doing crazy things like scheduling tasks using wall-clock and not event-stream time). Let the
"current time" be the timestamp on the current message, and you're good (probably).

### Implemented with C++ Coroutines

The asynchronous aspects of connecting to the tickerplants, logging-in, subscribing (which is discussed
below) and receiving data are all handled by specialised C++ coroutines. These `co_await` different network
events. As written, the project implements an Epoll event-loop, but it should be simple enough to update
the abstractions to use another type. This is a Linux-centric library at the moment.

### Subscription

The tickerplants must provide a slightly different response to the `.u.sub` call, although you could easily
use a different function name (see the `kdb_subscribe_and_replay` [function](src/mg_coro_kdb_subscribe_replay.cpp#L64)).
The `.u.sub` function must reply with the number of mesasges in its log file (`.u.i`, usually), the location
of its log file (`.u.L`) and the schema(s) of the subscribed tables:
```
(.u.i;.u.L;sch)
```
The library needs these ... coordinates so it can safely parse the data in the log file. It wouldn't want
to try to read a partially-written message. The subscription is established at message index `.u.i`, and
will receive subsequent mesages over the wire anyway.

### Current Status

Under development. Hopefully it showcases useful techniques that others may find interesting.

It's got a ridiculous amount of debug statements in it, at the moment. One reason is that I've gone to
to town on the `TRACE`-level logging, the other is that I've got a Coroutine Task implementation of my
own which is extensively commented. Getting your head around C++ Coroutines is tricky enough, and made
worse by the fact that the compiler-supplied functions only exist as assembly in Compiler Explorer. I had
limit joy using Andreas Fertig's cpp-insights tool.

##### Somewhat verbose logging
![Verbose logging](https://github.com/michael-g/mgkdb/blob/master/tpmux/docs/images/2025-07-14_tpmux_verbose_logging.png "Pretty, but prolix")
##### More concice output
![Concise output](https://github.com/michael-g/mgkdb/blob/master/tpmux/docs/images/2025-07-14_tpmux_concise_logging.png "Concission at last")

Both of these can be easily changed: you can turn down the log-level in [`mg_fmt_defs.h`](include/mg_fmt_defs.h#L32)
at line 32 to be one of the `_MG_TRACE_` .. `_MG_ERROR_` values.

You can use a "Lewis Baker" symmetric-transfer coroutine implementation, or the one provided here, both
of which act in almost exactly the same way. You can select one or the other this using a macro in the
[`mg_coro_task.h`](include/mg_coro_task.h#L67) at line 67. It will use Andreas Buhr's
[cppcoro](https://github.com/andreasbuhr/cppcoro) task implementation, which CMake needs to be told about
[here](src/CMakeLists.txt#L17) at line 17.

The demo app is currently hard-coded to connect to two tickerplants on ports 30098 and 30099. It requests
a subscription to the `position` table from the former and the `trade` table from the latter. You aren't
limited to one table per tickerplant, of course, that would be silly.

There's no failure-mode coding: the library is currently "fail fast" and won't try to reconnect. There
are code-stubs in-place which upon reconnetion will fast-forward past messages that have already been
copied into the app's log-file, however, this is by no means robust or tested. I've always considered the
"success case" code-path to be about 20% of the coding-effort anyway, perhaps less! If you want to implement
one of these, I'm sure you'll figure it out.

### Running the test/example tickerplants

I've used the kdb-tick Github project as a submodule here to drive the test tickerplants I've been using
during development. I think you'll have to do the Git submodule-init dance to get these working (if you want).
I'm just using `master` which points at commit `f31eaa8b54b`.

### Licence

This sub-project of the `mgkdb` repository is licensed under the GNU Affero General Public Licence v3.0
(the AGPL) and is, of course, provided "as is", and without any warranty as to its fitness for purpose, and
you agree to use it at your own risk. Dangerous stuff, this code.
