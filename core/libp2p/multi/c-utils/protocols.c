#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "libp2p/multi/c-utils/protocols.h"
#include "libp2p/multi/c-utils/varhexutils.h"

void unload_protocols(struct ProtocolListItem *head) {
  struct ProtocolListItem *current = head;
  while (current != NULL) {
    struct ProtocolListItem *next = current->next;
    free(current->current);
    free(current);
    current = next;
  }
}

/**
 * load the available protocols into the global protocol_P
 * @returns True(1) on success, otherwise 0
 */
int load_protocols(struct ProtocolListItem **head) {
  unload_protocols(*head);
  int num_protocols = 14;
  int dec_code[] = {4,   41, 6,   17,  33,  132, 301,
                    302, 42, 480, 443, 477, 444, 275};
  int size[] = {32, 128, 16, 16, 16, 16, 0, 0, -1, 0, 0, 0, 10, 0};
  char *name[] = {
      "ip4", "ip6",  "tcp",  "udp",   "dccp", "sctp",  "udt",
      "utp", "ipfs", "http", "https", "ws",   "onion", "libp2p-webrtc-star"};

  struct ProtocolListItem *last = NULL;

  for (int i = 0; i < num_protocols; i++) {
    struct ProtocolListItem *current_item =
        (struct ProtocolListItem *)malloc(sizeof(struct ProtocolListItem));
    current_item->current = (struct Protocol *)malloc(sizeof(struct Protocol));
    current_item->next = NULL;
    current_item->current->deccode = dec_code[i];
    strcpy(current_item->current->name, name[i]);  // NOLINT
    current_item->current->size = size[i];
    if (*head == NULL) {
      *head = current_item;
    } else {
      last->next = current_item;  // NOLINT
    }
    last = current_item;
  }
  return 1;
}

struct Protocol *proto_with_name(
    const struct ProtocolListItem *head,
    const char *proto_w_name)  // Search for Protocol with inputted name
{
  const struct ProtocolListItem *current = head;
  while (current != NULL) {
    if (strcmp(proto_w_name, current->current->name) == 0) {
      return current->current;
    }
    current = current->next;
  }
  return NULL;
}

struct Protocol *proto_with_deccode(
    const struct ProtocolListItem *head,
    int proto_w_deccode)  // Search for Protocol with inputted deccode
{
  const struct ProtocolListItem *current = head;
  while (current != NULL) {
    if (current->current->deccode == proto_w_deccode) {
      return current->current;
    }
    current = current->next;
  }
  return NULL;
}
