**Circus** — *Circus is a Ringmaster, keeping Credentials and User Stuff*

OK, the acronym is a bit contrived. Sorry.

No, I will not remove the red nose.

# What is it?

Circus is –well, will be– a password manager.

It is meant as the successor of the venerable
[pwd](https://github.com/cadrian/pwd/) but using standard technologies
(C and mainstream libraries).

# Dependencies

* The build system: https://github.com/apenwarr/redo
* The basic data structures: https://github.com/cadrian/libcad
* JSON: https://github.com/cadrian/yacjp
* Encryption routines: libgcrypt
* Communication protocol: zmq

# Design

## High-level

A client-server design. The server is responsible of keeping the vault
data, and ensuring data integrity. Clients ask the server for data.

Communication happens in layers a la OSI: the physical layer is zmq,
although this is abstracted away in the "*channel*" concept.  Above it
the message handlers virtually to talk to each other, exchanging the
JSON-serialized messages via the channel.

The messages themselves are kept in the "*protocol*" directory. Their
source is generated from the *messages.json* configuration file, which
describes the known messages and their structure; for each message,
one or more *queries* (messages from the client to the server) and
*replies* (messages from the server back to the client).

The message handlers are implemented as message "visitors", using the
OO design pattern.

        Client                       Server
      ----------      message      ----------
     | Message  | <-------------> | Message  |
     | handlers |                 | handlers |
      ----------                   ----------
     |   JSON   |                 |   JSON   |
      ----------                   ----------
          ^                             ^
          |                             |
          v                             v
      ----------      channel      ----------
     | zmq  REP | <-------------> | zmq  REQ |
      ----------                   ----------
          ^                             ^
          |                             |
          |      The network stack      |
           -----------------------------

The *Client* messages then talk to UI (whatever kind: console, web…)
while the *Server* messages talk to the vault (that actually keeps the
passwords).

Be aware of how many times the sensitive data may be duplicated to
keep the security as high as we can. Remember that we make *passwords*
transit through it all!

## Known issues

LOG is a global variable.

XDG is not an object.

Missing lots of tests.
