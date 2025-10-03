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
#ifndef MG_INC_MG_K_H
#define MG_INC_MG_K_H
#pragma once

#include <stdarg.h>

#include "k.h"

inline
K mg_krr(const char *msg)
{
  return krr(const_cast<char*>(msg));
}

inline
const char* mg_sn(const char *msg, I len) noexcept
{
  return sn(const_cast<char*>(msg), len);
}

#endif
