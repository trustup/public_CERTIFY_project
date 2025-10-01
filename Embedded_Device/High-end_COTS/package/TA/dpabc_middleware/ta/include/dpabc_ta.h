/*
 * Copyright (c) 2016-2017, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef TA_DPABC_H
#define TA_DPABC_H


/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html   3403da6d-806f-4ddd-893c-052b5be04184
 */
#define TA_DPABC_UUID \
	{ 0x3403da6d, 0x806f, 0x4ddd, \
		{ 0x89, 0x3c, 0x05, 0x2b, 0x5b, 0xe0, 0x41, 0x84} }

#define USE_CLIENT_AUTH 0
#define CLIENT_AUTH_SIZE 32

/* The function IDs implemented in this TA */
#define TA_DPABC_GENERATE_KEY		0
#define TA_DPABC_READ_KEY 		1
#define TA_DPABC_SIGN			2
#define TA_DPABC_ZKTOKEN		3
#define TA_DPABC_STORE_SIGN		4
#define TA_DPABC_SIGN_STORE		5
#define TA_DPABC_VERIFY_STORED		6
#define TA_DPABC_COMBINE_SIGNATURES	7	

#endif /*TA_DPABC_H*/
