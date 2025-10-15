/*
This file is part of the Mg KDB-IPC C++ Library (hereinafter "The Library").

The Library is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

The Library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Affero Public License for more details.

You should have received a copy of the GNU Affero Public License along with The
Library. If not, see https://www.gnu.org/licenses/agpl.txt.
*/

#include <stdlib.h> // exit, EXIT_FAILURE

#include "KdbType.h"
#include "MgIoDefs.h" // for the mg7x::io functions; thin wrappers around network calls for testing
#include "MgCircularBuffer.h"

#include <string.h>  // strerror

#include <expected>
#include <memory>    // std::unique_ptr
#include <algorithm> // std::swap
#include <string>
#include <string_view>
#include <unordered_set>
#include <filesystem>
#include <print>

int connect_and_login(const uint16_t port);

using namespace ::mg7x;

int main(void)
{
	// Re-use the blocking connect-and-login example's socketry and handshake boilerplate,
	// yielding a tickerplant handle, tph
	const int tph = connect_and_login(static_cast<uint16_t>(30099));

	// Define a subscription request-function, evaluated on the TP, which returns us the number of
	// messages in its log-file, the file's location (so we can replay it) and the result of the
	// subscription itself, although in this example I don't care about the shape of the tables.
	// If you're doing something like this, you probably have strong expectations about what each
	// schema is.
	KdbFunction fun{"{(.u.i;.u.L;.u.sub[;`] each x)}"};
	KdbSymbolVector tbs{{"trade","quote"}};
	KdbList req{};
	req.push(fun);
	req.push(tbs);

	// Sizing the request-buffer _before_ the creation of the KdbWriter can be done with this
	// call; here we're going to use it later as a counter for a bit of debug-output
	size_t remaining = KdbUtil::ipcMessageLen(req);

	// We define a circular-buffer we'll use for this initial request as well as later on when
	// parsing the TCP request stream. At some point we'll end up with multiple messages in
	// the buffer and by using this variant we won't ever have to do a "compact" operation.
	// I've sized it quite generously.
	// The init-function returns std::expected<std::unique_ptr<CircularBuffer>,std::string>
	auto buf_res = init_circ_buffer(4 * 4096);
	if (!buf_res) {
		std::print("ERROR: {}\n", buf_res.error());
		exit(EXIT_FAILURE);
	}
	std::unique_ptr<CircularBuffer> p_buf{};
	std::swap(p_buf, buf_res.value());

	KdbIpcMessageWriter writer{KdbMsgType::SYNC, req};

	// Note that I've not included this in a loop as I'm very certain that my small subscription-
	// request, above, will fit into the 16 kB buffer provided
	WriteResult wrr = writer.write(p_buf->write_ptr(), p_buf->writeable());
	if (WriteResult::WR_OK != wrr) {
		std::print("ERROR: failed to encode the IPC subscription-request\n");
		exit(EXIT_FAILURE);
	}

	p_buf->set_appended(remaining);

	do {
		std::expected<ssize_t,int> wr_res = io::write(tph, p_buf->read_ptr(), p_buf->readable());
		if (!wr_res) {
			std::print("ERROR: while writing the subscription-request: {}\n", wr_res.error());
			exit(EXIT_FAILURE); // you may want to implement a different reaction to an error
		}
		size_t written = static_cast<size_t>(wr_res.value());
		std::print("DEBUG: wrote {} request-bytes to the tickerplant; {} bytes remain\n", written, remaining - written);
		remaining -= written;
	} while (writer.bytesRemaining() > 0);

	// We now need to unpack the first response and act on its content:
	// . parse and validate the response
	// . open and filter the TP log-file up to .u.i messages
	// . begin de-queueing the IPC messages on the TCP connection; in this case I'm trusting that
	//   out network receive-buffer is big enough that we can complete the TP-log replay before
	//   it starts rejecting data

	// Clear the cursors set during the write-request operation
	p_buf->clear();

	//
	KdbIpcMessageReader rdr{};
	ReadMsgResult rmr{};

	bool complete;

	do {
		std::expected<ssize_t,int> rd_res = io::read(tph, p_buf->write_ptr(), p_buf->writeable());
		if (!rd_res) {
			std::print("ERROR: while reading subscription-response: {}\n", strerror(rd_res.error()));
			exit(EXIT_FAILURE); // again, you may want to do something else here
		}
		// account for the bytes just read
		p_buf->set_appended(rd_res.value());
		// offer to the reader
		complete = rdr.readMsg(p_buf->read_ptr(), p_buf->readable(), rmr);
		uint64_t used = rdr.getInputBytesConsumed();

		// Ensure we update the buffer's cursors so that any additional bytes from the next message are
		// properly retained until we need them
		p_buf->set_consumed(used);

	} while(!complete);

	// Let's hope we get the response (.u.i;.u.L;...); if not, we'll exit
	if (KdbType::LIST != rmr.message->m_typ) {
		std::print("ERROR: expected a response of type KdbType::LIST, instead found {}: {}\n", rmr.message->m_typ, *rmr.message);
		exit(EXIT_FAILURE);
	}

	KdbList *list = reinterpret_cast<KdbList*>(rmr.message.release());

	std::print(" INFO: have subscription response {}\n", *list);

	// Confirm the list-length and the types of the first two elements
	if (3 != list->count()) {
		std::print("ERROR: expected a 3-element list in response; found {}\n", list->count());
		exit(EXIT_FAILURE);
	}
	if (KdbType::LONG_ATOM != list->typeAt(0) || KdbType::SYMBOL_ATOM != list->typeAt(1)) {
		std::print("ERROR: unexpected type(s) at indices [0] or [1]\n");
		exit(EXIT_FAILURE);
	}

	const KdbLongAtom *p_msg_count = reinterpret_cast<const KdbLongAtom*>(list->getObj(0));
	const KdbSymbolAtom *p_log_path = reinterpret_cast<const KdbSymbolAtom*>(list->getObj(1));

	int64_t msg_count = p_msg_count->m_val;

	// We need a bit of boilerplate to convert KDB's hsym `:/path/to/whatever into something we can
	// feed the KdbJournal::init function
	size_t pth_off = 0;
	if (':' == p_log_path->m_val.at(0) && p_log_path->m_val.length() > 1)
		pth_off = p_log_path->m_val.find_first_not_of(":");

	// unpack the journal location, open, scan for tables, rewrite into ours
	const std::string_view src_path{p_log_path->m_val.c_str() + pth_off};

	// We control the behaviour of the KdbJournal::init function via some options, which I define here.
	// We set it up as read-only, and that we don't want it to scan the log-file just yet, as we're
	// going to scan each message anyway
	struct KdbJournal::Options opts{
		.read_only = true,
		.validate_and_count_upon_init = false,
	};

	// Could use 'auto' here but want to show the return type
	std::expected<KdbJournal,std::string> jnl_res = KdbJournal::init(std::filesystem::path{src_path}, opts);
	if (!jnl_res) {
		std::print("ERROR: failed while reading remote TP log {}: {}", src_path, jnl_res.error());
		exit(EXIT_FAILURE);
	}

	KdbJournal src_jnl = jnl_res.value();

	// We provide the TP-log replay-scanner/filter with the name of the function we want to include.
	// This _could_ be something other than `upd, so we need to be specific
	std::string_view fn_name{"upd"};

	// We provide a set of table-names to the scanner. This particular implementation expects a dyadic
	// function whose first parameter is a symbol-atom, and will only report to our listener those
	// which match. Let's just say here that we want to scan just the trades to accumulate some kind
	// of intraday volume figure (I'm not going to code that up, just show how to get the messages).
	const std::unordered_set<std::string_view> names{"trade"};

	auto listener = [](const int8_t *src, uint64_t len) -> int {
		ReadBuf buf{src, len};
		KdbBase *msg;
		// The complete message in 'src' is for a function which matches
		// upd:{[T;X] ...}
		// where T~`trade. You can elect to materialise it into an on-heap object here, or do something
		// more efficient by parsing the in-memory bytes
		ReadResult rr = KdbUtil::newInstanceFromIpc(buf, &msg);
		// Check the result, of course
		if (ReadResult::RD_OK != rr) {
			std::print("ERROR: unable to reify TP-journal message\n");
			return -1; // quit replaying, please
		}
		rr = msg->read(buf);
		if (ReadResult::RD_OK != rr) {
			std::print("ERROR: unable to read TP-journal message\n");
			return -1;
		}
		// Do something here with the three-element LIST message (`upd;`trade;<data>); here I just print it:
		std::print("TRACE: seen trade-message: {}\n", *msg);

		// return 1 to indicate that all's well
		return 1;
	};

	// Vary the first argument here if you wanted (for example, after reconnecting) to skip the first N
	// messages
	auto filter = KdbJournal::mk_upd_tbl_filter(0, fn_name, names, listener);

	// Again, we could have used 'auto' here but I'm just showing the result-type. These two values are:
	// .first: the total number of messages replayed
	// .second: the number of messages which were passed to the filter
	std::expected<std::pair<uint64_t,uint64_t>,std::string> res_ps;

	std::print(" INFO: replaying TP journal file {}\n", src_path);
	// Filter the journal, replaying at most .u.i messages
	res_ps = src_jnl.filter_msgs(msg_count, filter);
	if (!res_ps) {
		std::print("ERROR: while replaying the journal: {}\n", res_ps.error());
		exit(EXIT_FAILURE);
	}

	std::print(" INFO: replayed {} messages, of which {} matched the filter\n", res_ps.value().first, res_ps.value().second);

	// Reset the KdbIpcMessageReader for re-use
	rdr.reset(); // TODO do we need this?
	// The ReadMsgResult 'rmr' object had its message .release'd above

	size_t count = 0;
	do {
		if (0 == p_buf->writeable()) {
			std::print("ERROR: I've received an IPC message > the buffer-size; given that I'm byte-scanning, rather than "
				"using a KdbIpcMessageReader object, which can incrementally consume bytes from the buffer and generate "
				"message-graph objects without needing the entire message in memory, I have to give up now.\n"
			); // need a bigger boat, YMMV
			exit(EXIT_FAILURE);
		}
		std::expected<ssize_t,int> rd_res = io::read(tph, p_buf->write_ptr(), p_buf->writeable());
		if (!rd_res) {
			std::print("ERROR: while reading TP upd-messages: {}\n", strerror(rd_res.error()));
			exit(EXIT_FAILURE);
		}
		p_buf->set_appended(rd_res.value());

		// We use this internal loop because we need to ensure that the buffer is properly drained
		// before we try to read again. For example, in this demonstrator, we could easily encode
		// 100 trade messages from our load-injector that are collected in the very first io::read,
		// above; then, if we iterated just once over filter_msg and tried to io::read, it would
		// block as no further data was pending in the network-receive buffer.
		// The thread escapes from the do/while block if insufficient bytes are present to complete
		// the filter_msg function, or once 100 messages have been received.
		do {
			// See the docs for this function in KdbType.h which discuss the return value
			int64_t len = KdbUpdMsgFilter::filter_msg(static_cast<int8_t*>(p_buf->read_ptr()), p_buf->readable(), fn_name, names);
			if (len < 0) {
				if (-1 == len) {
					// insufficient data present in the buffer
					break;
				}
				if (-2 == len) {
					std::print("ERROR: IPC parse-error\n");
					exit(EXIT_FAILURE);
				}
				p_buf->set_consumed(static_cast<uint64_t>(-len));
			}
			else {
				count += 1;
				complete = rdr.readMsg(static_cast<int8_t*>(p_buf->read_ptr()), p_buf->readable(), rmr);
				if (!complete) {
					std::print(" WARN: filter_msg thinks this is complete but .readMsg doesn't...\n");
				}
				else {
					rdr.reset();
					std::print("TRACE: trade message [{:3d}] received: {}\n", count, *rmr.message.get());
					rmr.message.reset();
				}
				p_buf->set_consumed(static_cast<uint64_t>(len));
			}
		} while (count < 100);

	} while (count < 100);

	// NB the 'list' message received in response to our subscription request is not yet properly deleted.
	// Of course, we're about to exit the program but be aware we took pointers to two of its elements which
	// would be deleted and their pointers made dangling if we explicitly deleted it above. Just saying that
	// it's worth considering the lifetime of these object graphs.

	return EXIT_SUCCESS;
}
