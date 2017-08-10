/*
 * (c) 1997-2010 Netflix, Inc.  All content herein is protected by
 * U.S. copyright and other applicable intellectual property laws and
 * may not be copied without the express permission of Netflix, Inc.,
 * which reserves all rights.  Reuse of any of this content for any
 * purpose without the permission of Netflix, Inc. is strictly
 * prohibited.
 */
#include <linux/types.h>

/*
 * Based off public domain RSA Public-Key Cryptography package by Philip J.
 * Erdelsky <pje@efgh.com>.
 * http://www.alumni.caltech.edu/~pje/
 */
#ifndef H__MPUINT
#define H__MPUINT

/**
 * @struct mpuint mpuint.h
 * @brief Multiple-precision unsigned integer.
 */
struct mpuint;
typedef struct mpuint mpuint;

/**
 * Allocate a new integer of the specified element count.
 *
 * @param[in] length size in number of elements (bytes).
 * @return the new integer, which may be NULL if allocation failed.
 */
mpuint *mpuint_alloc(unsigned length);

/**
 * Deallocate an integer.
 *
 * @param[in] x integer to delete.
 */
void mpuint_delete(mpuint *x);

/**
 * Allocate and create a copy of an integer.
 *
 * @param[in] x integer to copy.
 * @return the new integer, which may be NULL if allocation failed.
 */
mpuint *mpuint_copy(const mpuint *x);

/**
 * Returns the element count currently allocated for this integer's
 * internal storage.
 *
 * @param[in] x integer to query.
 * @return the integer element count.
 */
unsigned mpuint_length(const mpuint *x);

/**
 * Resize an integer to the given length, truncating any high-order bits if the
 * new length is less than the existing one.
 *
 * @param[in] x integer to resize.
 * @param[in] length new size in number of elements (bytes).
 * @return the resized integer, which may be NULL if the resize failed (x will
 *         be deleted).
 */
mpuint *mpuint_resize(mpuint *x, unsigned length);

/**
 * Trim an integer to its required length (remove all high-order zero bits).
 *
 * @param[in] x integer to trim.
 * @return the trimmed integer, which may be NULL if the trim failed (x will be
 *         deleted).
 */
mpuint *mpuint_trim(mpuint *x);

/**
 * Returns a pointer to the internal storage currently allocated for this
 * integer. The length can be retrieved by mpuint_length(const mpuint *). Since
 * this is a pointer to the internal storage, the pointer may become invalid
 * after any operation is performed on the integer.
 *
 * There is no guarantee of the internal storage format.
 *
 * @param[in] x integer to query.
 * @return a pointer to the integer's value.
 */
const uint8_t *mpuint_value(const mpuint *x);

/**
 * Compute the quotient and remainder of x divided by y. The quotient and
 * remainder integer lengths may be increased to accomodate their values; their
 * lengths will never be decreased.
 *
 * @param[in] d the dividend.
 * @param[in] v the divisor.
 * @param[out] pq pointer to the quotient.
 * @param[out] pr pointer to the remainder.
 * @return true if the operation completed successfully, false if the divisor is
 *         zero or an operation failed (pq and pr will be deleted and set to
 *         NULL).
 */
int mpuint_divide(const mpuint *d, const mpuint *v, mpuint **pq, mpuint **pr);

/**
 * Return true if the integer is zero.
 *
 * @param[in] x the integer.
 * @return true if zero.
 */
int mpuint_iszero(const mpuint *x);

/**
 * Compares x and y.
 *
 * @param[in] x first integer.
 * @param[in] y second integer.
 * @return -1, 0, or 1 as x is less than, equal to, or greater than y.
 */
int mpuint_compare(const mpuint *x, const mpuint *y);

/**
 * Compares x and y.
 *
 * @param[in] x first integer.
 * @param[in] y second integer.
 * @return -1, 0, or 1 as x is less than, equal to, or greater than y.
 */
int mpuint_compareint(const mpuint *x, uint8_t y);

/**
 * Return true if the integers are equal.
 *
 * @param[in] x first integer.
 * @param[in] y second integer.
 * @return true if the integers are equal.
 */
int mpuint_eqint(const mpuint *x, uint8_t y);

/**
 * Return true if the integers are equal.
 *
 * @param[in] x first integer.
 * @param[in] y second integer.
 * @return true if the integers are equal.
 */
int mpuint_eq(const mpuint *x, const mpuint *y);

/**
 * Return true if one integer is less than or equal to another (x <= y).
 *
 * @param[in] x first integer.
 * @param[in] y second integer.
 * @return true if x is less than or equal to y.
 */
