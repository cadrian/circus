[![Build Status](https://travis-ci.org/cadrian/circus.png?branch=master)](https://travis-ci.org/cadrian/circus)

**Circus** — *Circus is a Ringmaster, keeping Credentials and User Secrets*

OK, the acronym is a bit contrived. Sorry.

No, I will not remove the red nose.

# What is it?

Circus is a multi-user credentials manager.

## Features

* Web interface
* Multiple users and administrators
* Users features:
  * list stored passwords
  * add or update passwords
* Administrators features:
  * can only add users and reset their master password
  * **cannot** decrypt user passwords
* User passwords are encrypted and can only be decrypted by the user using their master key
* Strong security:
  * the server OS-protected memory when dealing with passwords in clear
  * the CGI client is actually meant to be short-lived (no "FastCGI" nonsense)

## Future work

* User password criteria for user-friendly password list management (tags...)
* User e-mailing (for temporary master password resetting)
* Mutual authentication over SSL
* PKI (delivering and storing private keys and certificates)
* Language clarification: master password, password, key, credential…

# Design

## Dependencies

* The basic data structures: [libcad](https://github.com/cadrian/libcad)
* JSON: [YacJP](https://github.com/cadrian/yacjp)
* Encryption routines: [libgcrypt](https://www.gnu.org/software/libgcrypt/)
* Communication protocol: [zmq](http://zeromq.org/)
* Asynchronous event handling: [libuv](https://github.com/libuv/libuv)
* Vault database: [sqlite](https://www.sqlite.org/)
* C closures (for tests): [ffcall](http://www.haible.de/bruno/packages-ffcall.html)

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

* The server file contains the vault; it will be an sqlite database.
* The server will provide a mechanism to extract all the passwords of
  a given user.

## Future technical work

### Known issues

* XDG is not an object.
* Missing lots of tests.

### TODO list

* Debian packaging
* Using `sqlite_open_v2`, define a new vfs layer that uses the uv loop
  instead of sleeping; and maybe more operations such as encryption,
  secure malloc…
* Implement key tags. The database table exists, but is not used yet.
* Encrypt and sign the messages between the client and server (even
  if they are on the same machine!)

# Usage notes

## Warning!!

PII logging ("Personally-Identifiable Information") may reveal
sensitive information. Its level is lower than DEBUG, on purpose. Use
with care!

# History

Circus is the successor of the venerable
[pwd](https://github.com/cadrian/pwd/) but using standard technologies
(C and mainstream libraries).

## Differences with pwd

Circus is multi-user. this allows the
server to start independently from any client, e.g. from a sysv init
or systemd. That simplifies the client code and general
management.

UNIX philosophy: let the right tool do its job. Daemon maintenance is
not a goal of Circus.
