/*

Copyright (c) 2007, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef BROADINTER_BC_ASSERT
#define BROADINTER_BC_ASSERT

#ifndef TMAS_DEBUG
#define TMAS_DEBUG
#endif

namespace BroadInter
{

#ifndef TMAS_ASSERT

#if !defined TMAS_DEBUG

#define TMAS_ASSERT(a) do {} while(false)

#else

#if defined __linux__ && defined __GNUC__ && defined TMAS_DEBUG

void AssertFail(const char* expr, int line, const char* file, const char* function);

#define TMAS_ASSERT(x) \
	do	\
	{ \
		if (x) \
		{ \
		} \
		else \
		{ \
			AssertFail(#x, __LINE__, __FILE__, __PRETTY_FUNCTION__); \
		} \
	} while (false)

#else

#include <cassert>
#define TMAS_ASSERT(x) assert(x)

#endif	// __linux__ && TMAS_DEBUG

#endif	// TMAS_DEBUG

#endif	// TMAS_ASSERT

}

#endif
