#include <pcre.h>
#include <string.h>

#include "xff.h"

/**
 * @file xff.c
 * @author Aaron Bedra
 * @date 12/09/2014
 */

/**
 * Determines the IP address of the actor. If X-Forwarded-For has
 * information, the function takes the first address in the string. If
 * no XFF information is provided the function returns the
 * connected_address.
 *
 * @param connected_address The IP of the connection to the server
 * @param xff_header The contents of the X-Forwarded-For header
 */
const char *remote_address(char *connected_address, const char *xff_header)
{
  if (xff_header == NULL) {
    return connected_address;
  }

  int error_offset, subvector[30];
  const char *error, *address;

  pcre *re_compiled;

  char *regex = "\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b";

  re_compiled = pcre_compile(regex, 0, &error, &error_offset, NULL);

  int matches = pcre_exec(re_compiled, 0, xff_header, strlen(xff_header), 0, 0, subvector, 30);

  if(matches < 0) {
    return NULL;
  } else {
    pcre_get_substring(xff_header, subvector, matches, 0, &(address));
  }

  pcre_free(re_compiled);

  return address;
}
