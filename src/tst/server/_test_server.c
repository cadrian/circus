/*
    This file is part of Circus.

    Circus is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License.

    Circus is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Circus.  If not, see <http://www.gnu.org/licenses/>.

    Copyright © 2015-2016 Cyril Adrian <cyril.adrian@gmail.com>
*/

#include <callback.h>
#include <circus_message_impl.h>
#include <circus_xdg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <unistd.h>
#include <zmq.h>

#include "_test_server.h"

#define EXIT_BUG_ERROR 9
#define EXIT_TEST_FAILED 1
#define EXIT_SUCCESS 0

/* wait implementation inspired from http://stackoverflow.com/questions/8976004/using-waitpid-or-sigaction/8976461#8976461 */

typedef struct process_info {
   pid_t pid[2];
   struct pollfd poll[2];
   int running[2];
   int status[2];
   char *name[2];
} process_info_t;

static process_info_t process_info;

static int wait_for_processes(void) {
   int count = 0;
   while (count < 2) {
      int p = poll(process_info.poll, 2, 30000);
      if (p == 0) {
         return EXIT_TEST_FAILED;
      }
      for (int i = 0; i < 2; i++) {
         /* Has the pipe closed? */
         if ((process_info.running[i]) && (process_info.poll[i].revents & (POLLHUP | POLLERR))) {
            waitpid(process_info.pid[i], &process_info.status[i], 0);
            process_info.running[i] = 0;
            process_info.poll[i].fd = -1;
            count++;
         }
      }
   }
   return EXIT_SUCCESS;
}

static void send(circus_message_t *message, void *zmq_sock) {
   /* serialize the message to JSON */
   json_object_t *jmsg = message->serialize(message);

   /* further serialize the JSON to a string */
   char *szout = NULL;
   cad_output_stream_t *out = new_cad_output_stream_from_string(&szout, stdlib_memory);
   json_visitor_t *writer = json_write_to(out, stdlib_memory, json_compact);
   jmsg->accept(jmsg, writer);

   printf(">>>> %s\n", szout);

   /* send the message */
   zmq_msg_t msg;
   size_t len = strlen(szout) + 1;
   zmq_msg_init_size(&msg, len);
   memcpy(zmq_msg_data(&msg), szout, len);
   zmq_msg_send(&msg, zmq_sock, 0);
   zmq_msg_close(&msg);

   /* at last free the memory */
   stdlib_memory.free(szout);
   writer->free(writer);
   out->free(out);
   jmsg->accept(jmsg, json_kill());
}

static circus_message_t *recv(void *zmq_sock) {
   circus_message_t *result = NULL;

   size_t len = 0;
   char *szin = NULL;

   /* wait for the message */
   zmq_msg_t msg;
   zmq_msg_init(&msg);
   int n = zmq_msg_recv(&msg, zmq_sock, 0);
   if (n < 0) {
      printf("zmq_msg_recv failed: %d\n", n);
   } else {
      /* copy the message */
      len = zmq_msg_size(&msg);
      szin = stdlib_memory.malloc(len + 1);
      memcpy(szin, zmq_msg_data(&msg), len);
      szin[len] = 0;
   }
   zmq_msg_close(&msg);

   if (szin != NULL) {
      printf("<<<< %*s\n", (int)len, szin);

      cad_input_stream_t *in = new_cad_input_stream_from_string(szin, stdlib_memory);
      json_object_t *jin = (json_object_t*)json_parse(in, NULL, NULL, stdlib_memory);
      result = deserialize_circus_message(stdlib_memory, jin);

      /* at last free the memory */
      jin->accept(jin, json_kill());
      in->free(in);
      stdlib_memory.free(szin);
   }

   return result;
}

void send_message(circus_message_t *query, circus_message_t **reply) {
   /* now send the stream using zmq; we expect the server to use default conf */
   /* NOTE: we use low-level here, another test should use a proper zmq channel */
   void *zmq_context = zmq_ctx_new();
   if (!zmq_context) {
      printf("zmq_ctx_new failed\n");
      exit(EXIT_BUG_ERROR);
   }
   void *zmq_sock = zmq_socket(zmq_context, ZMQ_REQ);
   if (!zmq_sock) {
      printf("zmq_socket failed\n");
      exit(EXIT_BUG_ERROR);
   }
   int rc = zmq_connect(zmq_sock, "tcp://127.0.0.1:4793");
   if (rc) {
      printf("zmq_connect failed\n");
      exit(EXIT_BUG_ERROR);
   }

   printf("Sending query...\n");
   send(query, zmq_sock);
   printf("Query sent.\n");
   if (reply != NULL) {
      printf("Waiting for reply...\n");
      *reply = recv(zmq_sock);
      printf("Reply received.\n");
   }

   zmq_close(zmq_sock);
   zmq_ctx_term(zmq_context);
}

void database(const char *query, database_fn fn) {
   char *path = szprintf(stdlib_memory, NULL, "%s/vault", xdg_data_home());
   query_database(path, query, fn);
   stdlib_memory.free(path);
}

static int db_count_(int *counter, sqlite3_stmt *UNUSED(stmt)) {
   *counter = *counter + 1;
   return 0;
}

database_fn db_count(int *counter) {
   *counter = 0;
   database_fn result = alloc_callback(&db_count_, counter);
   return result;
}

