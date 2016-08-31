[![Build Status](https://travis-ci.org/cadrian/circus.png?branch=master)](https://travis-ci.org/cadrian/circus)

**Circus** — *Circus is a Ringmaster, keeping Credentials and User Secrets*

OK, the acronym is a bit contrived. Sorry.

No, I will not remove the red nose.

# What is it?

Circus is a multi-user credentials manager.

## Vocabulary

| Word            | Definition |
| ----            | ---- |
| **Clown**       | A non-administrator user of Circus |
| **Credentials** | A name and a Ticket (see below) |
| **Joker**       | A user secret (currently: a static password, either generated or entered by the user) |
| **Marquee**     | The web interface of Circus |
| **Ring-Master** | An administrator user of Circus |
| **Ticket**      | The static password used by a user to enter the Marquee |
| **User**        | Anyone with credentials to enter the Marquee |

## Features

* Web interface
* Multiple users (ring-masters and clowns)
* Clowns features:
  * list stored passwords
  * add or update passwords
* Ring-Masters features:
  * can only add users and reset their Ticket
  * **cannot** decrypt Clown Jokers
  * can be created either from the web interface or from command line
    (useful for first initialization)
* Clown Jokers are encrypted and can only be decrypted by the Clown
  using their Ticket
* Strong security:
  * the server OS-protected memory when dealing with passwords in
    clear
  * the CGI client is actually meant to be short-lived (no "FastCGI"
    nonsense)

## Future work

* Joker criteria for user-friendly Joker list management (tags...)
* User e-mailing (for temporary Ticket resetting)
* Mutual authentication over SSL
* PKI (delivering and storing private keys and certificates)
* Allow a Ring-Master to invalidate a user

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
Jokers and Tickets).

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
* The server will provide a mechanism to extract all the Jokers of
  a given Clown (future work).

## Future technical work

### Known issues

* XDG is not an object.
* Missing lots of tests.

### TODO list

* Using `sqlite_open_v2`, define a new vfs layer that uses the uv loop
  instead of sleeping; and maybe more operations such as encryption,
  secure malloc…
* Implement Joker tags. The database table exists, but is not used yet.
* Encrypt and sign the messages between the client and server (even
  if they are on the same machine!)

# Usage notes

## Roles

### Ring-Masters

The Ring-Masters manage the Users database: create a User (either
a Clown or a Ring-Master), reset their Ticket.

A Ring-Master *cannot*:
* Delete a user (but see future work: invalidate a user)
* See the user's passwords (neither the list nor their value)

### Clowns

The Clowns are the actual people for whom Circus was written.

The Clowns can:
* Change their own Ticket
* Create or update Jokers
* See their Jokers list
* Get a Joker value
* (see future work: add tags to the Jokers)

## Using the Marquee

The Marquee is the web interface of Circus. It contains only a few pages.

To enter the Marque, input a user name and Ticket, and click "Get
in". Depending on the user rights, the shown page will be the one for
a Ring-Matser or a Clown.

### Ring-Master

The main page of a Ring-Master allows to create or update a
user. Enter the user name, its rights (Ring-Master or Clown), and
click "Create user". (Note: creating another Ring-Master is not yet
supported.)

The create User Ticket is then displayed, with its validity
date. Please send it to the User using appropriate means. (See future
work: send an e-mail).

The only possibility here is to come back to the main page, from which
you can either create / update another User, or Disconnect.

### Clown

The main page of a Clown is a bit more complex. It allows a Clown to:
* list the Jokers
* create a Joker
* change the Ticket

Clicking on a given Joker loads its value. The value is never shown on
screen; click on "Copy to clipboard" to copy it to the clipboard. It
may then be pasted wherever needed (e.g. a web login form).

A Clown may create or update a Joker in two ways:
* either let Circus generate the Joker value, using a "recipe" which
  describes the Joker characteristics: figures, letters, symbols, and
  how many of each;
* or directly type (twice) the Joker value

When a Joker is created or updated, its value is loaded.

Changing a Ticket allows a Clown to choose their own entry
password. This must be performed before the previous Ticket is expired
(otherwise ask your Ring-Master for a new Ticket).

#### A note on Joker recipes.

The recipes take that form: `<number of characters> <character set>`
once or more.

The `<number of characters>` is either a number, or a range. In the
latter case, Circus will randomly choose the actual number of
characters within the given range.

The `<character set>` is either a combination of `n` (figures), `a`
(letters), and `s` (special symbols), or a litteral set of characters
enclosed in single or double quotes. For the given (or chosen) number
of times, Circus will randomly choose a character within the character
set and add it to the Joker value.

The sequence `<number of characters> <character set>` might be set
more than once, for instance to ensure that some character sets appear
at least once (for instance: force at least one symbol)...

At the end, all the characters of the Joker value are randomly mixed.

Examples:

| Recipe | Example | Explanation |
| ---- | ---- | ---- |
| **16ans** | `aP7f*9Gi65!wTgi-` | 16 characters that can be either figures, letters, or symbols. The default recipe. |
| **23-31an 1s** | `8Pg7tz56Ov8#0PQGi5pm07hY1hJ` | 23 to 31 characters that can be either figures or letters, and exactly one symbol |
| **4-8a4-8n** | `hP5o69G4T` | 4 to 8 letters, and 4 to 8 figures; no symbol. Note that spaces are not mandatory in the recipe. |
| **8-12"qwertyuiop"** | `pqytiopwret` | A strange recipe that provides weak passwords: between 8 and 12 characters on the first row of the keyboard… (not a good idea!) |

## Warning!!

PII logging ("Personally-Identifiable Information") may reveal
sensitive information. Its level is lower than DEBUG, on purpose. Use
with care!

# History

Circus is the successor of the venerable
[pwd](https://github.com/cadrian/pwd/) but using standard technologies
(C and mainstream libraries).

## Differences with pwd

Circus is multi-user. This allows the server to start independently
from any client, e.g. from a SysV init or systemd. That simplifies the
client code and general management.

UNIX philosophy: let the right tool do its job. Daemon maintenance is
not a goal of Circus.
