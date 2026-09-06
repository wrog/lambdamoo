Extensions
==========
Contents
--------
* [Tutorial](#user-content-tutorial)
  + [Ground Rules / Preparation](#user-content-ground-rules-and-preparation)
  + [Your First Extension](#user-content-babys-very-first-extension)
  + [Bonus Round for Emacs Users](#user-content-bonus-round-for-emacs-users)

* [Some HOW-TOs](#user-content-how-tos)
  + [How to require a system shared library](#user-content-how-to-require-a-system-shared-library)
  + [How to incorporate a build directory](#user-content-how-to-incorporate-a-build-directory)

* [XT Reference](#user-content-xt-reference)
  + [About XT Syntax](#user-content-about-xt-syntax)
  + [Cmd Reference](#user-content-cmd-reference)
    - [<code>--enable-</code>](#user-content---enable--ew_name--text-),
      [<code>--with-</code>](#user-content---with----ew_name--text-),
      [<code>%cdefine</code>](#user-content-cdefine--name--value-),
      [<code>%%extension</code>](#user-content-extension-extension_name),
      [<code>%?</code>](#user-content--text),
      [<code>%?-</code>](#user-content---text),
      [<code>%?*</code>](#user-content--text-1),
      [<code>%ac</code>](#user-content-ac-m4_code),
      [<code>%ac_yes</code>](#user-content-ac_yes-m4_code),
      [<code>%alt</code>](#user-content-alt-keyword),
      [<code>%build</code>](#user-content-build),
      [<code>%dirvar</code>](#user-content-dirvar-varname),
      [<code>%disabled</code>](#user-content-disabled),
      [<code>%implies</code>](#user-content-implies-keyword),
      [<code>%lib</code>](#user-content-lib-lib_keyword-cpp_keyword),
      [<code>%lib_list</code>](#user-content-lib_list-lib_keyword--lib_keywordlib_keyword),
      [<code>%make</code>](#user-content-make-makefile_insert),
      [<code>%option_set</code>](#user-content-option_set-keyword--keywordkeyword),
      [<code>%option</code>](#user-content-option-keyword-cpp_keyword),
      [<code>%path</code>](#user-content-path-directory_path),
      [<code>%require</code>](#user-content-require-requirement_name),
      [<code>=</code> (<code><em>VAR</em> = <em>value</em></code>)](#user-content--var--value),
  + [MakeVar Reference](#user-content-makevar-reference)

* [Philosophical Issues](#user-content-philosophical-issues)
  + [When is an Option not an Option](#user-content-when-is-an-option-not-an-option)
    + [Shorter Version](#user-content-shorter-version)

Tutorial
--------
### Ground Rules and Preparation

What follows depends on `autoconf`, so we're assuming you're working
off of a `git` clone of the server sources rather than a tarball
(since you wouldn't be here unless you were interested in developing
new features and for that you need `git`; `autoconf` you do __not__
necessarily have to learn too much about; we just need you to be
running a recent version).

We will also assume you have successfully cloned the MOO server source
directory and done `autoconf && ./configure -C && make` to build a
vanilla server.  (If you hadn't actually done `-C`, that's okay, just
rerun the configure script as `./configure -C`; it should be a fast
no-op.  Also `make` again, too, if you want, but that should also be a
fast no-op.)

(... also hoping you have an editor that does syntax highlighting so
that `#` and `dnl` comments look right?  Emacs has an`autoconf` mode
that is different from its `m4` mode and you should use the former
when editing _both_ `.ac` and `.m4` files in this source tree.
See [below for further Emacs tips](#user-content-bonus-round-for-emacs-users))

In what follows, `$` at the beginning of the line is intended to be a
shell prompt -- yes, real world people will display current
directories and other things that are all irrelevant for what we're
doing.  You'll be staying in the root MOO source directory throughout.

Here is a quick convenience command you can set up:

```
$ mooval() {
  printf "%s\n" ";$1" 'abort' |
   ./moo -e -l /dev/null Minimal.db /dev/null |
   sed -e '0,/^[*]/d;/^MOO.*Bye.*NOT saving/,$d;/^$/d'
 }
```

(Cutting&pasting everything after the `$` is probably your best bet
here.  Or just save it to a file and source it from there (i.e., using
the `.` command).  And yes, we're assuming you have a modern Unix
shell that understands shell functions; any of `bash`, `ksh`, `zsh`,
or `dash` will do.  Otherwise, you will need to write an equivalent
shell script yourself and store it on your `PATH` somewhere (sorry)).

As you might guess, `mooval` runs a server, loads Minimal.db,
redirects both log and checkpoints to nowhere, fires up Emergency
Wizard Mode, evals the first argument, shows you the result, and then
exits without trying to save anything:

Here are, respectively, what successful evaluations and errors look
like in this world:

```
$ mooval 'server_version("features")'
MOO (#3): => {"regexp"}

$ mooval 'hello()'
MOO (#3): ** 1 errors during parsing:
  Line 2:  Unknown built-in function: hello
```

and just __always__ put single quotes around that first argument; it
means everything in between will be passed by the shell verbatim and
MOO almost never uses single quotes for anything, so this is about as
ideal as it gets.

(And yes, this version of `mooval` is quite simplistic.  Doubtless,
you can already see multiple ways to improve or confuse it, all
entirely beyond the scope of this document.)

### Baby's Very First Extension

0.  Copy the file [extensions2_tutorial.ac](./extensions2_tutorial.ac)
    to `extensions2.ac` (which, whenever it exists, is automatically
    hooked into the build process the way `extensions.ac` is, but
    there's no penalty for it not existing.)

  > [!IMPORTANT]
  >
  > When you are done with this tutorial, be sure to delete or
  > rename `extensions2.ac` and then do one last `autoconf -f`
  > to flush out whatever effects it's had.  Otherwise it lingers.

1.  Visit `extensions2.ac`; it's nothing but comments and is where
    we'll be doing most of our edits.  You should probably take a moment
    to read through the comments.

You may notice that the "TOP" section is entirely within square
brackets `[` `]`, which, in this edition of `m4`, are _the quote
characters_ , which means `m4` strips off the outer pair of brackets
(yes they have to nest properly) and sends everything between verbatim
as an argument to whatever the context is, in this case, we're in the
middle of a (gigantic, taking up the entire file) invocation of
`MOO_XT_DECLARE_EXTENSIONS` which interprets its first argument as
being in `MOO_XT` this weird, idiosyncratic, domain-specific language
I made up.

Here, indentation (of noncommented lines) matters.  Exact amounts
don't matter so much as "indented less" vs. "indented the same"
vs. "indented more".

And by "same", I mean "same sequence of tab and space characters".

Also, tabs __have__ to come first, trying to indent in any other way
will give you a "spaces before tabs" error).  Alsoalso each tab counts
as a Very Large number of spaces, meaning if you are going to use tabs
at all, you need to be absolutely consistent how you use them
(e.g., if tabs are 8 spaces, then __all__ instances of 8 spaces in
indentation need to be tabs)

(Suffice it to say, life will be much easier if you never use
tabs in this file, but, for the tutorial's sake, I wanted the
commented and uncommented lines to match up with each other to make it
more readable so I'm doing that __just this once__ (promise))

(And yes, the "MIDDLE" and "BOTTOM" sections work differently.
Really, they're just straight `m4` code).

Okay, enough talk.  Let's do stuff:

2.  In the "TOP" section, uncomment the `%%extension hello` line,
    (it tells you how to do this).

    Also, save the file.  (Since this will get old quickly, I'll
    assume from now on I don't have to keep telling you save the
    file after every change.)

3.  Rerun `autoconf -f && ./configure -C && make`
    (You may want to alias this to something short since you'll be
     typing it a lot.  From now on, I'm just going to say "Rebuild")

    (as for why the `-f`:  This makes `autoconf` re-read things it
     would normally skip if it has reason to believe they don't
     matter, and since `./configure` already has weird dependency
     rules due to `Makefile` itself needing to be rebuilt whenever
     `./configure` or anything else in Autoconf Land changes, and the
     GNU folks having come up with extra-clever ways for Makefile to
     behave in situations when it's rebuilding itself, and trying to
     come up with a way to _not_ include `extensions2.ac` in the
     distribution, but have `./configure` _both_ depend on
     `extensions2.ac`, _but_ quietly skip over it not being there so
     as not to confuse all of the People Who Are Not Writing
     Extensions (which is at least 7 billion more than the People
     Who _Are_ Writing Extensions), _without_ causing random, extra
     unnecessary runs of `./configure` was all making my head explode,
     all to cover situations that are normally quite rare, so I
     punted.  Yes, that was all one sentence.

     The rule is specific to `extensions2.ac` and it's simple:
     If you edit this file, you need to do `autoconf -f`.  Done.)

You should now be able to do _this_:

```
$ mooval 'server_version("features")'
MOO (#3): => {"regexp", "hello"}
```

In other words, there is now a `hello` extension.  Feel free to look
at how `configure` and `version_src.h` changed.  The extension does
absolutely nothing, but it is, at least, listed now.

It is also a _mandatory_ extension, i.e., always active,
because there is as yet no way to disable it.

4.  Uncomment the `%disabled` line.  Rebuild.

```
$ mooval 'server_version("features")'
MOO (#3): => {"regexp"}
```

So you made the extension go away again.  Congratulations.

Actually, this is important: Whatever you do, __make sure you're
cleaning up after yourself__, so that when your extension is disabled,
your impact on the existing code is __zero__.  Yes, I know I may have
violated that rule a bit for Unicode, but ... Unicode, so...  Point
is, the less you're doing when your extension is disabled, the more
The People Who Hate Your Extension will thank you.

5.  Recomment the `%disabled` line.  Rebuild.

```
$ mooval 'server_version("features")'
MOO (#3): => {"regexp", "hello"}
```

Surely, there's some better way to turn it off and on.
Perhaps a command line argument?

```
$ ./configure -C --disable-hello
configure: WARNING: unrecognized options: --disable-hello
...
$ ./configure -C --enable-hello
configure: WARNING: unrecognized options: --enable-hello
...
```

Evidently not, but maybe we can fix this?

6.  Uncomment the first `--enable-hello` line in `extensions2.ac`
    and rebuild.

Oh, look:

```
$ mooval 'server_version("features")'
MOO (#3): => {"regexp", "hello"}

$ ./configure -C --disable-hello && make
...
configure: (%%extension hello:) disabled
...
$ mooval 'server_version("features")'
MOO (#3): => {"regexp"}

$ ./configure -C --enable-hello && make
...
$ mooval 'server_version("features")'
MOO (#3): => {"regexp", "hello"}
```

No message the second time because enabled is the default.
And, yay, our extension is back.

And, in case you were wondering, the name of the extension and the
name for the `--enable-` argument do not have to be the same.

7.  Recomment `--enable-hello`, uncomment `--enable-hi`,
    and rebuild with `./configure -C --disable-hi`

Still kind of annoying that `./configure -hs` says nothing about it.
But then, we haven't specifed what it should say, so how could it know?

8.  Uncomment the two lines (`%?` and `%?-`) that are _immediately after_
    `--enable-hi`.  These are indented more, meaning they are
    subdirectives of `--enable-hi`.

    Rebuild.

And,... oh look, now we have argument descriptions:

```
$ ./configure -hs
...
  --disable-hi         no hello() function, sorry
  --enable-hi          provide a hello() function
```

If you are annoyed that these two lines are mushed in with other
options (it sort of looks like they're part of the Waifs package),
we can fix that by adding a section header:

7.  Uncomment the topmost `%?`, the one that has the _same_
    indentation as `--enable-`; it is a sibling directive and actually
    applies to the entire extension, saying in effect, "This extension
    has so much crap under it, it needs its own section header".
    Rebuild.

And now we get to see:

```
$ ./configure -hs
...
 Hello Facilities:
  --disable-hi         no hello() function, sorry
  --enable-hi          provide a hello() function
```

At this point, it's natural to think there should be some way to
_actually provide_, say, a `hello()` function.  So let's do that.

8. Recomment __everything__ except the `%%extension` line,
   so that we're back to the mandatory invisible extension
   we had as of step 4.

   Then, create a file `hello.c` with _this_ in it:

```
    /* #include "hello.h" */
    #include "bf_register.h"

    #include "config.h"
    #include "options.h"

    #include "functions.h"
    #include "storage.h"

    static package
    bf_hello(Var arglist UNUSED_, Byte  next  UNUSED_,
             void *vdata UNUSED_, Objid progr UNUSED_)
    {
        const char *msg =
    #if HELLO_ESMTP
        "ehlo world"
    #else
        "hello world"
    #endif
        ;
        return make_string_pack(str_dup(msg));
    }

    void
    register_hello(void)
    {
        register_function("hello", 0, 0, bf_hello);
    }
```

For now, assume this code works (it should!).  I could go into how
builtin functions work, what a `package` is, why you need that
`str_dup()` call, and so on, but this is described elsewhere.

The question here is how to get `make` and friends to do the right
things and get this compiled in:

9.  Uncomment the `XT_CSRCS = hello.c` line and rebuild.

For this one, you should look at what happens to `Makefile` and
`bf_register.{h,c}`.

Extension commands of the form "VAR`=`value" refer to Makefile
variables.  In most cases where the VAR is something random, this line
will just get copied in when the extension is active (when you need
your own special makevars to make life easier in other parts of your
extensions, not that you'll need to do this that often).

But certain VARs are special to the extension framework: `XT_CSRCS`
is the list of C sources that each extension potentially adds to.  In
other words, `XT_CSRCS` is going to list _all_ of the extra C source
files needed by all active extensions, not just `hello`.  (And if we
were to need to have a `hello.h` -- we do not in this version of the
world -- that's what `XT_HDRS` is for).

And then all C sources, both the common ones _and_ the "XT" ones,
(always) get scanned for registration functions -- read
`bf_register.h` if you care about how that works -- which then show up
in `bf_register.{h,c}`.  Which is then how the server knows to invoke
`register_hello()` on startup.

Which means this should all now Just Work:

```
$ mooval 'server_version("features")'
MOO (#3): => {"regexp", "hello"}

$ mooval 'hello()'
MOO (#3): => "hello world"
```

Now, as it happens, `hello.c` has some Optional Behavior,
i.e., there appear to be multiple conflicting paradigms for what
`hello()` should actually do.  And so, we have an Option,
`HELLO_ESMTP`, which presumably gets `#define`d somewhere.

In times of yore, Pavel would just add a suitable default
`#undef HELLO_ESMTP` to `options.h`, provide a description,
and if you wanted to change it, you'd edit that file manually.

Here in The Future, we are More Advanced:

10.  Put the following lines in `options.h.in` somewhere before the
     `#include "options_epilog.h"` that ends everything there (and you
     can have the comment say anything you want because we don't
     care).

```
/************************************************************
 *  The HELLO_ESMTP option is an important component of the
 *  Hello extension.  If #defined, it makes the new builtin
 *  hello() behave in a more ESMTP-ly fashion
 */

#undef HELLO_ESMTP
```

and it's `#undef` __not__ because that's the default but because
__everything__ in `options.h.in` needs to be `#undef`.

Normally, the actual default would have to be set in `options.ac`,
however since this option is essentially useless without the extension,
we'd just as soon keep this information in the same file _with_
the extension description.

You'll notice that `options.ac` consists entirely of a single call to
`MOO_DECLARE_OPTIONS`, which is an `m4` macro that can _actually_ be
invoked multiple times.

So _that_ is what the "MIDDLE" section of `extensions2.ac` (also
`extensions.ac`, for that matter) is.

11.  Uncomment the first line after `MOO_DECLARE_OPTIONS` by deleting
     the characters `dnl` from the beginning of the line.

```
$ autoconf -f && ./configure -hs
...
  --enable-def-HELLO_ESMTP          make hello() do The ESMTP Thing

$ ./configure -C --enable-def-HELLO_ESMTP && make
...
$ mooval 'hello()'
MOO (#3): => "ehlo world"
```

But really this is all `MOO_DECLARE_OPTIONS` at work.  This is how you
make a regular option and the only thing we're doing differently is
that the `MOO_DECLARE_OPTIONS` invocation is not the one that's in
`options.ac`.  Thus far, `MOO_XT_DECLARE_EXTENSIONS` is not seeing any
of this.

We haven't even put the `--enable-hi` argument back.

But maybe this is enough.

Or maybe, once we get back to the release situation where this
extension is disabled by default, and people start getting annoyed
that they have to do both `--enable-hi` _and_
`--enable-def-HELLO_ESMTP` in order to get what they want.
We can do something to make their lives easier?

Time for some option keywords.

11.  Uncomment `--enable- hi = KWD` and the next two lines (`%?-`, `%?`).
     This brings back `--enable-hi` and adds a bit more to tell people they
     can add keywords if they want (they don't have to).

     Then uncomment `%option esmtp...`, and the next two lines
     (`%alt` and `%?`).

     Finally, to make it all look truly professional, we will put back the
     `%? Hello Facilities:` section header as well.

And voilà!

```
$ autoconf -f && ./configure -hs
...
 Hello Facilities:
  --disable-hi            no hello() function, sorry
  --enable-hi=KWD,...     provide a hello() function
            ehlo|esmtp: . . do the esmtp thing
...
$ ./configure -C --enable-hi=ehlo && make && mooval 'hello()'
...
MOO (#3): => "ehlo world"

$ ./configure -C --enable-hi=esmtp && make && mooval 'hello()'
...
MOO (#3): => "ehlo world"

$ ./configure -C --enable-hi && make && mooval 'hello()'
...
MOO (#3): => "hello world"

$ ./configure -C --disable-hi && make && mooval 'hello()'
...
MOO (#3): ** 1 errors during parsing:
  Line 2:  Unknown built-in function: hello
```

Congratulations, you've completed the tutorial and earned your
Extensions White Belt.  Go forth, and remember:  With great power
comes great responsibility.

### Bonus round for Emacs users

If you want to learn more from this (meaning the following is all
optional, but really good if you can manage it), you can visit
`configure`, `Makefile`, `config.status`, `config.h`, `options.h`,
`bf_register.c`, and `version_src.h` in Emacs buffers, and then repeat
as much of the tutorial as you can manage, but, this time:

* After each rebuild, you diff the various buffers with what they've
  been been replaced with on-disk to see what got changed, then revert
  the buffers to ready them for the next command.  Just so that you
  know,
  + `autoconf` writes `configure`
  + `./configure` writes `Makefile`, `config.status`,`config.h`, and `options.h`
  + `make` writes `bf_register.c`, and `version_src.h`

* This is actually relatively easy to set up in Emacs if you
  + load my [`bdiff.el`](https://wrog.net/emacs/bdiff.el) package, which has
    * `M-x bdiff` (compare a buffer with its disk file),
    * `M-x list-munged-buffers` (list buffers whose files were changed outside of Emacs),
    * `M-x Buffer-Menu-bdiff` (bound to `c`; invokes `bdiff` from buffer listing)
    * `M-x Buffer-Menu-revert` (bound to `R`; reverts buffer from the buffer listing)

    (I wrote this rather a __long__ time ago; someone may have something better, now.)

  And then you
  + `(setq bdiff-reverse-p t)` to make `bdiff` treat the disk file as
    the newer one (rather than the older one as it usually does), and
  + set a post-compilation hook

    ```
    (defun c-f-show-munged (&rest r)
      (call-process "sh" nil '(:file "configure-hs.out") nil "-c" "./configure -hs")
      (list-munged-buffers))
    (add-hook 'compilation-finish-functions 'c-f-show-munged)
    ```

    (where you also want a buffer visiting `configure-hs.out` so you can watch that, too)
  + do `(remove-hook 'compilation-finish-functions 'c-f-show-munged)`
    when you are done with this; otherwise it's going to get annoying.


HOW-TOs
-------
On general principles, you can use [extensions.ac](./extensions.ac)
as a source of examples.

### How to require a system shared library

The bare minimum for this would be something like

```
    %%extension myx
      %require myxlib
        %lib obf
          %ac AC_SEARCH_LIBS([[obf_bar_create]], [[openbar]], [%USE%])
```

This will, if the extension is enabled, invoke `AC_SEARCH_LIBS` in the
usual way to attempt to compile with `-lopenbar` and see if the
function `obf_bar_create()` gets defined.  If the `%ac` code is
successful (indicated by expanding `%USE%`, which should be in there
literally), then this library is deemed chosen and we are done with
this requirement.  Otherwise, since this library is the only way
specified for satisfying this requirement, `./configure` will fail.

`%ac` can be omitted if you want a search that automatically succeeds
without doing anything (e.g., if you already know the library code is
present)

Multiple `%lib` stanzas get tried in order, by default.  Adding a
`--with` argument allows the user to specify a search order using the
keywords (`obf`, etc...).  `%alt` (under `%lib`) can be used to
specify library keyword aliases.

An `%ac_yes` directive (under `%require`) can be used to change the
default behavior (what happens when the search order is `yes`).

`AC_SEARCH_LIBS` takes care of setting the `LIBS` makevar, but if
anything else needs to happen then further makevar settings can appear
under `%lib` (e.g., `XT_CSRCS = myxlib.c` to add a file to the list of
sources that need to be compiled and linked in when this library is
chosen)

Typically one will also want to create a cppname for `config.h` that
gets #defined when this library is chosen.  This can be accomplished
via a second argument to `%lib`.  Alternatively, a `%cdefine`
directive (under `%require`) can be provided and given a template
(name or value) where the chosen library keyword gets filled in.

Note that requirement names (`myxlib`) are global but generally
invisible to the user; the current convention, if the extension has
only the one requirement, is to use some prefix derived from (and
preferably unique to) the extension followed by `lib`.

Library keyword names are specific to a given `%require`.

### How to incorporate a build directory

To specify a static build directory for a given library, assuming you
have already done everything above under [How to require a system
shared library](#user-content-how-to-require-a-system-shared-library)
to create this library choice, add a `%build` stanza

```
    %%extension myx
      %require myxlib
        %lib obf
          %ac AC_SEARCH_LIBS([[obf_bar_create]], [[openbar]], [%USE%])
          %build
            --with- obfpath = DIR
              %?  use this obf build
            %dirvar obf_dir

            CPPFLAGS = -I$(obf_dir)/include
            XT_LOBJS = $(obf_dir)/libobf.a

            %make <<END
$(obf_dir)/libobf.a:
	$(MAKE) -C $(obf_dir) libobf.a
END
```

Notice that the `%ac` directive is unchanged.  This means if the user
does not specify `--with-obfpath` then the `%build` stanza is ignored
and this tries `-lopenbar` as above.  What is different is that if
`--with-obfpath` **is** given, that pre-empts all `%ac` lines from
this and any other `%lib` stanzas for this requirement, no search is
conducted, and we just go straight to the specified build directory to
get the library we want.

If the corresponding system-installed dynamic library is nonexistent
or typically never gets built with the options needed, then the `%ac`
line should probably be changed to something like

```
    %ac %FAIL%[="must use static build: --with-obfpath=DIR"]
```

so that we don't try to build with it and the user gets a useful error
message.  You may also want to use `%ac_yes` to remove `obf` from the
default search list if there are alternative libraries available.

`%make` arguments are included verbatim in the Makefile and likewise
for makevar settings that are *not* on the list of special cases
(e.g., `CPPFLAGS`, `XT_CSRCS`, `XT_HDRS`, `XT_LOBJS`, where assigning
actually means append or prepend new entries to a list).  Exactly what
will be needed depends entirely on how building the library is
supposed to work, but the above is a typical recipe.

`%dirvar` is required; it's really hard to imagine how you'd write the
Makefile inserts without it.

In a development context you can use `%path` in place of
`--with-obfpath` to specify a fixed build directory and save typing,
but if you do, make sure to remove it before shipping / sending the
pull request.

------------
XT Reference
------------

### About XT Syntax

At top level, `XT` is a sequence of commands/directives/whatevers
(hereafter referred to as "cmd"s because I don't really know what to
call them).

Blank lines and comments starting with `#` are completely ignored
(except in here-docs (keep reading), where they are passed through
verbatim, though nearly always into a context that *also* treats `#`
as a comment character.)

A cmd is a single line, which may then be followed by zero or more
child cmds, to be terminated by end-of-file (i.e., the closing `]`
in the `MOO_XT_DECLARE_EXTENSIONS` invocation) or another cmd at the
same or higher level (i.e., **less** indented; yes, levels are
determined by indentation as in Python; all children of a particular
cmd must have **identical** indentation, meaning same sequence of tabs
and spaces, tabs must come first; but really, just don't use tabs).

…except there is also a provision for "here-doc" arguments:
A cmd line that ends with `<<`_keyword_ (no comments allowed) also
includes everything down to the next instance of _keyword_ on a line
by itself with no leading or trailing whitespace.  Text included in
this way is passed verbatim, with linebreaks, as a single parameter.
This allows specifying multi-line snippets of `m4` code or makefile
rules/recipes to be directly included somewhere as needed.

Most cmds are a keyword followed by whitespace-separated arguments,
though there are exceptions, noted in the individual cmd descriptions
below.  In particular, lines of the form _VARIABLE_`=`_VALUE_ are all
instances of the `=` cmd.

Because the sequence of cmds is ultimately processed by `m4`, every cmd
**must** be quote-balanced, i.e., occurences of `[` and `]` must be
always be paired and nest properly.  This also applies to here-doc
arguments, though there we only require that the **entire** here-doc be
quote-balanced (i.e., linebreaks in here-docs do **not** need to
respect the quote nesting).

Note that brackets `[]` in the cmd synopses below are used to indicate
optional parameters.  Literal square brackets should never be needed
unless a parameter really does need to be raw `m4` code, as for, e.g.,
`%ac` and `%ac_yes`.  (There is also, of course, the surrounding
context of `MOO_XT_DECLARE_EXTENSIONS`, which is also raw `m4`).

### Cmd reference

Here are all of the cmds.

The source that actually does the work is in [ac_include/ax_xt.m4](ac_include/ax_xt.m4).

#### `--enable-` _ew_name_ [`=` _text_ ]

Declares an `--enable-` argument for `./configure` for the parent
`%%extension` that will generally be of the form
`--enable-`_ew_name_`=`_keywords_, where _keywords_ is a
comma--separated list indicating which subfeatures are being chosen,
the individual possible keywords being declared via `%option` or
`%option_set` (to have a single keyword that selects multiple
subfeatures).

Note that `--enable-` arguments are expected to be all one word in
shell terms, that is, even though the declaration in `extensions.ac`
is allowed additional whitespace for clarity, the actual argument
provided on the command line must not do this (e.g.,
`--enable- unicode = ids, nums` on a command line will not be recognized;
it needs to be `--enable-unicode=ids,nums` )

#### `--with-`   _ew_name_ [`=` _text_ ]

Declares a `--with-` argument for `./configure`.

If the parent declaration is `%require`, the expected form of the argument is
`--with-`_ew_name_`=`_lib_keywords_, where _lib_keywords_ is a
comma-separated list of library keywords in preference order.  The
default (`yes` keyword) is to search for all of the libraries in order
declared.

If the parent declaration is `%build`, the expected form is
`--with-`_ew_name_`=`_directory_path_, where _ew_name_, by convention,
is expected to end in `path` (we don't enforce this yet, but maybe we
should), and _directory_path_ indicates where the build directory is
to be found relative to the MOO source directory.  This directory is
then assigned to the make variable specified by `%dirvar` and also
substituted for `%DIR%` in any `%ac` code provided.

Similarly to `--enable-`, a `--with-` argument likewise must be all
one word in shell terms.

#### `%cdefine`  _name_ [ _value_ ]

For `%%extension` this determines the extension's cppname and/or its
various option cppnames.  `%cdefine` must precede all `%option` declarations.

It is intended that an `AC_DEFINE` be issued for an extension
cppname exactly when the extension is active.  For option cppnames
an `AC_DEFINE` is issued if an option keyword is chosen either
directly or indirectly (via `%option_set` or `%implies`) by
the `--enable-`_ew_name_ argument.  Either way, if a given cppname
is _also_ defined in a `MOO_DECLARE_OPTIONS` then the corresponding
`--enable-def-` argument can override this

(... and if there's some reason that being able to override would be a
bad idea, that's then also a reason to __not__ declare it in
`MOO_DECLARE_OPTIONS` and instead put that cppname in `config.h.in`
instead of `options.h.in`).

For `%require`, this determines a requirement cppname and/or the
library cppnames.  `%cdefine` must precede all `%lib` declarations.

An `AC_DEFINE` for the requirement cppname is issued if and only if
the requirement has been satisfied, which must and can only happen
if the extension is active (if, say you want to do without having a
separate extension cppname).  An `AC_DEFINE` for a Library cppname
is issued if the library is selected.

> [!IMPORTANT]
>
> There is currently no notion of being able to override library
> selections with `--enable-def-`.  cppnames for libraries and
> requirements must go in `config.h.in`.

There are actually 3 forms of this declaration:

* `%cdefine` _cppname_

  Sets _cppname_ as the cppname for the extension or requirement,
  to be `AC_DEFINE`d if the parent extension is active or if
  the parent library requirement has been satisfied.

  In this case, an individual option or lib will only get a cppname
  if the corresponding `%option` or `%lib` cmd has a 2nd argument.

* `%cdefine` _cppname%s_template_

  Creates a cppname for each option or lib by substituting the
  `%option` or `%lib` cppname argument into _cppname_template_
  (wherever the `%s` is).

  Note that for extensions, this form allows no way to create a cppname
  for the extension.  However, you _can_ still set up an option keyword
  that is `%implied` by all of the others, which is functionally
  the same thing

* `%cdefine` _cppname_ _value%s_template_

  Sets _cppname_ as the cppname for the extension or requirement, to
  be `AC_DEFINE`d if the parent extension is active or if the parent
  library requirement has been satisfied, where the value

  + in the case of a library, is the result of substituting library
    cppname into _value_template_, or

  + in the case of an option, is a bitwise or (`|`) of the various
    substitutions of option cppnames into _valuetemplate_ correponding
    to options that have been selected,

  where, in either case, all of the possible substitution results need
  to have been given suitable distinct power-of-2 integer values in
  one of `options_epilog.h` or `config_epilog.h`, depending.

For either of the templated cases, the option or library cppname to be
substituted, if it is not specifed in the `%option` or `%lib` cmd,
defaults to the option or library keyword in all-caps.

In all cases you need to ensure that each possible cppname has an
`#undef` line in one of  `config.h.in` or `options.h.in`,
otherwise the C code won't see it.

#### `%%extension` _extension_name_

Declares an extension.
Allowed subcmds are `%?`, `%disabled`, `--enable-`, `%cdefine`,
`%option`, `%option_set`, `=`, and `%require`.

#### `%?` _text_

In general, this adds helptext to  `configure -hs`.
* For `%%extension`, creates an overall section header.
* For `--enable-` or `--with-`,
  describes what `--enable-`_ew_name  or `--with-`_ew_name does.
* For `%option` describes this option keyword.

#### `%?-` _text_

For `--enable-` or `--with-`,
describes what `--disable-`_ew_name_  or `--without-`_ew_name_ does.

#### `%?*` _text_

Add a line of description to a `--with-`.

#### `%ac` _m4_code_

Allowed in `%lib` or `%build`, this inserts the given _m4_code_,
which is expected to expand to shell code

For `%lib`, the expansion executes in the library search loop.
Typical usage looks like

```
   %ac  AC_SEARCH_LIBS([[ble_func]], [[best_lib_evar]], [%USE%])
```

i.e., attempt to compile a small program calling `ble_func()` using
`-lbest_lib_evar`, and, if that succeeds, then whatever `%USE%`
expands to is invoked to indicate that this library is to be selected
(which then breaks out of the search loop).

The other available placeholder is `%FAIL%` which is expected to
appear in an lvalue position (left side of an `=`) getting assigned an
error message in a failure block.  A list of the `%FAIL%` messages for
all libraries is displayed if none of the libraries can be selected.

(If you invoke both `%USE` and `%FAIL%`, the library will be selected
and all `%FAIL%` messages will be ignored.)

If `%lib` has no `%ac` subdirective, this is equivalent to specifying

```
    %ac %USE%
```

i.e., a search that automatically succeeds.  If you want a dynamic
library search to automatically fail, do something like

```
    %ac %FAIL%[="this .so is Evil; do not use"]
```

For `%build`, the expansion shell code is executed if the build is
selected.  In this case, `%DIR%` is available as a placeholder that
expands to a shell code expression that evaluates to the build directory,
for situations where you need to investigate something about it for
`./configure` purposes).

Quoting within _m4_code_ should be such that placeholders will be
m4-expanded.  E.g., do __not__ do
`%ac  AC_SEARCH_LIBS([[f]], [[lib]], [[%USE%]])`.

#### `%ac_yes` _m4_code_

Allowed in `%require`, this inserts the given _m4_code_, which is
expected to expand to shell code to determine the default library
search list for when there is no `--with-`_ew_name_ argument or the
keyword given is an implicit or explicit `yes`, for situations where
this needs to be determined dynamically depending on whether, e.g.,
other extensions are active or not.

The expansion is expected to either be a shell expression
that expands to a comma-separated list of library keywords
(as would be provided in `--with-`_ew_name_`=`) or a shell
command that assigns to the placeholder `%LIB%`.

Quoting within _m4_code_ should be such that placeholders will be
m4-expanded.  E.g., to protect `uvw,xyz` from expansion
do  `%ac_yes AS_IF([...],[%LIB%[=uvw,xyz]],[%LIB%[=xyz]])`,
not `%ac_yes AS_IF([...],[[%LIB%=uvw,xyz]],[[%LIB%=xyz]])`,

#### `%alt` _keyword_

Specfies an alternate keyword for an `%option` or `%lib`.

#### `%build`

Specfies a static build for a `%lib`.  Allowed subcmds are
`--with-`, `%ac`, `%make`, `%dirvar`, `%path`, and `=`.

#### `%dirvar` _varname_

Required in `%build` to specify the name of the make variable to be
assigned the (absolute) build directory path (whether this comes from
`--with-*path` or `%path`).

#### `%disabled`

Indicates that an extension is disabled by default.
Otherwise it is enabled by default.

#### `%implies` _keyword_

For `%option`, indicates that _keyword_, which may be
a single option or a set thereof is implied by selecting
the keyword of the parent `%option`.

#### `%lib` _lib_keyword_ [_cpp_keyword_]

Declares a library choice for `%require` with  _lib_keyword_ and a
corresponding cppname as determined by _cpp_keyword_, which defaults
to _lib_keyword_ upcased, and the setting of `%cdefine`.

Allowed subcmds are `%ac`, `%alt`, `=`, and `%build`

#### `%lib_list` _lib_keyword_ `=` _lib_keyword_[,_lib_keyword_...]

For `%require`, this declares a library search list _lib_keyword_ that,
if included in a `--with-`_ew_name_`=` list expands as declared, i.e.,
the libraries will be searched for in the order specified if this part
of the search list is reached.

If _lib_keyword_ is `yes`, then this declares the default search list
(in which case `%ac_yes` should not be used).

A `%lib_list` does __not__ get a cppname.

#### `%make` _makefile_insert_

For a `%build`, declare the additional dependencies and recipes to be
included in `Makefile` if this build is selected.

#### `%option_set` _keyword_ `=` _keyword_[,_keyword_...]

For an `%%extension`, this declares a option set _keyword_ that, if
selected, implies all of the right-hand-side _keywords_.

An `%option_set` does __not__ get a cppname.

#### `%option` _keyword_ [_cpp_keyword_]

For an `%%extension`, this declares an option _keyword_ and a
corresponding cppname as determined by _cpp_keyword_, which defaults
to _keyword_ upcased, and the setting of `%cdefine`.

> [!TIP]
>
> Declaring an option keyword for an `--enable-` argument and
> declaring an option cppname for `options.h` are __not__ the same thing.

Allowed subcmds are `%implies`, `%?`, and `%alt`.

#### `%path` _directory_path_

Declares a fixed directory for `%build` or a default directory for the
situation where `--with-*path` has not been supplied by the user.

(Unless you're actually planning to include this build directory as part
of the MOO distribution (please don't; I worked hard on getting rid of
these), this is really only recommended/intended for development in order to
spare you from having to retype `--with-whateverpath=path/to/your/build`
over and over while you're working through the issues.)

#### `%require` _requirement_name_

Declares a requirement for an extension.  Currently, the only
supported notion of "requirement" is that of a required library that
needs to be linked in order for the extension to function.

Allowed subcmds are `--with-`, `%cdefine`, `%ac_yes`, `%lib`,
and `%lib_list`.

Candidate libraries to satisfy the requirement are declared using
`%lib` and, if the extension is enabled, the library search loop
tests them in the order specified by the `--with-`_ew_name_
argument given by the user, which defaults to `yes`, which,
in turn, can be

*  defined and set as a list keyword by `%lib_list`,
*  declared directly with `m4` code using `%ac_yes`
   _(this possibility may get deprecated)_, or, otherwise, is,
   by default
*  the list of all `%lib` declarations in the order they occur.

#### `=` (_VAR_ `=` _value_)

Allowed in `%%extension`, `%lib` or `%build`.
The specified variable setting is included in `Makefile` if the
extension is active, the library is selected, or the build is
selected, respectively.

Either this sets _VAR_ to _value_ or, in the case of variables special
to the extension framework, appends _value_ to whatever may already
have already been added by other active extensions or selected
libaries/builds,  with either a space or a newline separator as
appropriate for that _VAR_.

### MakeVar reference

The following are the "standard" makevars that the extension framework
currently knows about and makes substitutions in:

* `CPPFLAGS`
  which is really only needed for `-I` options needed to
  reference additional library headers or headers from a `%build`
  directory

 > [!WARNING]
 >
 > Putting `-D` in `CPPFLAGS` for an extension is likely a mistake
 > and you should instead consider adding a setting to `config.h`
 > or `options.h`.
 > (At the moment, unit tests and instrumentation builds are the
 > only scenarios I can come up with where `-D` makes more sense
 > and you're not going to be making extensions out of those.)

* `LIBS`
  (prepend) System libraries (`-lfoo`) needed by your extension;
  these will be found and filled in by autoconf snippets
  (what the `%ac` and `%ac_yes` directives are for).

 > [!IMPORTANT]
 >
 > Any libraries or object files we need to `%build` ourselves
 > and statically link go in `XT_LOBJS`, not `LIBS`.

* `XT_CSRCS`
  `.c` source files present/compiled-in only if the extension is
   active (`$(XT_CSRCS:.c=.o)` is automatically included in `OBJS`)

* `XT_HDRS`
  `.h` headers used if the extension is active (admittedly, the `.h`
  groupings may not matter so much since `.d` files are now built
  automatically to take care of dependencies by `-MMD -MP` and lists of
  headers are really only needed for the tarball.)

* `XT_LOBJS`
  (prepend) Libraries or external `.o` files that come from separate
  build directories with their own Makefiles, but need to be included
  in our build process

* `XT_DIRS`
  list of build directories (for targets like `make clean-all` that
  need to descend into them).  Relative paths appearing in `XT_DIRS`
  are always with respect to `$(abs_srcdir)` in order to make VPATH
  builds work.

 > [!IMPORTANT]
 >
 > `XT_DIRS` cannot actually be set directly.  Use `%path` (to use a
 > hardwired directory) or the combination of `--with-*path` and
 > `%dirvar` (to get the directory from an argument).

There are also two additional Makefile substitutions which also cannot
be set directly, start out empty, and get lines appended to them
depending on which extensions are active

* `@XT_MAKEVARS@`
  Makefile variable definitions other than those above -- which
  extension authors should feel free to use to simplify whatever
  they're doing in the other makefile vars and rules -- are added here.

* `@XT_RULES@`
  This collects everything appearing in `%make` directives
  for extensions, including but not limited to
  1. recipes for invoking builds of `XT_LOBJS` items
  2. dependency rules that `-MMD -MP` won't find

The rule for building the server is then


```make
moo: $(OBJS) $(XT_LOBJS)
    $(CC) $(LDFLAGS) $(OBJS) $(XT_LOBJS) $(LIBS) -o $@
```

while individual modules depend on `CPPFLAGS` and `CFLAGS`
in the usual way.


--------------------
Philosophical Issues
--------------------
### When is an Option not an Option?

So, a cppname can either appear in `config.h` or `options.h`.

As far as `./configure` is concerned, there is no distinction between
these files.  A cppname for which an `AC_DEFINE` has been issued
(due to `./configure` deciding it needs a value) will have its
`#undef` line either substituted with a `#define` to put in a value
or commented out to indicate it should stay `#undef`, wherever it appears;
whether that's `config.h.in` or `options.h.in` doesn't matter.
Both files are declared with `AC_CONFIG_HEADERS` in `configure.ac`;
`autoconf` would allow us to have ten more such files if we wanted.

There are multiple ways to draw a distinction and they won't always
completely agree:

* In the old days we had settings that `./configure` did automatically
  in `config.h` and settings the user did by hand in `options.h`.

  And even if we want to let ourselves be confused by `./configure`
  doing everything now, there's still a distinction between
  + settings that were never intended to be anything other than
    automatic vs.
  + settings that we _want_ the user to be able to do,

  and it's not that huge a difference whether we're making someone manually
  edit `options.h` vs. making them type `--enable-def-OPTIONNAME`,
  even if some of the settings in `options.h` really __should not__
  be touched.

* `autoconf` draws a distinction between
  + `--enable-FEATURE` arguments that are there to "allow users to
    choose which optional features to build and install" vs.
  + `--with-PACKAGE` arguments "to specify which external software
    packages to use" (i.e., of some set of packages that presumably
    serve the same purpose; the examples they give are using a
    different linker or using a different window system).

  These somewhat line up with the previous distinction if you think
  about the ideal world where a user __really__ should not care which
  packages are being used, the question of which is best being an
  implementation detail of the sort that the super-smart version of
  `./configure` that we don't have yet __would__ be able to figure out
  on its own.

  So you _could_ then take the stance that `--with-` arguments are
  "implementation choices we're helping `./configure` with because
  it's not smart enough yet" vs. `--enable-` being feature choices we
  actually care about.

  ... from which you then get this vague sense that "`--with-` things
  go in `config.h`" while "`--enable-` things go in `options.h`,

  which is a possible way to organize things, even if, historically,
  that's __not__ how this was done and there are any number of matters
  that are decided in `options.h` because someone put them there at
  some point, because once-upon-a-time it was the only place to put
  such things, and it would now be painful to move them.

The __actual__ distinction goes something like this:

There is a set of high-level choices one needs to make up front.
E.g., are we doing Unicode or not?

If we are, then `./configure` has work to do; in this case, there's
an extra source module that needs to be compiled in, we need to
find a Unicode character data (`uclib`) library, and so on.

Later, there are some low-level choices that can be made, e.g.,
what are we doing about identifiers in verbcode?  are we allowing
non-ASCII there?  This low level choice is just an `#ifdef`;
the  code is all there for either version of the world.

Therefore we can allow `UNICODE_IDENTIFIERS` to be an option,
i.e., a genuine `options.h` option, where the user can use
`--enable-def-UNICODE_IDENTIFIERS` to set it or not,
and it'll work either way.

On the other hand, we have `UNICODE_STRINGS`, which answers
the question of whether we have arbitrary UTF-8 in our string
constants.

Which is a __pervasive__ thing.

That is, once we decide we are __not__ going to have Unicode, no
amount of `#ifdef`-ing is going to make the code and libraries
that `./configure` __should have__ installed magically appear.
Thus, allowing the user to change `UNICODE_STRINGS` after the fact
is going to fail horribly and shouldn't be allowed.

Therefore, `UNICODE_STRINGS` belongs in `config.h`.
It is __not an option__, it's a setting that `./configure` does
automatically, even if it happens to be based on our high-level
feature choice, we don't get a further choice to change it back.

... even though it __is__ an `%option` keyword according to
`extensions.ac`, being part of the original high-level choice
whether to do Unicode.

#### Shorter version

The extensions framework creates a number of cppnames.
All of the ones at the `%require` level and below have to go
in `config.h`.

The rest are created by `%option` declarations, in which case you (the
extension developer) have a choice whether they can/should be set
separately later by `--enable-def`.

If you decide __not__ to allow this for a particular cppname,
you put it in `config.h.in` and you're done.

If you're okay with people messing with it independently,
__then__ you
+ put it in `options.h.in`,
+ add a `MOO_DECLARE_OPTIONS` line, in which case
  + the type __has__ to be `[bool]` or `[int]` (for bitmasks) and
  + the default value __has__ to be `no`
  What's going on here is the extension framework gets first
  crack at setting it, so that's where the __real__ default
  value lives.
