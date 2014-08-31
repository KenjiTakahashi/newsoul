/* Mucipher - Cryptograhic library for Museek
 *
 * Copyright (C) 2003-2004 Hyriand <hyriand@thegraveyard.org>
 * Karol 'Kenji Takahashi' Woźniak © 2013 - 2014
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __UTILS_CIPHER_H__
#define __UTILS_CIPHER_H__

#include <nettle/aes.h>
#include <nettle/md5.h>
#include <nettle/sha1.h>
#include <nettle/sha2.h>
#include <string>

#define CIPHER_BLOCK(length) (((length / 16) + ((length % 16) ? 1 : 0)) * 16)

void shaBlock(unsigned char *dataIn, int len, unsigned char hashout[20]);
void sha256Block(unsigned char *dataIn, int len, unsigned char hashout[32]);
void md5Block(unsigned char *dataIn, int len, unsigned char hashout[16]);

typedef struct CIPHERCONTEXT {
    struct aes_ctx D;
    struct aes_ctx E;
} CipherContext;

void cipherKeySHA256(CipherContext* ctx, char* key, int len);
void cipherKeyMD5(CipherContext* ctx, char* key, int len);

void blockCipher(CipherContext* ctx, const std::string& dataIn, int length, unsigned char* dataOut);
void blockDecipher(CipherContext* ctx, unsigned char* dataIn, int length, unsigned char* dataOut);

void hexDigest(unsigned char *digest, int length, char* digestOut);

std::string sha256Digest(std::string &str);

#endif /* __UTILS_CIPHER_H__ */
