/*
 * (c) 1997-2010 Netflix, Inc.  All content herein is protected by
 * U.S. copyright and other applicable intellectual property laws and
 * may not be copied without the express permission of Netflix, Inc.,
 * which reserves all rights.  Reuse of any of this content for any
 * purpose without the permission of Netflix, Inc. is strictly
 * prohibited.
 */

/*
 * Based off public domain RSA Public-Key Cryptography package by Philip J.
 * Erdelsky <pje@efgh.com>.
 * http://www.alumni.caltech.edu/~pje/
 */
#ifndef H__RSA
#define H__RSA

#include <crypto/mpuint.h>

/**
 * Perform RSA encryption or decryption (source^e mod n).
 *
 * @param[in] source base number.
 * @param[in] e exponent.
 * @param[in] n modulus.
 * @return the encrypted to decrypted value, or NULL if the operation failed. It
 *         is the caller's responsibility to delete the returned integer.
 */
mpuint *rsa_encryptdecrypt(const mpuint *source, const mpuint *e, const mpuint *n);

#endif
