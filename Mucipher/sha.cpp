/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is SHA 180-1 Reference Implementation (Compact version).
 *
 * The Initial Developer of the Original Code is
 * Paul Kocher of Cryptography Research.
 * Portions created by the Initial Developer are Copyright (C) 1995-9
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "sha.h"

static void shaHashBlock(SHA_CTX *ctx);

void shaInit(SHA_CTX *ctx) {
  int i;

  ctx->lenW = 0;
  ctx->sizeHi = ctx->sizeLo = 0;

  /* Initialize H with the magic constants (see FIPS180 for constants)
   */
  ctx->H[0] = 0x67452301L;
  ctx->H[1] = 0xefcdab89L;
  ctx->H[2] = 0x98badcfeL;
  ctx->H[3] = 0x10325476L;
  ctx->H[4] = 0xc3d2e1f0L;

  for (i = 0; i < 80; i++)
    ctx->W[i] = 0;
}


void shaUpdate(SHA_CTX *ctx, unsigned char *dataIn, int len) {
  int i;

  /* Read the data into W and process blocks as they get full
   */
  for (i = 0; i < len; i++) {
    ctx->W[ctx->lenW / 4] <<= 8;
    ctx->W[ctx->lenW / 4] |= (unsigned long)dataIn[i];
    if ((++ctx->lenW) % 64 == 0) {
      shaHashBlock(ctx);
      ctx->lenW = 0;
    }
    ctx->sizeLo += 8;
    ctx->sizeHi += (ctx->sizeLo < 8);
  }
}


void shaFinal(SHA_CTX *ctx, unsigned char hashout[20]) {
  unsigned char pad0x80 = 0x80;
  unsigned char pad0x00 = 0x00;
  unsigned char padlen[8];
  int i;

  /* Pad with a binary 1 (e.g. 0x80), then zeroes, then length
   */
  padlen[0] = (unsigned char)((ctx->sizeHi >> 24) & 255);
  padlen[1] = (unsigned char)((ctx->sizeHi >> 16) & 255);
  padlen[2] = (unsigned char)((ctx->sizeHi >> 8) & 255);
  padlen[3] = (unsigned char)((ctx->sizeHi >> 0) & 255);
  padlen[4] = (unsigned char)((ctx->sizeLo >> 24) & 255);
  padlen[5] = (unsigned char)((ctx->sizeLo >> 16) & 255);
  padlen[6] = (unsigned char)((ctx->sizeLo >> 8) & 255);
  padlen[7] = (unsigned char)((ctx->sizeLo >> 0) & 255);
  shaUpdate(ctx, &pad0x80, 1);
  while (ctx->lenW != 56)
    shaUpdate(ctx, &pad0x00, 1);
  shaUpdate(ctx, padlen, 8);

  /* Output hash
   */
  for (i = 0; i < 20; i++) {
    hashout[i] = (unsigned char)(ctx->H[i / 4] >> 24);
    ctx->H[i / 4] <<= 8;
  }

  /*
   *  Re-initialize the context (also zeroizes contents)
   */
  shaInit(ctx);
}


void shaBlock(unsigned char *dataIn, int len, unsigned char hashout[20]) {
  SHA_CTX ctx;

  shaInit(&ctx);
  shaUpdate(&ctx, dataIn, len);
  shaFinal(&ctx, hashout);
}


#define SHA_ROTL(X,n) ((((X) << (n)) | ((X) >> (32-(n)))) & 0xffffffffL)

static void shaHashBlock(SHA_CTX *ctx) {
  int t;
  unsigned long A,B,C,D,E,TEMP;

  for (t = 16; t <= 79; t++)
    ctx->W[t] =
      SHA_ROTL(ctx->W[t-3] ^ ctx->W[t-8] ^ ctx->W[t-14] ^ ctx->W[t-16], 1);

  A = ctx->H[0];
  B = ctx->H[1];
  C = ctx->H[2];
  D = ctx->H[3];
  E = ctx->H[4];

  for (t = 0; t <= 19; t++) {
    TEMP = (SHA_ROTL(A,5) + (((C^D)&B)^D)     + E + ctx->W[t] + 0x5a827999L) & 0xffffffffL;
    E = D; D = C; C = SHA_ROTL(B, 30); B = A; A = TEMP;
  }
  for (t = 20; t <= 39; t++) {
    TEMP = (SHA_ROTL(A,5) + (B^C^D)           + E + ctx->W[t] + 0x6ed9eba1L) & 0xffffffffL;
    E = D; D = C; C = SHA_ROTL(B, 30); B = A; A = TEMP;
  }
  for (t = 40; t <= 59; t++) {
    TEMP = (SHA_ROTL(A,5) + ((B&C)|(D&(B|C))) + E + ctx->W[t] + 0x8f1bbcdcL) & 0xffffffffL;
    E = D; D = C; C = SHA_ROTL(B, 30); B = A; A = TEMP;
  }
  for (t = 60; t <= 79; t++) {
    TEMP = (SHA_ROTL(A,5) + (B^C^D)           + E + ctx->W[t] + 0xca62c1d6L) & 0xffffffffL;
    E = D; D = C; C = SHA_ROTL(B, 30); B = A; A = TEMP;
  }

  ctx->H[0] += A;
  ctx->H[1] += B;
  ctx->H[2] += C;
  ctx->H[3] += D;
  ctx->H[4] += E;
}

