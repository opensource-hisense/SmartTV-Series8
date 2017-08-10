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
#include <crypto/mpursa.h>

mpuint *rsa_encryptdecrypt(const mpuint *source, const mpuint *e, const mpuint *n)
{
	return mpuint_power(source, e, n);
}