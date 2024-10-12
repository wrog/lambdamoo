> [!WARNING]
>
> You are viewing a **__work-in-progress__** branch that is **constantly
> being rebased** and will likely be **eventually deleted**.
> **__Fork at your own risk__**.  The following text is also undergoing
> editing, i.e., _not everything mentioned below is strictly true_
> yet, as this is not yet any kind of release.

# LambdaMOO Server

This is the source for the latest version of the LambdaMOO server.

Veteran users with existing MOO databases will, at this point, want to
consult `./ChangeLog.txt`

## Getting Started

For those of you starting fresh, what to do will depend on how you got
here.  There are a number of posibilities:

### Building the LambdaMOO server from a distribution

This is arguably the simplest option for those less familiar with `git`
and the autotools.

Here, we assume you have downloaded a distribution tarball --- a file of
the form `LambdaMOO-1.9.nn-dist.tar.gz` --- from somewhere, did the
usual `tar xzf` to unpack it, `cd`ed into the directory you created,
and are now reading this file.

The process from here on is:

	./configure
	make
	make test

The `./configure` script may possibly complain about missing
pre-requisites.  Various situations are listed below.

### VPATH builds

To build in a different directory from where the sources live (useful
to simultaneously create versions with different options set), you
should instead do

	cd BUILD
	PATH/TO/SRC/configure --srcdir=PATH/TO/SRC
	make
	make test

where `BUILD` is a fresh directory and `PATH/TO/SRC` is whatever path
gets you from there back to the sources.  (E.g., you could just make a
new subdirectory of the source directory:

	mkdir b
	cd b
	../configure --srcdir=..
	make
	make test

).

Note that if you had previously done any building in the source
directory itself, you will need to do a `make distclean` there
before doing any VPATH builds.

### Building the LambdaMOO server from the git repository

This process is more involved:

	git clone https://github.com/wrog/lambdamoo SRCDIR
	cd SRCDIR
	autoconf
	./configure
	make
	make test

## Notes about prequisites that you might need

* `autoconf` is from the GNU Autotools suite.  Try the latest
   version first.  At the time of this writing, our package is known
   to work with autoconf versions in the range 2.69-2.72.

   Also __never__ run `autoreconf`.  We are not using the full suite.
   In particular, we are using neither `automake` nor `autoheader` and
   they will both get very confused.

* `gperf`, preferably version 3.1 or later

* A `yacc`-compatible parser generator: Any of `yacc`, `bison`, or
  `byacc` will do.  (We do not use `lex`.)

* If you are using the Unicode version of the server you will need
  `pcre`, the Perl Compatible Regular Expression library, by which we
  mean the old version, PCRE1, not the newer and fancier PCRE2.

  Confusingly, in Debian and Ubuntu, the package to install for
  this is actually labeled `libpcre3-dev`, I have no idea why.
  MacOS homebrew, FreeBSD, and Fedora all have perfectly
  reasonable `pcre` packages that do the right thing.

* `make`, preferably GNU make.


# Playing with options

`./configure --help=short`

will give you a summary of the configuration options available.

The last chapter of the Programmers Manual mentions some of the more
significant ones, but the full gory details are spelled out in
`options.h`

> [!NOTE]
>
> __To MOO veterans__:  Unlike with the pre-1.9.0
> server versions, you most likely __do not__ want to edit this file;
> there are now `./configure` flags that cover pretty much everything
> you are likely to need.

E.g.,

	./configure --enable-net=tcp,8888,out

sets all of `NETWORK_PROTOCOL=NP_TCP`, `DEFAULT_PORT=8888`, and
`OUTBOUND_NETWORK=OBN_ON` in a single argument.)


# Historical

See [`README.1997`](./README.1997) for Pavel Curtis' original
`README`, which has important information for people living in 1997
and may also be an interesting window on the world of 25 years ago for
the rest of us, what with its references to ancient operating systems,
FTP servers, mailing lists, and physical addresses that no longer
exist.
