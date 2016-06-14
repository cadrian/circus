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
* Vault database: sqlite
* C closures (for tests): http://www.haible.de/bruno/packages-ffcall.html

# Design

## High-level

A client-server design. The server is responsible of keeping the vault
data, and ensuring data integrity. Clients ask the server for data.

The server and the vault are multi-user.

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
      ----------      message      ----------            --------
     | Message  | <-------------> | Message  | <------> | vault  |
     | handlers |                 | handlers |          |(sqlite)|
      ----------                   ----------            --------
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

The messages are protected by a session id and a random token that
changes at each message exchange, to protect against message
replay.

The messages themselves are *not* encrypted because the client and the
server are expected to run on the *same machine*. Remote access but be
taken care of by using an encrypted protocol (ssh, https…)

Be aware of how many times the sensitive data may be duplicated to
keep the security as high as we can. Remember that we make *passwords*
transit through it all!

## Vault

At the beginning, I expected the vault to be implemented as a simple
encrypted JSON file (same as pwd). Instead:

* The server file contains the vault; it will be an sqlite database.

* The server will provide a mechanism to extract all the passwords of
  a given user.

# Differences with pwd

But that does not allow multi-user. I want multi-user to allow the
server to start independently from any client, e.g. from a sysv init
or systemd. That simplifies the client code and general
management.

UNIX philosophy: let the right tool do its job. Daemon maintenance is
not a goal of circus.

## Known issues

XDG is not an object.

Missing lots of tests.

Debug logging may reveal sensitive information. Maybe have a specific
log level for those?

## TODO

Using `sqlite_open_v2`, define a new vfs layer that uses the uv loop
instead of sleeping; and maybe more operations such as encryption,
secure malloc…

Implement key tags. The database table exists, but is not used yet.
