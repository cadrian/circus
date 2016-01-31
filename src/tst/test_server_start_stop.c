#include <cad_shared.h>
#include <cad_stream.h>
#include <circus.h>
#include <circus_message_impl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <zmq.h>

#define EXIT_BUG_ERROR 9
#define EXIT_TEST_FAILED 1
#define EXIT_SUCCESS 0

static void send_message() {
   /* make a "stop" query and serialize it to json */
   circus_message_query_stop_t *stop = new_circus_message_query_stop(stdlib_memory, "", "test");
   json_object_t *jstop = I(stop)->serialize(I(stop));

   /* further serialize the json to a string */
   char *szout = NULL;
   cad_output_stream_t *out = new_cad_output_stream_from_string(&szout, stdlib_memory);
   json_visitor_t *writer = json_write_to(out, stdlib_memory, json_compact);
   jstop->accept(jstop, writer);

   printf(">>>> %s\n", szout);

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
   jstop->free(jstop);
   I(stop)->free(I(stop));

   zmq_close(zmq_sock);
   zmq_ctx_term(zmq_context);
}

int main() {
   pid_t pid_server = fork();
   if (pid_server < 0) {
      printf("fork (server): %s\n", strerror(errno));
      exit(EXIT_BUG_ERROR);
   }
   if (pid_server == 0) {
      /* I am the child */
      sleep(1);
      int e = execl("../exe/main/server.exe", "server.exe", NULL);
      if (e < 0) {
         printf("execl: %s\n", strerror(errno));
         exit(EXIT_BUG_ERROR);
      }
   }
   pid_t pid_client = fork();
   if (pid_client < 0) {
      printf("fork (client): %s\n", strerror(errno));
      exit(EXIT_BUG_ERROR);
   }
   if (pid_client == 0) {
      sleep(2);
      send_message();
      exit(0);
   }
   int status, st;
   pid_t p;

   int res = EXIT_SUCCESS;

   sleep(3);
   p = waitpid(pid_client, &status, WNOHANG);
   if (p == 0) {
      printf("waitpid: pid_client %d did not finish\n", pid_client);
      res = EXIT_TEST_FAILED;
      kill(pid_client, 15);
   } else if (p != pid_client) {
      printf("waitpid: pid_client %d != %d\n", pid_client, p);
      res = EXIT_BUG_ERROR;
   } else if (WIFSIGNALED(status)) {
      printf("waitpid: pid_client was killed by signal %d\n", WTERMSIG(status));
      res = EXIT_TEST_FAILED;
   } else {
      st = WEXITSTATUS(status);
      if (st != 0) {
         printf("waitpid: pid_client exited with status %d\n", st);
         res = EXIT_TEST_FAILED;
      }
   }
   printf("pid_client: OK\n");

   sleep(2);
   p = waitpid(pid_server, &status, WNOHANG);
   if (p == 0) {
      printf("waitpid: pid_server %d did not finish\n", pid_server);
      res = EXIT_TEST_FAILED;
      kill(pid_server, 15);
   } else if (p != pid_server) {
      printf("waitpid: pid_server %d != %d\n", pid_server, p);
      res = EXIT_BUG_ERROR;
   } else if (WIFSIGNALED(status)) {
      printf("waitpid: pid_server was killed by signal %d\n", WTERMSIG(status));
      res = EXIT_TEST_FAILED;
   } else {
      st = WEXITSTATUS(status);
      if (st != 0) {
         printf("waitpid: pid_server exited with status %d\n", st);
         res = EXIT_TEST_FAILED;
      }
   }
   printf("pid_server: OK\n");

   return res;
}
