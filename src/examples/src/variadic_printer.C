/* This file is part of the "Mg kdb+ Library" (hereinafter "The Library").
 *
 * The Library is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * The Library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License along with The
 * Library. If not, see https://www.gnu.org/licenses/agpl.txt.
 */
#include <format>
#include <print>
#include <expected>

#include <cstring>

template<typename... Args>
inline std::expected<int,std::string> send_error(const std::format_string<Args...> fmt, Args&&... args)
{
	return std::unexpected(std::vformat(fmt.get(), std::make_format_args(args...)));
}

std::expected<int,std::string> do_summat(int fail)
{
	if (0 == (fail % 4)) {
		return send_error("it failed: {}, {}\n", strerror(42), fail);
	}
	return 1;
}

int main(void)
{
	for (int i = 0 ; i < 10 ; i++) {
		auto val = do_summat(i);
		if (!val) {
			std::print("ERROR: {}\n", val.error());
		}
		else {
			std::print("RESLT: {}\n", val.value());
		}
	}


	return 0;
}
