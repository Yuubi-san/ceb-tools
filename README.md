
# CEB tools
Tools for working with [Conversations](https://conversations.im) Encrypted
Backups

CEB is: gzip-compressed SQL `insert` statements, encrypted with AES-128-GCM,
plus a simple header.  The SQL encodes various local account data, such as
message history and OMEMO secrets, as well as login credentials.  The login
passphrase is usually also the backup encryption passphrase, the key derived
from it with 1024 rounds of PBKDF2-HMAC-SHA1.

The toolbox currently contains:

* `ceb2sqlgz` command for decrypting the backup files;
* `schema.sql` file containing accounts database table creation statements.

### Comparison with similar tools

| name            | conversions¹           | language   | license    | crypto library | memory use⁴|
| --------------- | ---------------------- | ---------- | ---------- | -------------- | ---------- |
| [ceb2txt][]     | ceb→txt²               | Java 11    | Apache 2.0 | Conscrypt      | database   |
| [ceb-extract][] |ceb→{sqlgz,db}, db→json²| Python 3.8 | *none*     | `cryptography` | ceb+gz+sql |
| [ceb2xml][]³    | ceb→db                 | Lua 5.3    | *none*     | OpenSSL        | *constant* |
| **ceb-tools**   |ceb→sqlgz, sql→databases| C++20      | GNU AGPL3+ | OpenSSL        | *constant* |

1. 'db' = SQLite 3 database.
2. Partial export (mainly messages), no particular schema.
3. XML part not actually published/finished.
4. If not '*constant*', linear is implied, with purposes indicated.  Relevant
   when handling huge backups in memory-constrained environments.

[ceb2txt]:     https://github.com/iNPUTmice/ceb2txt
[ceb-extract]: https://gitlab.com/alping/ceb-extract
[ceb2xml]:     https://code.matthewwild.co.uk/ceb2xml


## Requirements

* A C++20-ish compiler. g++ 8 or clang++ 9 will do.
* GNU Make (not *strictly* necessary – ceb2sqlgz is not terribly hard to build
manually – but the instructions below rely on it).
* *(optional)* For Android, you might want unsupported things to be cleaned out
of the produced binaries to avoid warnings printed to stderr.  The makefile does
this if `termux-elf-cleaner` is found.

To install on...

* GNU/Linux
    * Debian >= 10:
        * `# apt install g++ make`
* Android
    * Termux:
        * `$ apt install clang make termux-elf-cleaner`
* Windows
    * MSYS2 (untested):
        * `$ pacman --sync gcc make`

### Build-time dependencies

* `libcrypto`
* [unaesgcm 0.4](https://github.com/Yuubi-san/unaesgcm/tree/v0.4) (in source
form)

To install on...

* GNU/Linux
    * Debian:
        * `# apt install libssl-dev`
* Android
    * Termux:
        * `$ apt install openssl`
* Windows
    * MSYS2 (untested):
        * `$ pacman --sync openssl-devel`

If you're fetching this repo with `git clone`, add `--recurse-submodules` option
or do `git submodule init` after.

If not using git, manually place unaesgcm 0.4 sources under `unaesgcm`
directory, e.g.:
`curl https://codeload.github.com/Yuubi-san/unaesgcm/tar.gz/refs/tags/v0.4 |
tar -zx && mv unaesgcm-0.4 unaesgcm`.

### Run-time dependencies

* `libcrypto`;
* *(optional)* `gunzip` or some other gzip decompressor;
* *(optional)* `sqlite3` or any other tool that understands SQL and does with it
what you need.

To install on...

* GNU/Linux
    * Debian:
        * `# apt install libssl1.1 gzip sqlite3`
* Android
    * Termux:
        * `$ apt install openssl gzip sqlite`
* Windows
    * MSYS2 (untested):
        * `$ pacman --sync openssl gzip sqlite`


## Building

```
$ make
```


## Usage examples

***Caution: All your database are belong to us!***  (Some of) the examples below
may persist unencrypted account data, including passwords and cryptographic
secrets, to permanent storage.  User descretion is advised, full-disk
encryption is recommended.  Alternatively, see
[secure-delete](https://packages.debian.org/stable/secure-delete) and
`man shred`.

### Create an SQLite database from an account backup

```sh
ceb2sqlgz account.ceb | gunzip | cat schema.sql - | sqlite3 account.db
```
ceb2sqlgz will prompt you to enter the passphrase to decrypt the .ceb.

### ...or from a bunch of (distinct) accounts

```sh
cat schema.sql | sqlite3 accounts.db &&
for ceb in backup-2022-04-13/*.ceb; do
  ceb2sqlgz "$ceb" | gunzip | sqlite3 accounts.db
done
```

### Search things directly in SQL

```sh
ceb2sqlgz backup.ceb | gunzip | \
  sed -e's/VALUES(/VALUES\n(/' -e's/),(/),\n(/g' | \
  grep -i -A10 "cake recipe"
```
The sed incantations try to put line breaks between messages (there are 20 per
line, usually), but aren't strictly necessary.


## Security & privacy considerations

Currently no effort has been put into keeping secure the decryption
passphrase/key or CEB contents (which include private OMEMO keys, among other
potentially sensitive data), beyond not echoing the passphrase at entry and
not explicitly writing anything to non-volatile memory (storage).  Secrets can
still leak from *volatile* memory, e.g., via swap space (unless it's encrypted).


## License

[GNU Affero General Public License v3.0](COPYING.md) or later, with OpenSSL
linking exception
