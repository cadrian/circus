#ifndef __CIRCUS__TEST_H
#define __CIRCUS__TEST_H

void send_message(circus_message_t *query, circus_message_t **reply);

int test(int (*fn)(void));

#endif /* __CIRCUS__TEST_H */
