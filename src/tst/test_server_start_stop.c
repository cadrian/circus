#include <cad_shared.h>
#include <cad_stream.h>
#include <circus.h>
#include <circus_message_impl.h>
#include <errno.h>
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
      fprintf(stderr, "zmq_ctx_new failed\n");
      exit(EXIT_BUG_ERROR);
   }
   void *zmq_sock = zmq_socket(zmq_context, ZMQ_REQ);
   if (!zmq_sock) {
      fprintf(stderr, "zmq_socket failed\n");
      exit(EXIT_BUG_ERROR);
   }
   int rc = zmq_connect(zmq_sock, "tcp://127.0.0.1:4793");
   if (rc) {
      fprintf(stderr, "zmq_connect failed\n");
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
}

int main() {
   pid_t child = fork();
   if (child < 0) {
      fprintf(stderr, "fork: %s\n", strerror(errno));
      exit(EXIT_BUG_ERROR);
   }
   if (child == 0) {
      /* I am the child */
      int e = execl("../exe/main/server.exe", "server.exe", NULL);
      if (e < 0) {
         fprintf(stderr, "execl: %s\n", strerror(errno));
         exit(EXIT_BUG_ERROR);
      }
   }
   sleep(5);
   send_message();
   int status;
   pid_t p = waitpid(child, &status, 0);
   if (p != child) {
      fprintf(stderr, "waitpid: pid %d != %d\n", child, p);
      exit(EXIT_BUG_ERROR);
   }
   if (WIFSIGNALED(status)) {
      fprintf(stderr, "waitpid: child was signalled\n");
      exit(EXIT_TEST_FAILED);
   }
   return EXIT_SUCCESS;
}