static void run_server_install(void) {
   const char *h = xdg_data_home();
   if (h == NULL) {
      printf("xdg_data_home is NULL\n");
      exit(EXIT_BUG_ERROR);
   }
   char *vault_path = szprintf(stdlib_memory, NULL, "%s/vault", h);
   if (vault_path == NULL) {
      printf("vault_path is NULL\n");
      exit(EXIT_BUG_ERROR);
   }
   int n = unlink(vault_path);
   if (n != 0 && errno != ENOENT) {
      printf("unlink %s failed: %s\n", vault_path, strerror(errno));
      exit(EXIT_BUG_ERROR);
   }
   stdlib_memory.free(vault_path);
   pid_t pid_install = fork();
   if (pid_install == 0) {
      execl("../../exe/main/server.dbg.exe", "server.exe", "--install", "test", "pass", NULL);
      printf("execl: %s\n", strerror(errno));
      exit(EXIT_BUG_ERROR);
   }
   int st;
   pid_t p = waitpid(pid_install, &st, 0);
   if (p != pid_install) {
      printf("wait: %ld != %ld\n", (long int)p, (long int)pid_install);
      exit(EXIT_BUG_ERROR);
   } else if (WEXITSTATUS(st) != 0) {
      printf("server install exited with status %d\n", WEXITSTATUS(st));
      exit(EXIT_BUG_ERROR);
   } else if (WIFSIGNALED(st)) {
      printf("server install was killed by signal %d\n", WTERMSIG(st));
      exit(EXIT_BUG_ERROR);
   }
}

static int server_fn(void) {
   execl("../../exe/main/server.dbg.exe", "server.exe", NULL);
   printf("execl: %s\n", strerror(errno));
   return EXIT_BUG_ERROR;
}

static void run(int (*fn)(void)) {
   exit(fn());
}

static void start(char *name, int process_info_index, int (*fn)(void)) {
   int process_pipe[2];
   pipe(process_pipe);
   process_info.name[process_info_index] = name;
   process_info.pid[process_info_index] = fork();
   if (process_info.pid[process_info_index] < 0) {
      printf("fork (%s): %s\n", name, strerror(errno));
      exit(EXIT_BUG_ERROR);
   }
   if (process_info.pid[process_info_index] == 0) {
      /* I am the child */
      close(process_pipe[0]);
      run(fn);
   }
   close(process_pipe[1]);
   process_info.poll[process_info_index].fd = process_pipe[0];
   process_info.running[process_info_index] = 1;
}

static int check_processes(int res) {
   int i;
   for (i = 0; i < 2; i++) {
      if (process_info.running[i]) {
         printf("waitpid: %s did not finish\n", process_info.name[i]);
         res = EXIT_TEST_FAILED;
         kill(process_info.pid[i], 15);
      } else if (WIFSIGNALED(process_info.status[i])) {
         printf("waitpid: %s was killed by signal %d\n", process_info.name[i], WTERMSIG(process_info.status[i]));
         res = EXIT_TEST_FAILED;
      } else {
         int st = WEXITSTATUS(process_info.status[1]);
         if (st != 0) {
            printf("waitpid: %s exited with status %d\n", process_info.name[i], st);
            res = EXIT_TEST_FAILED;
         } else {
            printf("%s: OK\n", process_info.name[i]);
         }
      }
   }
   return res;
}

int test(int argc, char **argv, int (*client_fn)(void)) {
   if (argc > 1) {
      /* --server or --client are useful to debug independant pieces of a server test */
      if (!strcmp("--server", argv[1])) {
         run(server_fn);
         return 0;
      }
      if (!strcmp("--client", argv[1])) {
         run(client_fn);
         return 0;
      }
      printf("Invalid argument: %s\n", argv[1]);
      return 1;
   }

   /* nominal test run */

   run_server_install();

   start("server", 0, server_fn);
   start("client", 1, client_fn);

   int res = wait_for_processes();
   return check_processes(res);
}

void *check_reply(circus_message_t *reply, const char *type, const char *command, const char *error) {
   void *result = NULL;
   if (reply == NULL) {
      printf("NULL reply!\n");
   } else {
      if (strcmp(type, reply->type(reply))) {
         printf("Invalid reply: type is \"%s\"\n", reply->type(reply));
      } else if (strcmp(command, reply->command(reply))) {
         printf("Invalid reply: command is \"%s\"\n", reply->command(reply));
      } else {
         const char *err = reply->error(reply);
         if (strcmp(err, error)) {
            printf("Unexpected error: %s\n", err);
         } else {
            result = reply;
         }
      }
   }
   if (result == NULL) {
      reply->free(reply);
   }
   return result;
}

int do_login(const char *userid, const char *password, char **sessionid, char **token) {
   int result = 0;

   circus_message_query_login_t *login = new_circus_message_query_login(stdlib_memory, userid, password);
   circus_message_t *reply = NULL;
   send_message(I(login), &reply);
   circus_message_reply_login_t *loggedin = check_reply(reply, "login", "reply", "");
   if (loggedin == NULL) {
      result = 1;
   } else {
      printf("Login OK.\n");
      const char *s = loggedin->sessionid(loggedin);
      *sessionid = szprintf(stdlib_memory, NULL, "%s", s);
      const char *t = loggedin->token(loggedin);
      *token = szprintf(stdlib_memory, NULL, "%s", t);
      reply->free(reply);
   }
   I(login)->free(I(login));

   return result;
}
