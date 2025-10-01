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
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html  325f6279-1c19-476c-bf5a-f0133c44c156 
 */
#define TA_SECURITY_API_UUID \
	{ 0x325f6279, 0x1c19, 0x476c, \
		{ 0xbf, 0x5a, 0xf0, 0x13, 0x3c, 0x44, 0xc1, 0x56} }

#define USE_CLIENT_AUTH 0
#define CLIENT_AUTH_SIZE 32

/* The function IDs implemented in this TA */
#define TA_CREATE_PRESISTENT_OBJECT	1
#define TA_WRITE_OBJECT_DATA		2
#define TA_READ_OBJECT_DATA		3
#define TA_INSTALL_PSK			4
#define TA_JOIN_REQUEST			5
#define TA_DPABC_GENERATE_KEY		6
#define TA_DPABC_READ_KEY 		7
#define TA_DPABC_SIGN			8
#define TA_ALLOCATE_OPERATION		9
#define TA_FREE_OPERATION		10
#define TA_SET_KEY			11
#define TA_CYPHER_INIT			12
#define TA_CIPHER_DO_FINAL		13
#define	TA_INSTALL_MUD_URL		14
#define TA_DERIVE_MSK			15
#define TA_SIGN				16
#define TA_GENERATE_RANDOM		17
#define TA_STORE			18
#define TA_RETREIVE			19
#define TA_AES_256			20
#define TA_AES_DECODE_256		21
#define TA_DERIVE_EDK			22
#define TA_WIPE_BOOTSTRAP		23
#define TA_CHANGE_SIGNATURE_ALG		24


#endif /*TA_DPABC_H*/
