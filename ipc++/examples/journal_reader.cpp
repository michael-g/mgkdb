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

#include <cstddef>

#include "KdbType.h"
#include <expected>
#include <string>
#include <print>

using namespace mg7x;

int main(int argc, char *argv[])
{
	KdbJournal::Options opts{
		.read_only = true,
		.validate_and_count_upon_init = true,
	};
	std::expected<KdbJournal,std::string> exp_os = KdbJournal::init("/home/michaelg/tmp/dst.journal", opts); 
	if (!exp_os) {
		std::print("ERROR: failed while opening journal: {}\n", exp_os.error());
		return EXIT_FAILURE;
	}
	KdbJournal jnl = exp_os.value();
	std::print(" INFO: opened journal {} containing {} messages\n", jnl.path().c_str(), jnl.msg_count());

	// you probably wouldn't `.validate_and_count_upon_init` if you're subsequently going to 
	// replay the journal, but for "exposition purposes", we will do just that. Here we create
	// a function that will receive a pointer and length value for all messages that match
	// the filter we'll define below:
  auto scribe = [](const int8_t *src, uint64_t len) -> int {
		// return 1 to indicate the message was used, 0 if it was skipped or -1 if an error condition
		// arose and we need the filtering to exit
		ReadBuf buf{src, len};
		if (KdbType::LIST != static_cast<KdbType>(src[0])) {
			std::print("Unexpected message type {}\n", static_cast<KdbType>(src[0]));
			return 0;
		}
		KdbBase *base = nullptr;

		// Construct a copy using the machinery in the underlying IPC library; if you were feeling
		// really perforamnce-sensitive you could operate over the in-place bytes in the log file,
		// for example, to rewrite them into a new journal somewhere else. The TPMux sibling project
		// does exactly this.
		ReadResult rr = KdbUtil::newInstanceFromIpc(buf, &base);
		if (ReadResult::RD_OK != rr || nullptr == base) {
			std::print("Failed while constructing KdbList: {}\n", rr);
			return -1;
		}
		// Having been given the "right kind of object" (a `new KdbList`), now ask it to read the
		// remaining IPC bytes into its object graph:
		rr = base->read(buf);
		if (ReadResult::RD_OK != rr) {
			std::print("Failed to materialise KdbList: {}\n", rr);
			return -1;
		}
		// Get access to the KdbList functions
		KdbList *msg = dynamic_cast<KdbList*>(base);
		if (msg->count() != 3) {
			std::print("Message does not have 3 arguments: {}\n", msg->count());
			return 0;
		}
		std::print("Have matching message: {}\n", *msg);

		delete base;

    return 1;
  };

	std::string_view fn_name{"upd"};
	// Let's skip the quotes, we only want trades:
  const std::unordered_set<std::string_view> names{"trade"};
	// Here we use the helper-function `mk_upd_tbl_filter` to match:
	// a) list-like functions with three parameters
	// b) for the `upd function 
	// c) for table-names in the set 'names'
	std::function<int(uint64_t ith, const int8_t*, uint64_t)>
	                   filter = KdbJournal::mk_upd_tbl_filter(0, fn_name, names, scribe);
	
	std::expected<std::pair<uint64_t,uint64_t>,std::string>
	                   res_ps = jnl.filter_msgs(jnl.msg_count(), filter);

	return EXIT_SUCCESS;
}