int mpuint_le(const mpuint *x, const mpuint *y);

/**
 * Set the value of an integer.
 *
 * @param[in] x integer to set.
 * @param[in] y new integer value.
 */
void mpuint_setint(mpuint *x, uint8_t y);

/**
 * Set the value of an integer. The integer length may be increased to
 * accomodate the new value; the length will never be decreased.
 *
 * This method is suitable for pairing with
 * mpuint_getbytes(const mpuint *, uint8_t *).
 *
 * @param[in] x integer to set.
 * @param[in] y big-endian value.
 * @param[in] len size of y in number of elements.
 * @return the set integer, or NULL if the set failed (x will be deleted).
 */
mpuint *mpuint_setbytes(mpuint *x, const uint8_t *y, unsigned len);

/**
 * Get the value of an integer. The necessary buffer size can be
 * determined by querying mpuint_length(const mpuint *) which counts
 * the number of elements. It may be wise to trim the integer before
 * getting its value.
 *
 * This method is suitable for pairing with
 * mpuint_setbytes(mpuint *, const uint8_t *, unsigned).
 *
 * @param[in] x integer to get.
 * @param[in|out] y big-endian value.
 * @return the number of elements copied into y.
 */
unsigned mpuint_getbytes(const mpuint *x, uint8_t *y);

/**
 * Set the value of an integer. The integer length may be increased to
 * accomodate the new value; the length will never be decreased.
 *
 * @param[in] x integer to set.
 * @param[in] y new integer value.
 * @return the set integer, or NULL if the set failed (x will be deleted).
 */
mpuint *mpuint_set(mpuint *x, const mpuint *y);

/**
 * Multiply an integer by another (x = x * y). The integer length may be
 * increased to accomodate the new value; the length will never be decreased.
 *
 * @param[in] x integer to change.
 * @param[in] y multiplier for x.
 * @return the changed integer, or NULL if the multiplication failed (x will be
 *         deleted.
 */
mpuint *mpuint_mul(mpuint *x, const mpuint *y);

/**
 * Add an integer to another (x = x + y). The integer length may be increased to
 * accomodate the new value; the length will never be decreased.
 *
 * @param[in] x integer to change.
 * @param[in] y integer to add.
 * @return the changed integer, or NULL if the addition failed (x will be
 *         deleted.)
 */
mpuint *mpuint_addint(mpuint *x, uint8_t y);

/**
 * Add an integer to another (x = x + y). The integer length may be increased to
 * accomodate the new value; the length will never be decreased.
 *
 * @param[in] x integer to change.
 * @param[in] y integer to add.
 * @return the changed integer, or NULL if the addition failed (x will be
 *         deleted).
 */
mpuint *mpuint_add(mpuint *x, const mpuint *y);

/**
 * Subtract an integer from another (x = x - y).
 *
 * @param[in] x integer to change.
 * @param[in] y integer to subtract.
 * @return the changed integer, or NULL if the subtraction would result in a
 *         negative value (x will be deleted).
 */
mpuint *mpuint_subint(mpuint *x, uint8_t y);

/**
 * Subtract an integer from another (x = x - y).
 *
 * @param[in] x integer to change.
 * @param[in] y integer to subtract.
 * @return the changed integer, or NULL if the subtraction would result in a
 *         negative value (x will be deleted).
 */
mpuint *mpuint_sub(mpuint *x, const mpuint *y);

/**
 * Compute the modulo (remainder) of an integer to another (x = x % y).
 *
 * @param[in] x integer to change.
 * @param[in] y modulus.
 * @return the changed integer, or NULL if the modulo failed (x will be
 *         deleted).
 */
mpuint *mpuint_mod(mpuint *x, const mpuint *y);

/**
 * Compute a power modulo some value (b^e (mod m)). I think.
 *
 * @param[in] b base.
 * @param[in] e exponent.
 * @param[in] m modulus.
 * @return the power or NULL if the operation failed. The caller must delete the
 *         returned integer.
 */
mpuint *mpuint_power(const mpuint *b, const mpuint *e, const mpuint *m);

/**
 * I'm not really sure what this does. The integer length may be increased to
 * accomodate the new value; the length will never be decreased.
 *
 * I think this is supposed to be a shift-left by one, and bit is a carry bit.
 *
 * @param[in] x integer to change.
 * @param[in] bit not sure.
 * @return the changed integer, or NULL if the shift failed (x will be deleted).
 */
mpuint *mpuint_shift(mpuint *x, unsigned bit);

#endif
