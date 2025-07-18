### Connecting, Writing & Reading Data
I've added an example using really simple blocking reads/writes to show how you would send the handshake bytes to a remote KDB process (and await its single-byte response), and write a message to it and display the response.

This library deliberately makes no attempt to own your application's TCP interactions, and you can use the bytes generated by the encoders in blocking or asynchronous comms. 

See [connect_to_kdb.cpp](connect_to_kdb.cpp) for the working example, which unless you hack the source requires a KDB remote listening on port `30998` to echo the message.

### Using `KdbJournal`

I've recently added a new helper-class `mg7x::KdbJournal` which lets you scan and do various things with an existing journal. Check [journal_reader.cpp](journal_reader.cpp) for a working example of an implementation that will filter all`` `upd`` messages for the `trade` table. In this dummy implementation, it simply prints the message content to the console, _e.g._:
```
ipc++$ bin/journal_reader
 INFO: opened journal /home/michaelg/tmp/dst.journal containing 600 messages
Have matching message: (`upd;`trade;(2025.06.10D08:00:00.000000000 2025.06.10D08:00:00.000000001 2025.06.10D08:00:00.000000002;`DHC.L`MHO.L`MGP.L;156.15260740742087 107.04381144605577 121.24007358215749f;185 189 123))
Have matching message: (`upd;`trade;((enlist 2025.06.10D08:05:06.000000000);(enlist`HCH.L);(enlist 153.50922802463174f);(enlist 184)))
Have matching message: (`upd;`trade;(2025.06.10D08:10:12.000000000 2025.06.10D08:10:12.000000001 2025.06.10D08:10:12.000000002;`KKC.L`JMA.L`JOP.L;197.97280908096582 158.23058849200606 125.56863487698138f;174 118 137))
Have matching message: (`upd;`trade;(2025.06.10D08:20:24.000000000 2025.06.10D08:20:24.000000001 2025.06.10D08:20:24.000000002;`KFL.L`BBB.L`OGO.L;199.96081616263837 174.33285135775805 123.7128796055913f;189 149 180))
Have matching message: (`upd;`trade;(2025.06.10D08:30:36.000000000 2025.06.10D08:30:36.000000001 2025.06.10D08:30:36.000000002;`BNJ.L`KON.L`HDM.L;163.571823714301 177.71303341723979 108.22687379550189f;122 151 104))
Have matching message: (`upd;`trade;(2025.06.10D08:40:48.000000000 2025.06.10D08:40:48.000000001;`GPH.L`LFM.L;111.75354989245534 192.34385737217963f;144 115))
Have matching message: (`upd;`trade;(2025.06.10D08:51:00.000000000 2025.06.10D08:51:00.000000001;`ILE.L`BFA.L;134.0392955346033 196.14594101440161f;100 155))
...
```

### Tickerplant Subscriber

The [subscribe_to_tp.cpp](subscribe_to_tp.cpp) app showcases how this library can get you connected to a tickerplant, request the location of its log-file, replay that file (while filtering just the tables you want) and then listen for subsequent messages delivered over the wire. 

It too uses blocking-IO so that the async-boilerplate doesn't get in the way. The library's components are entirely ambivalent about where their data come from: they just operate over buffers, how you get the data is up to you.

