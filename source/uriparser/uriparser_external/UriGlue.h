#ifndef URIGLUE_H
#define URIGLUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * Transform relative URI to the target URI according to the base URI.
 *
 * @param absolute_uri     <b>IN</b>:  Reference to resolve
 * @param relative_uri     <b>IN</b>:  Base URI to apply
 * @param buffer           <b>OUT</b>: Result URI 
 * @param buffer_len       <b>IN</b>:  buffer the max length

 * @return                 true or false, if it is true, the result uri would be filled into buffer
 *
 */
bool URIParser_get_url(const char* absolute_uri, const char* relative_uri, char* buffer, uint32_t buffer_len);

#ifdef __cplusplus
}
#endif

#endif /* URIGLUE_H */