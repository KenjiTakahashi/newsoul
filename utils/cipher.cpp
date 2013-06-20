/*
 This is a part of newsoul @ http://github.com/KenjiTakahashi/newsoul
 Karol "Kenji Takahashi" Woźniak © 2013

 Copyright (C) 2003-2004 Hyriand <hyriand@thegraveyard.org>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cipher.h"

void shaBlock(unsigned char *dataIn, int len, unsigned char hashout[20]) {
    struct sha1_ctx ctx;

    sha1_init(&ctx);
    sha1_update(&ctx, len, dataIn);
    sha1_digest(&ctx, len, hashout);
}

void sha256Block(unsigned char *dataIn, int len, unsigned char hashout[32]) {
	struct sha256_ctx ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, len, dataIn);
    sha256_digest(&ctx, len, hashout);
}

void md5Block(unsigned char *dataIn, int len, unsigned char hashout[16]) {
    struct md5_ctx ctx;
    md5_init(&ctx);
    md5_update(&ctx, len, dataIn);
    md5_digest(&ctx, len, hashout);
}

void cipherKeySHA256(struct aes_ctx* ctx, char* key, int len) {
    unsigned char digest[32];
    
    sha256Block((unsigned char*)key, len, digest);
    aes_set_encrypt_key(ctx, SHA256_KEY_SIZE, digest);
}

void cipherKeyMD5(struct aes_ctx* ctx, char* key, int len) {
    unsigned char digest[16];
    
    md5Block((unsigned char*)key, len, digest);
    aes_set_encrypt_key(ctx, MD5_KEY_SIZE, digest);
}

void blockCipher(struct aes_ctx* ctx, const std::string& dataIn, int length, unsigned char* dataOut) {
    std::string buffer(dataIn);
    length = CIPHER_BLOCK(length);
    buffer.resize(length);

    aes_set_encrypt_key(ctx, SHA256_KEY_SIZE, (const uint8_t*)ctx->keys);
    aes_encrypt(ctx, length, dataOut, (unsigned char*)buffer.data());
}

void blockDecipher(struct aes_ctx* ctx, unsigned char* dataIn, int length, unsigned char* dataOut) {
    length = CIPHER_BLOCK(length);

    aes_set_decrypt_key(ctx, SHA256_KEY_SIZE, (const uint8_t*)ctx->keys);
    aes_decrypt(ctx, length, dataOut, dataIn);
}

static char hextab[] = "0123456789abcdef";

void hexDigest(unsigned char *digest, int length, char* digestOut) {
	int i;
	for(i = 0; i < length; i++) {
		digestOut[i*2] = hextab[digest[i] >> 4];
		digestOut[i*2 + 1] = hextab[digest[i] & 0x0f];
	}
	digestOut[i*2] = '\0';
}
