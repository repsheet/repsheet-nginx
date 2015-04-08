#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "repsheet.h"
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
int remote_address(char *connected_address, char *xff_header, char *address)
{
  if ((connected_address == NULL && xff_header == NULL) || address == NULL) {
    return NIL;
  }

  int length;
  memset(address, '\0', INET6_ADDRSTRLEN);

  if (xff_header != NULL) {
    char *p;

    for (p = xff_header; p < (xff_header + strlen(xff_header)); p++) {
      if (*p == ' ' || *p == ',') {
        break;
      }
    }

    length = p - xff_header;
    char *test_address = malloc(length + 1);
    memcpy(test_address, xff_header, length);
    test_address[length] = '\0';

    unsigned char buf[sizeof(struct in_addr)];
    unsigned char buf6[sizeof(struct in6_addr)];

    if (inet_pton(AF_INET, (const char *)test_address, buf) == 1 || inet_pton(AF_INET6, (const char *)test_address, buf6) == 1) {
      memcpy(address, test_address, length);
      free(test_address);
      return LIBREPSHEET_OK;
    } else {
      free(test_address);
      return BLACKLISTED;
    }
  } else {
    memcpy(address, connected_address, strlen(connected_address));
    return LIBREPSHEET_OK;
  }
}
