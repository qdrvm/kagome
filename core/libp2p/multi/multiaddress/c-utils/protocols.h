#ifndef PROTOCOLS
#define PROTOCOLS

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libp2p/multi/multiaddress/c-utils/varhexutils.h"

struct Protocol {
  // char hexcode[21];
  int deccode;
  int size;
  char name[30];
};

struct ProtocolListItem {
  struct Protocol *current;
  struct ProtocolListItem *next;
};

int protocol_REMOVE_id(struct ProtocolListItem *head,
                       int remid);  // Function to remove & shift back all data,
                                    // sort of like c++ vectors.

void unload_protocols(struct ProtocolListItem *head);

/**
 * load the available protocols into the global protocol_P
 * @returns True(1) on success, otherwise 0
 */
int load_protocols(struct ProtocolListItem **head);

struct Protocol *proto_with_name(
    const struct ProtocolListItem *head,
    const char *proto_w_name);  // Search for protocol with inputted name

struct Protocol *proto_with_deccode(
    const struct ProtocolListItem *head,
    int proto_w_deccode);  // Search for protocol with inputted deccode

void protocols_with_string(const struct ProtocolListItem *head,
                           char *meee,
                           int sizi);  // NOT FINISHED, DO NOT USE!

#endif
