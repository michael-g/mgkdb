## Testing whether a "Magic" or circular buffer works with `io_uring`'s fixed-buffers

Fabien Giesen christened the nose-to-tail mapping of the same underlying memory allocation as a "[Magic Buffer](https://fgiesen.wordpress.com/2012/07/21/the-magic-ring-buffer/)". In my code I've named it a circular buffer, which is clearly wrong, since only one arc is repeated ;)

This small research project tests whether the double-mapping is compatible with `io_uring`'s fixed-buffer mechanism. In short: it behaves exactly as expected, and it's virtual memory all the way down.

## How I've tested

This project contains a C++ application which creates a circular buffer of size 4096 (which can be addressed as if it were 8192 bytes). It writes a KDB IPC message at offset 4090, so that it straddles the join-point in the mapping, and then uses `io_uring_prep_write_fixed` to send the message to two connected KDB instances. Both receive the message and print the standard greeting to their consoles.

## Initial thoughts

It's fairly easy to see how this could be useful in a native-code tickerplant. Together with a kernel polling-thread you'd make no system calls when publishing. Accounting correctly for slow consumers becomes the name of the game.

It would be interesting to see whether you could _also_ write to the journal by simply specifying a start-offset which strips off the IPC header, and do all of that from the same receive-buffer. You could use multiple SQEs each with a single write or one larger `writev` SQE. I'm extemporising here, as you may have noticed ;)

While looking for `io_uring_prep_writev_fixed` (which doesn't exist at the moment), [this](https://github.com/axboe/liburing/issues/1191) `liburing` issue is interesting, and Jens Axboe [replies](https://github.com/axboe/liburing/issues/1191#issuecomment-2436616145) that the `writev` variant is on the to-do list.

Later, Pavel Begunof [comments](https://github.com/axboe/liburing/issues/1191#issuecomment-2436640290):
> The caveat is, as Jens mentioned, it's only useful for zero copy, i.e. SEND_ZC and O_DIRECT read/write.
> If you writev to a socket, it'll be copied anyway and registered buffers won't be on any help to performance.

He then references the [patch](https://lore.kernel.org/io-uring/cover.1741362889.git.asml.silence@gmail.com/) landed in version the 6.15 kernel:

> Add registered buffer support for vectored io_uring operations. That allows to pass an iovec, all entries of which must belong to and point into the same registered buffer specified by sqe->buf_index.

I'm left wondering (at 2025-10-17, but have to crack on with other things) whether this exists in the 6.15 kernel but does not yet a `liburing` "prep" function.
