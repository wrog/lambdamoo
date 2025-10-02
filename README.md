# LambdaMOO Server

> [!CAUTION]
>
> This is the 'dev' branch, as in development.  Meaning if
> totally latest bleeding edge is what you need, then fine.
> But you probably want to checkout the 'main' branch instead.
>
> Also, this branch gets rebased __a lot__.
> Expect surprises if you fork from here.

This is the source for the latest version of the LambdaMOO server.

Please consult [the ChangeLog](./ChangeLog.txt) for the full list
of new features, changes, and bugfixes incorporated into this release.

For legacy databases that previously ran on LambdaMOO 1.8.4 or CodePoint
(Unicode+int64) servers -- 1.9.0 being a unification of both of these
lines of development -- or earlier servers, see [Legacy database support](#user-content-legacy-database-support)
below for options/extensions you will need to enable/disable from
`./configure` in order to continue running with minimal database changes.

### Test Suite

Unlike most modern applications, the LambdaMOO test suite is in a
separate package not included here.  You should retrieve that as well
(see instructions wherever you got this from).

## Getting Started

For those of you starting fresh, what to do will depend on how you got
here.  There are two ways to proceed:

### Building from a distribution

These are the instructions for building from a distribution tarball
— a file of the form `LambdaMOO-1.9.nn-dist.tar.gz` — that you can (and
likely already have) run `tar xzf` to unpack and then `cd`ed into the
source directory created (at which point you most likely started
reading this file).

The process from here is:

```
    ./configure
    make
```

The `./configure` script may complain about missing prerequisites.
Various situations are covered below.

### Building from git

This process is more involved and requires `git`, `autoconf`,
`gperf`, and `yacc`:

```
    git clone REPOSITORY SRCDIR
    cd SRCDIR
    autoconf
    ./configure
    make
```

As of this writing (Sept. 2025), `REPOSITORY` is

   [https://github.com/wrog/lambdamoo.git](https://github.com/wrog/lambdamoo)

If this has changed and for whatever reason we were not able to put up a notice
there about where we've moved to, [the LambdaMOO site](https://lambda.moo.mud.org)
or [my own site](https://wrog.net/moo) will likely have something to say about this.

### VPATH builds

To build in a different directory from where the sources live, useful
to simultaneously create versions with different options set, you
can instead do

```
    cd BUILD
    PATH/TO/SRC/configure --srcdir=PATH/TO/SRC
    make
```

where `BUILD` is a fresh directory and `PATH/TO/SRC` is whatever path
gets you from there back to the sources.  (E.g., if you wish to build
within the subdirectory `b` of the source directory, you can:

```
    mkdir b
    cd b
    ../configure --srcdir=..
    make
```

).

VPATH builds require the source directory to be pristine (i.e., no
post-`./configure` or `make` artifacts can be present).  This means if
you have previously done any building there, you will be needing to do
at least a `make distclean` before any VPATH builds will be allowed.

Note that `autoconf` always has to be run in the source directory
(since it has to create the `./configure` script there).

### Prerequisites

#### Build tools

* `make`, preferably GNU make.
   We've tried to code portably for other versions; your mileage may vary.

   GNU make is required if you want `server_version(1)` to report
   `make` command-line arguments (that you shouldn't use anyway).

#### Build tools for building from git

* `git` (surprise).

* `autoconf` is from the GNU Autotools suite and is needed for
  building `configure` and related scripts.

  Try the latest version first.  As of this writing, `autoconf`
  versions in the range 2.69-2.73 are known to work with this
  distribution.

  Also __never__ run `autoreconf`.  We are not using the full suite.
  In particular, we are using neither `automake` nor `autoheader`,
  which will both get very confused.

* `gperf` version 3.1 or later is needed for building `keywords.c`

* A `yacc`-compatible parser generator is needed for building `parser.c`:
  Any of `yacc`, `bison`, or `byacc` will do.  (We do not use `lex`.)

#### Libraries

For all library prerequisites, the library itself may well already be
installed on your system, but you will also need the correponding
build package with the `#include` headers (the `-dev` package in
Debian/Ubuntu terminology).

In some cases, an extension may instead allow for building a
statically linked library (`--with-ucdpath`, `--with-expatpath`)
if you have the corresponding library source distribution.

* The `iconv` library is needed to support `encode_chars()` and
  `decode_chars()` which are available regardless of whether the
  Unicode extension is enabled.

* The XML extension requires the `expat` library, preferably version 2,
  though the `with-expatpath` static build option allows versions
  going back to 1.2.

* The Unicode extension requires a library to provide character data.
  The GNU project's `libunistring` is recommended and is the default.
  ICU's `libicuuc` is also supported.  `./configure` will, by default,
  find whichever of these is available, but, again, you may still need
  to install a `-dev` package to make compilation headers available.
  If both are available, you can use `--with-uclib=gnu` or `--with-uclib=icu`
  to force a particular choice.

  (unless, for some reason, you need backwards compatibility with
  H. Peter Anvin's `libucd` (used by Codepoint and restricted to
  Unicode 5.1), in which case you should clone
  `https://github.com/wrog/libucd` and check out the `dev` branch,
  which contains the necessary modifications to function with the MOO
  server, whether you build and install this as a shared library on
  your system, in which case `--with-uclib=ucd` should find it, or you
  use `--with-ucdpath=/path/to/libucd-build` to incorporate the build
  statically.)

* If Unicode is enabled then the regexp extension will require `pcre`,
  the Perl Compatible Regular Expression library to implement
  `match()` properly for strings containing non-ASCII codepoints.

  And here we mean the old version, PCRE1, not the newer and fancier
  PCRE2.  Confusingly, in Debian and Ubuntu, the package to install
  for this is actually labeled `libpcre3-dev`, I have no idea why.
  Debian has also helpfully deprecated this library so it is not
  available in the latest release (you will need to install it from
  the `bookworm` repositories or make sure you retain it when
  upgrading).  MacOS homebrew, FreeBSD, and Fedora all have perfectly
  reasonable `pcre` packages that do the right thing.

### Using ./configure

#### `options.h` is a generated file

Once you have satisfied the prerequisites, `./configure` will reach the
end of its run and you will see

```
    config.status: creating options.h
```

Which means you now have an [`options.h`](./options.h) file, which
lists all of the individual settings with the fully up-to-date and
gory details of what each setting actually does __and__ _how each
option is currently set_.

> [!NOTE]
>
> __To MOO veterans__:  Yes, as of version 1.9, `options.h` is now
> _generated_ and _read-only_.  Which may seem weird for a file that
> was originally meant to be edited, but ... keep reading.

The best and easiest way to change option settings is by rerunning
`./configure`.

#### General principles concerning `./configure`

* If you give `./configure` a `-C` flag, it will run _really, really
  fast_ because it will be remembering the results of tests it ran in
  the previous rounds and not having to re-do them.

* You can, in fact, rerun `./configure` as many times as you like —
  all it ever does is keep rewriting the same 4 or 5 files
  (`Makefile`, `config.h`, `options.h`, and a few others).  So you can
  use it repeatedly to try out its various arguments to see what they
  do to those files.

* Per the general autoconf framework for how `./configure` arguments work,

  + `--disable-OPTION` is a synonym for
    `--enable-OPTION=no`, while

  + `--enable-OPTION` with no `=` and no value is a synonym for
    `--enable-OPTION=yes`

* Once you get a configuration you like, you should, if you want to be
  _completely_ rigorous, do one last `./configure` run with your
  preferred arguments but __without__ the `-C` in order to make it start
  from scratch and be sure it's doing its tests in the right order
  (admittedly, I have never seen this make any difference).

  ...  and _then_ you run `make`.

#### Specific arguments for ./configure

Either of

```
    ./configure --help=short
    ./configure -hs
```

will list all arguments that are specific to the LambdaMOO server
build process.

> [!NOTE]
>
> For now, you need not bother looking at the *longer* `./configure -h`
> or `./configure -hr` help messages, since there has been, as
> yet, **zero** effort put into getting any of the `make install`
> options listed there — which are mainly about getting general
> purpose libraries and applications installed into the correct system
> directories — to actually work, it not yet actually being clear what
> `make install` should even do given the wide variety of server
> setups that people have.  For now, as far as we are concerned, the
> build process is complete once a new `moo` executable exists.

There are two classes of command-line arguments for `./configure`

* The **low-level** `--enable-def-`NAME arguments each correspond to a
  single #define-able NAME in `options.h`.  These arguments, in combination,
  make it possible to _completely determine_ the contents of that
  file:

  + `--disable-def-`NAME translates to
    `#undef` NAME

  + `--enable-def-`NAME`=`VALUE translates to
    `#define` NAME VALUE

    with VALUE being literally inserted, so if, say, VALUE is a C
    string constant that needs double quotes around it, then you
    need to provide those explicitly, __and__ escape them for
    your shell.  Conversely, if VALUE is part of an enumeration,
    you need to be sure there are __no__ quotes there.

  + `--enable-def-`NAME(`=yes`) translates to
    `#define` NAME `1`

  + (`--enable-def-`NAME`=` with no value translates to
    `#define` NAME `OPTION_DEFAULT`

    which you should __never__ do on a command line since _if_ this is one
    of the (newer) options that understands `OPTION_DEFAULT`, that
    will already be the default and, if not, confusing/unexpected
    things will likely happen.)

  For example:

  ```
   ./configure --enable-def-LOG_COMMANDS            \
               --enable-def-DEFAULT_FG_TICKS=75000  \
               --disable-def-INPUT_APPLY_BACKSPACE
  ```

  causes the following lines to appear in `options.h`

  ```
    #define    LOG_COMMANDS          1
    #define    DEFAULT_FG_TICKS      75000
    /* #undef  INPUT_APPLY_BACKSPACE       */
  ```

  thence ultimately producing a server that will be logging commands,
  setting a default limit of 75000 ticks on every foreground task, and
  ignoring backspaces on non-binary connections.

* The **high-level** option and extension packages, each of which
  manages a collection of options, allow you to set _all_ options
  in a collection with a single argument, using some combination
  of keywords.

  + `--enable-net`

    manages all networking options (`NETWORK_PROTOCOL`,
    `NETWORK_STYLE`, `MPLEX_STYLE`, `DEFAULT_CONNECT_FILE`,
    `DEFAULT_PORT`, and `OUTBOUND_NETWORK`),
    thence allowing you to do, e.g.,

    ```
       ./configure --enable-net=tcp,8888,out
    ```

    which is equivalent to

    ```
       ./configure --enable-def-NETWORK_PROTOCOL=NP_TCP \
                   --enable-def-DEFAULT_PORT=8888       \
                   --enable-def-OUTBOUND_NETWORK=OBN_ON  \
    ```

    to get a server with TCP networking, default listener on port
    8888, and `open_network_connection()` enabled by default.

  + `--enable-sz`

    manages all datatype size options (`INT_TYPE_BITSIZE`,
    `FLOATING_TYPE`, `BOXED_FLOATS`) and `BYTE_QUOTA_MODEL`,
    thence allowing you to do, e.g.,

    ```
       ./configure --enable-sz=i32,fquad,box,bqhw
    ```

    which is equivalent to

    ```
       ./configure --enable-def-INT_TYPE_BITSIZE=32     \
                   --enable-def-FLOATING_TYPE=FT_QUAD   \
                   --enable-def-BOXED_FLOATS            \
                   --enable-def-BYTE_QUOTA_MODEL=BQM_HW
    ```

    to get a server with 32 bit integers and boxed quad floats (a
    strange combination, perhaps) using the hardware byte quota
    model for `object_bytes()` and `value_bytes()`.

    Most of the extensions also have corresponding option packages as
    described below.

For those running legacy (LambdaCore, JHCore, etc.) databases, there
are more examples [further down](#user-content-legacy-database-support)
specifically intended for those situations.

Note that the ordering of `./configure` arguments does not matter
since (1) the high-level packages affect disjoint sets of options, and
(2) the low-level `--enable-def-` arguments will take precedence
whenever they occur.  This way, if there's some high-level setting
that does most of what you want, you can include the additional
`--enable-def-` arguments to fix the settings it got "wrong".  You
_should_, however, try to do as much as you can with just the
high-level settings since they'll do a bit more sanity-checking than
the low-level settings can..

#### saving your ./configure arguments

If you forget what arguments you used on your last run of `./configure`

```
   ./config.status --config
```

will repeat them back to you.  Once you have a configuration you like,
you may want to save the output of this command somewhere, in, say,
a shell script to run the next time you do a `git pull` or otherwise
upgrade your server sources.

This is also available in-MOO as `server_version("config/args")`

#### verifying your option settings

If, in a successfully compiled or running server, you need to verify
that a particular option setting is what you think it is, or how a
particular `OPTION_DEFAULT` value was resolved, you can invoke
`server_version("options")` from MOO code to see all compiled-in
options or, say, `server_version("options/MPLEX_STYLE")` for just one.

#### how **NOT** to do options.h or config.h settings

As a matter of general principle, if you ever find yourself trying to
use a `-D NAME` or `-U NAME` cc argument or in a `$(CFLAGS)` or
`$(CPPFLAGS)` specification in `Makefile`(`.in`) in order to affect a
`NAME` intended to be #defined in either options.h or config.h,...

... just don't.  I can pretty much promise it won't end well.

## Optional Features

### New Options

The following options were introduced in 1.9.0.

* `BITWISE_OPERATORS` enables bitwise-operator syntax
* `INT_TYPE_BITSIZE` sets the width of the `NUM`/`INT` datatype
* `FLOATING_TYPE`, `BOXED_FLOATS` determine the `FLOAT` datatype
* `BYTE_QUOTA_MODEL`, `BQM_BOXED_FLOATS`, `BQM_INCLUDES_WAIFS`
   determine how `value_bytes()` and `object_bytes()` are calculated

The following options were introduced in 1.8.4.

* `DEFAULT_MAX_LIST_CONCAT`, `MIN_LIST_CONCAT_LIMIT`
   determines the length limit on lists
* `DEFAULT_MAX_STRING_CONCAT`, `MIN_STRING_CONCAT_LIMIT`
   determines the length limit on strings

See [`options.h`](./options.h) for the full descriptions.  Each of
these can now be set individually by `--enable-def-`OPTION_NAME
on the `./configure` command line.

### Extensions

The 1.9.0 release includes the following optional extensions and
associated new options

* `--enable-unicode`
  adds Unicode support (below) with new options
  `UNICODE_IDENTIFIERS` and `UNICODE_NUMBERS`.

* `--enable-waifs` (`=dict`, `=all`)
  adds full support for the Waif datatype including the (new option)
  `WAIF_DICT` features: Array syntax and indexed-get/set.
  `--enable-waifs=core` disables `WAIF_DICT`.

* `--enable-xml`
  adds new builtins to parse XML.

The regular expression matching facility has been repackaged as a
mandatory extension in order to expose the library choices, currently

*  `--with-relib=pcre`,
   use Perl-Compatible Regular Expressions version 1

*  `--with-relib=ylo`,
   use the original implementation (not suitable for Unicode)

For developers, the extensions framework is described in [Extensions.md](./Extensions.md).

#### Unicode support

> [!WARNING]
>
> Certain aspects of Unicode support are currently deemed
> **experimental** and will likely change in future versions.

Enabling unicode support expands the MOO character set, used for non-binary
connections, string constants, and all object/verb/property names,
to include nearly all legal Unicode codepoints.

The exact degree of Unicode support is governed by the
`--enable-unicode` command line arguments for `./configure` and (more
directly) by the `UNICODE_IDENTIFIERS`, and `UNICODE_NUMBERS` settings
in `options.h`

* The base server (no arguments or `--disable-unicode`) now includes
  five new builtin functions `ord()`,`tochar()`, `charname()`,
  `encode_chars()`, and `decode_chars()`, for transating chars and
  strings.  Even though with this setting, the MOO language and
  available string constants remain restricted to the original
  pure ASCII MOO character set, one *can* handle binary connections
  encoded in UTF-8 using "binary" ('~'-escaped) strings.  Use
  `decode_chars()` to extract lists of codepoint numbers (rather than
  `decode_binary()` to extract lists of byte values, as per the
  original prescription for dealing with binary connections).  Then,
  on output use `encode_chars()` (instead of `encode_binary()`)
  can be used to convert a list of general Unicode code points back
  into tilde-escaped format that can then be sent via `notify()`
  to get UTF-8 sequences headed back to the client.

  This strategy can also be used for any other other connection
  encodings that `iconv` supports.

* Basic Unicode support is set up by `--enable-unicode`

  This allows most "printable" non-ASCII Unicode characters to appear
  in database strings and MOO-language string constants.  This means
  they can now be showing up in the return values of `tochar()` and
  `decode_chars()`. They can also now be be used directly on
  non-binary connections, which means they can now appear in and be
  used as arguments to `notify()`, `$do_command`, `$do_login_command`
  and `$do_out_of_band_command` and, via the built-in command parser,
  be showing up in `args`, `argstr`, `dobjstr`, and `iobjstr`.

  In this version of the world, the MOO programming language otherwise
  remains the traditional ASCII version, and thus, while `eval()`,
  `set_verb_code()`, and `.program` are now able to parse strings with
  non-ASCII characters in them, the use of such characters outside of
  string constants remains forbidden.  Verbs and properties whose
  names contain non-ASCII characters will only be referenceable via
  the indirect syntax, e.g., `o.("💩")` and `o:("💩")(@args)`.

  The functions `tonum()`, `toobj()`, and `tofloat()` continue to only
  recognize ASCII decimal digits when attempting to parse numbers out
  of strings.

* `--enable-unicode=ids` extends the MOO programming language to allow
  non-ASCII codepoints in verbcode identifiers, meaning variable names
  and direct-syntax verb/property references, by which we mean constructs like
  `a.Σ` and `a:Σ()` -- as opposed to the corresponding indirect forms
  `a.("Σ")` and `a:("Σ")()`) -- are now allowed.

  Note that `a.💩` will never be allowed because `💩` is not in the
  character class `XID_START` (sorry).

* `--enable-unicode=numbers` extends the MOO programming language to
  allow non-ASCII decimal digits in numeric constants and enables
  `tonum()`, `toobj()`, and `tofloat()` to recognize non-ASCII digits
  in string arguments.

  One change from the Codepoint version is that all digits in a given
  number are required to be from the same digit family.  For example,
  `tonum("13༥")`, a possibly-sneaky attempt to pass off a Tibetan 5 as
  a 4, will now fail to parse in 1.9.0.  You can still use Tibetan
  digits if you want, but the number has to be *entirely* Tibetan
  digits (so `tonum("135") == tonum("༡༣༥")` will work).

  However, the internal representation of numeric constants does not
  currently retain any record of the source character set, so even if
  one introduces non-ASCII numeric constants into verbcode, they will
  still be unparsed back as ASCII digits on the first database save or
  on any call to `verb_code()` and thus non-ASCII digits cannot
  currently persist as numeric constants in any database file.

* `--enable-unicode=code` or, equivalently,
  `--enable-unicode=ids,numbers` is the 4th distinct configuration and
  enables *all* Unicode options currently available.  This also
  currently corresponds to `--enable-unicode=all` though that can be
  expected to diverge in future server versions.

##### Experimental aspects of Unicode

Particular issues under consideration are as follows:

* Currently, as of 1.9.0., the "printable" MOO character set for the
  Unicode-enabled server includes everything in the legal Unicode
  range except for surrogates, the "non-character" (permanently
  reserved forever) characters (0xFE, 0xFF, 0x1FE, ...), and the ASCII
  codepoints already excluded by the traditional MOO language (control
  characters other than TAB).

  Further exclusions are likely and needed in order to better adhere
  to current Unicode best practices concerning whitespace and line
  breaking, including but not necessarily limited to

  + the non-ASCII control characters (0x80-0x9f) and

  + the additional Unicode linebreaking characters LS (0x2028, line
    separator), PS (0x2029, paragraph separator), which likely
    <em>should</em> be treated as linebreaks on non-binary connections
    but are not currently.

  __non-ASCII control characters and linebreaks should be avoided in
  string values; it is possible future server versions will forbid
  or otherwise fail to preserve them.__

  Treating `VERTICAL TAB` (0xb), `FORM FEED` (0xc), and
  `NEXT LINE` (0x85) characters as line-breaks on non-binary
  connections is also under consideration; this should be less of an
  issue since these characters have historically been ignored on
  connections and forbidden in MOO code strings.

* String relational operators, matching of strings to verb and
  property names, and the string matching done by the built-in command
  parser, which currently follows what the 1.8.3 Codepoint server did,
  is all currently unorthodox in taking no account of canonical or
  compatibility equivalence, and in not attempting any case-folding
  for non-ASCII characters even though matching of ASCII characters
  *does* remain case-insensitive.

  __Verb code that currently depends on case-insensitive relational
  operators being case-sensitive for non-ASCII characters, or on
  canonically equivalent or compatible strings comparing as different
  will likely behave differently in future server versions.__

  (In theory, if you cared about case-sensitivity, you were already
  using `strcmp()` instead of `==`,`<`, etc., never mind there are
  also arguments that `strcmp()` should still respect canonical
  equivalence even while preserving case differences).

  There are also related questions concerning how `strsub()` should
  work that have yet to be resolved.

* Corresponding issues arise for identifier matching in MOO language
  source with `UNICODE_IDENTIFERS`, though, since the identifier
  character set is already restricted to the `XID_START` and
  `XID_CONTINUE` classes, the ultimate resolution here may be different
  (there being various simpler options available in this case that
  won't work for the general case).

  Given that the current state of affairs in 1.9.0 with
  `--enable-unicode=ids` treating compatible identifiers, which in
  many cases will be visually identical, as different:

  __it is strongly recommended that you not enable recognition of
  Unicode identifiers if you do not have to__

  (i.e., #undef `UNICODE_IDENTIFERS` / do not use
  `--enable-unicode=ids`).

##### On upgrading to Unicode

Database files dumped from a stock 1.8.4- server can be loaded
directly into a unicode-enabled server.  Since ASCII representation is
a subset of UTF-8, this preserves all prior string data and verbcode.

However, if you have a database that previously contained and relies
on any strings with non-ASCII bytes (which __can__ be introduced to db
files via direct editing even though this was never officially
supported) those sequences __will__ get interpreted differently and
potentially cause trouble.  If there is any chance of this, you should
ensure your database _is_ truly pure ASCII before you attempt a
Unicode upgrade.

You will also need to revise any verbcode that implictly assumes that
all string chars will be within the ASCII range

## Legacy database support

Version 1.9, in its default installation introduces significant
changes that are likely to have consequences for existing dbs:

- The new default integer datatype is 64 bits wide, which means
  integer arithmetic wraps differently, which matters, e.g., for
  LambdaCore's `$seq_utils`, anything that uses it (e.g., `@mail`), or
  anything else that relies on `$maxint` being a certain value.

- Changes to structure layouts/sizes (the inevitable result of
  compiling on a 64-bit platform, given that all previous compilations
  of stock 1.8.4- would have to have been on a 32-bit platform due to
  the previously obsolete `configure.in`) would normally be
  invisible except in that they change the values returned by
  `object_bytes()` and `value_bytes()`, which matter, if, e.g., you
  use LambdaCore's byte-quota system.

Databases produced by pre-1.9 servers are likely to need updates in
order to function correctly on a 1.9+ server compiled with the new
default settings.

There are no "compatibility modes" _per se_, i.e., it's likely no
single argument will do everything you want, and how much you will
actually *need* to do ultimately depends on what is in your database
that you care about.

However, there exist configurations that should match up pretty well,
all things considered.  The next sections describe them.

### For users of LambdaMOO 1.8.4 and earlier versions

Here, by "LambdaMOO 1.8.4", since there was never any actual 1.8.4
release, we mean a server compiled from either the (final)
2010 sourceforge repository head or `github.com/wrog/lambdamoo`
(tag "1.8.4", commit `232293922083623e45a11b3aff2575104ad6081a`)
—these sources both being essentially identical to what has been
running at LambdaMOO for the past 14 years — with no modifications
other than changing the `options.h` settings available in that version.

The following configuration

```
    ./configure --enable-sz=i32,bq32
```

creates an ASCII-only (still the default) server with 32-bit INTs (`i32`)
and with `object_bytes()` and `value_bytes()` returning results as
if the server were still running on 32 bit hardware (`bq32`).

_(You will most likely want to switch to 64-bit integers sometime
before 2038, which is coming sooner than you think.)_

Note that `--enable-sz` is entirely orthogonal to (and may thus be
safely combined with) any of the `--enable-net` settings or any
`--enable-def-`OPTION for `options.h` settings previously
available in 1.8.4 (or earlier).

You may also omit `bq32` if you are __not__ using the LambdaCore/JHCore
byte-quota system or if having the byte-quota measurement functions
return the same values as before is __not__ a priority.

### For users of Codepoint 1.8.3

This refers to the final Codepoint "unicode" branch
(commit `51761cdc98176f68a955b76a056ae364b2092a6e`,
dated May of 2011), also not an actual release.

The following configuration

```
    ./configure --enable-unicode=ids,nums --enable-sz=bq64b
```

creates a Unicode server with all subfeatures (`ids` and `numbers`)
enabled, a 64-bit INT type (default), with (`bq64b`) `object_bytes()`
and `value_bytes()` accounting for 64-bit hardware but counting floats
as boxed (probably _not_ what was intended, but this is what that server
actually did...)

Omitting `--enable-sz=bq64b` reverts to the new 1.9 default
byte-quota memory model in which floats are _not_ counted as boxed.

Also, the available Waifs (`--enable-waifs`) and XML (`--enable-xml`)
extensions are derived from and essentially equivalent to the Codepoint
"unicode-waif" and "unicode-xml" branches, except for the minor changes in
the XML functions discussed under "Changes relevant to programmers / wizards".

You may also wish to _back out_ of certain Unicode features if you can
(see [Unicode support](#user-content-unicode-support)).

## Database Format Issues

The database format has not changed.  The database version number was
not raised for the Codepoint server.  Therefore __database files do
not self-identify as to which server with which features enabled
created them__.  This can be an issue if you run multiple versions of
the server and thus have some chance of attempting to load a backup
file onto the wrong executable.  Certain combinations __will__ fail;
others will lead to mysterious behavior.

The cross-load situation most likely to cause trouble is loading a
checkpoint with non-ASCII bytes -- something that can be caught by a
simple egrep test -- onto a non-Unicode server, since, of the various
problematic cross-load situations, this one does not necessarily fail
immediately (or at all) when it arguably should.

For all situations, the full details are in [the ChangeLog](./ChangeLog.txt)
(look for the big "WARNING" under "Changes significant to people
compiling and running the server") which goes through every setting
that affects database file format and what happens with each of the
various kinds of problematic cross-loads.

The short answer for how to prevent problems is to keep your
respective backups/checkpoints segregated and properly labeled.

A slightly more sophisticated workaround would be to create, in
databases you care about, a canonical property, e.g.,
`$saved_version_info` to which the value of `server_version(1)` gets
written by `$server_started()`.  You can then, as needed, query this
value in Emergency Wizard Mode, which runs _after_ the database is
loaded but _before_ any tasks (including the `$user_disconnected` or
`$server_started` tasks that a session typically begins with)
and if you `abort` or `quit` out of Emergency Wizard Mode,
no tasks are run at all), to see what the settings were for
the instance of the server that saved this checkpoint file.

## Historical material

See [`README.1997`](./README.1997) for Pavel Curtis' original `README`,
which has important information for people living in 1997 and may also
be an interesting window on the world of 25 years ago for the rest of
us, what with its references to ancient operating systems, FTP
servers, mailing lists, and physical addresses that no longer exist.
