Coding Style Guidelines
=======================
Contents
--------
* [Preface](#user-content-preface)
* [Topics](#user-content-topics)
  + [File Structure, Headers and `#include`](#user-content-file-structure-headers-and-include)
  + [Whitespace, Indentation, and Tabs](#user-content-whitespace-indentation-and-tabs)
  + [Comments](#user-content-comments)
  + [On `NULL` and Comparisons](#user-content-on-null-and-comparisons)
  + [Statement Structure](#user-content-statement-structure)
  + [Declarations](#user-content-declarations)

Preface
-------
It's interesting to me that no one in the past 30+ years has produced
style guidelines for the LambdaMOO server source.  Part of it, I'm
sure was time constraints.  Why bother to document when you could be
writing actual code and fixing stuff?

But, as it happens, I do (surprise) care rather a lot about code
quality and maintainability, so there **is** a coding style here, of
sorts, and, on general principles, it will most likely save time for
everyone if you can follow it when submitting your pull requests.

Yes, some of these guidelines are completely arbitrary and stupid.
Or they are issues where there are no objective correct answers and
we just had to Pick Something to go with.

Yes, there are aspects of the prevailing style that I completely
disagree with, that I would have done differently if I had written
this all myself from scratch, but changing it now would be too
pervasive and mess up the history, and consistency matters more than
anything else.

(E.g., I hate tabs.  But I've already done one Whitespace Festival
and that was painful enough…  Which is not to say other query-replace
festivals won't ever be happening; but they do tend to be lower-priority
affairs and I couldn't begin to guess what the schedule will be…)

Yes, there are historical exceptions.  Previous maintainers had
different opinions from me on certain matters; who knew?  My goal is
to eradicate all evidence they ever existed …  Well okay, no, I'm not
doing that; oddly enough, I do **not** actually want to spend **all**
of my time reformatting code that already works.  But, suffice it to
say, it's a fair bet that things that annoy me sufficiently **will**
eventually get cleaned up as the opportunities arise.

Note that when I say "will most likely save time", there's an emphasis
on *likely*…

… meaning nothing is guaranteed.

… meaning do not take any of what follows as a promise that your pull
request won't be completely rewritten anyway, even if you thought you
were assiduously following all of the "rules" (or maybe you
**actually** were successful at following the "rules").  I do this
sometimes; try not to take it personally.  I try not to do it without
reason but sometimes the reasons are nebulous.  Or maybe there are new
rules that I have only recently discovered myself and haven't gotten
around to mentioning here.

Above all, it does **not** mean I don't appreciate your contribution.
In fact, in many cases it's **easier** to modify an existing solution
to a problem than to try to create a new one from scratch; meaning you
probably *did* save me time, even if might not look that way to you.

Welcome to the world of open-source software.

Finally, in case it wasn't already obvious, everything here is
strictly my own opinion, not to be attributed to or blamed on anyone
else.

        Roger Crew (a.k.a. wrog)

Topics
------
### File Structure, Headers and `#include`

`experiments.c` and `experiments.h` are where I've spelled out some
more recently introduced style rules and include (most of) my
expectations for headers and file layout (which I did spend some time
trying to normalize) and, in particular, how `#include` lines should
be grouped.

Be conservative about what you `#include` in a `.h` (beyond `config.h`
and `options.h` which are always welcome).

Be liberal about what you include in a `.c` (I may, at some point go
through and just make sure *every* `.c` includes `config.h` and
`options.h`, but right now they do all explicitly include at least
`config.h`)

If a substitute system header ("`my-foo.h`") exists, **use** it.
E.g.,, do not do `#include <stdio.h>` because we have `"my-stdio.h"`.

Every global function we define lives *somewhere* i.e., some `.o` file
that will have a `.h` file describing its interface and therefore
there should be an `extern` declaration there.  There are some
historical exceptions in which interfaces have been combined (`db.h`)
or where `.c` files `#include` other `.c` files (making the latter not
True Modules in their own right), notably with the network code

The big exception is registration functions `void register_foo(void)`,
which go at the end of their respective files and are treated **AS
IF** they were defined in `bf-register.c` even though they are not
(see `bf-register.h` for more on how registration functions work).

Documentation describing global functions should be in their
respective `.h` files.  It is not necessary to repeat it in the `.c`
file (meaning if you come across a function in a `.c` that seems
overly mysterious, check out the `.h` file before assuming we screwed
up and neglected to document it, even if there **are** rather a lot of
functions you **are** expected to figure out strictly from the C
source)

### Whitespace, Indentation, and Tabs

* **No** trailing whitespace, ever.

* Use 4 spaces as the basic indentation and increment thereof.

* Tab stops go at 8-space intervals.

* In C sources *only* (`.c`, `.h`, `.y`, `.gperf`), we use tabs for
  indentation everywhere (let me emphasize how much I myself **hate**
  tabs, but we are constrained by history here).

  But for other files, as a general rule,

  + use tabs **only** in places where they are explicitly required
    (`Makefile` rules), and

  + **not at all** in M4 code (`.ac`, `.m4`)
    or new documentation (`.md`) files.

  …unless there's some really, **REALLY** good reason
  (`extensions2_tutorial.ac` is probably the poster-child for me
  violating my own rules; I discovered that having commented-out lines
  automatically line up with un-commented-out lines makes the tutorial
  way easier to follow (or so I think) -- but this is, like, the
  3-sigma exception).

* For function calls and function definitions/declarations,
  do **not** put whitespace between the function name and the
  open paren for the parameter list.

  Conversely, for control constructs, **do** put a space after the
  control keyword and keep the open brace on the same line (e.g.,
  ```
  if (…) {
      …
  }
  while (…) {
      …
  }
  ```
  But do not use braces if the body is only a single statement
  (something I have historically disagreed with, but I think I may be
  changing my mind; I'm finding that I **more** like removing
  unnecessary vertical space.))

* Function definitions should look like this
  ```
  [storage_class] return_type
  name(parameter1 decl, parameter2 decl, …)
  {
      …
  }
  ```
  That is, storage class and return type go on the *preceding line*,
  the function name itself is at the very start of the line, no space
  before the parameter list, and the open brace that starts the body
  should be on the very next line by itself.  Also remember that C99
  now forbids non-prototype functions and thus requires `(void)` for
  empty parameter lists.

  … **Exception**:  Inline function declarations **can** be squished
  together; and if it's really short and you can fit it on a single
  line, or there's some pattern that's made clearer by cramming a
  number of tiny functions into a small space, go for it
  (see `fl_unbox` and `box_fl` in `structures.h`).

### Comments

* We try to avoid comment-bloat.  Aside from certain bit-twiddling
  tricks based on weird not-commonly-known properties of 2-complement
  arithmetic or number theory, you generally should not need to
  explain **what** the code itself is doing; we already know how to
  read C source.  Usually, when you need to explain something, what
  you need to explain is **why** you are doing it this way (i.e., if
  there is some more obvious way to do something that you seem to be
  avoiding for some reason, it may be good to spell that out to
  de-mystify your code).

* Understand the difference between a source comment and a commit comment.

  + a _commit comment_ describes a change.  If it's sufficiently
    obvious, you may be able to fit it into the title.  But sometimes
    there is history to explain, or otherwise additional material to
    make clear why the change was necessary (or to make clear that it
    wasn't, that something was done for purely stylistic reasons).
    That goes in the body of the commit.

    It should generally be assumed that everyone has (or can get to) a
    copy of the repository, do a `git blame` to see the list of
    commits affecting a particular range of lines, and, if one feels
    the need, read off the entire history of how this block got the
    way it is.

  + a _source comment_, something appearing directly in the source
    describes the *current* code as it is, the role it plays in the
    *current* codebase.  Your comment should be informative to someone
    who does **not** know the history.  In most cases they should not
    **need** to know the history.  Detailed references to history thus
    tend to be useless bloat.

    Which is not to say there can't be cases where the history
    matters, e.g., knowing about alternatives that were tried and
    failed for various reasons can be useful.  But that will all be in
    the log; the most you should ever need to do in the source would
    be a **very brief** summary; and it is completely okay to refer
    people to the log for details if they care (except this will
    usually already be implicit and not need to be said).

  Avoiding source-bloat is more important than avoiding repository-bloat.

* For long block-style comments, we prefer this (well okay, we also
  prefer English):
  ```
  /*
   *  Minime, hanc linguam numquam re vera didici.
   *  Hoc tantum facio ut mihi cool videar.
   *  Ceterum autem censeo Carthaginem esse delendam.
   */
  ```
  the extra `*` at the beginning of each line may seem tedious but
  it's very nice to be able to grep files and know immediately which
  hits are coming out of comments.  Yes, some of us still use the
  Ancient Tools, sorry.

* Do not use C++-style comments `// text…`

### On `NULL` and Comparisons

* It seems at least one previous maintainer preferred using `0` for
  null pointers; I had thought this practice had died out by 1990 but
  apparently not.  Many people later on have been happily using
  `NULL`.  So, suffice it to say, usage of `NULL` here has been
  inconsistent.

  I myself like `NULL`; I'd just as soon keep `NULL` instances and
  replace `0` wherever it's being used this way.  Maybe someday I will
  do a `NULL` Festival, but as of this writing, that hasn't happened
  yet.

  I also think it's natural to use pointer expressions in C boolean
  contexts, i.e., to read null pointers as false, so feel free to
  leave out `!= NULL` or use `!p` for `p == NULL`.

  (**Exception**:  If we're needing to capture a pointer value in the
  middle of a test, then you **do** want to do, e.g.,
  `if (NULL == (f = fopen(…)))`
  to specifically call out the assignment as not being a typo for `==`).

* Boolean functions are currently declared as returning `int` and are
  as per the usual C convention expected to return 0 for false and
  something nonzero for true.

  (The use of `int` here is historical.  Yes, I am aware that C99
  makes a `_Bool` type available.  There may yet come a day where
  I get sufficiently bored to hold the `_Bool` Festival and change
  everything, but as of this writing we're not there yet.)

  For UNIX system calls and other Stupid API situations where, for
  whatever reason, the sense of boolean function results is reversed
  from what a normal C reading would be, where a function name evokes
  a clear expectation of what success means (e.g., `fsync(fd)` = "file
  fd successfully synchronized with disk") and yet success returns
  zero while failure returns non-zero, then please **do** use `0 == `
  or `0 !=` out in front to call attention to this, e.g.,
  ```
  if (0 != fsync(fd)) {
       errlog("sync failed, oh no, we are doomed");
       break/return/goto failed/whatever;
  }
  ```
  The extra `0 == ` or `0 !=` is also useful for embedded assignments
  in `if`/`while`, just to make obvious that assignment is what was
  intended, say, in `if( 0 != (a = fncall())`, that the second `=` is
  not a typo for `==`

* It also seems a previous maintainer actually _liked_ doing
  `!strcmp(a,b)` to test equality, which, as far as I am concerned,
  is **chaotic evil**.  String comparisons should look like comparisons
  and be arranged so that the operator matches, i.e.,
  ```
  strcmp(a,b)  < 0   /* a  < b */
  strcmp(a,b) <= 0   /* a <= b */
  strcmp(a,b)  > 0   /* a  > b */
  strcmp(a,b) >= 0   /* a >= b */
  strcmp(a,b) == 0   /* a == b */
  strcmp(a,b) != 0   /* a != b */
  ```
  Except that for equality/inequality
  ```
  0 == strcmp(a,b)   /* a == b */
  0 != strcmp(a,b)   /* a != b */
  ```
  are acceptable and may actually be preferred (`==` and `!=` are
  symmetric; we don't have to care about the order here).

  (…insert usual admonition about not using `strcmp()` unless you are
  absolutely certain about there being a terminating 0 byte, which
  **will** be the case for strings created by `str_dup()`,
  `reset_stream()`, C string constants, and anything assigned
  to a Var's `.v.str` slot…)

* Also remember that `NULL` is a pointer and `'\0'` is a zero byte
  (for terminating strings); do not confuse these.

### Statement Structure

* You do not have to show off how much you understand Structured
  Programming™.  Yes, it's often/usually true that finding the
  structured way to express an algorithm improves clarity.
  But there are exceptions.

  E.g., you might have a whole sequence of actions to do, each of
  which might fail.  The extended `if-then-elsif-…` is the usual
  paradigm for this but sometimes there are unpleasant choices between
  putting complicated actions inside of tests vs. ridiculously deep
  nesting.  Sometimes it's just better to write the straight-ahead
  sequence pretending everything will just work and then have the
  errors all be escapes/jumps.  C lets you do that.

* I really, **really** hate cut&paste programming; large blocks of
  code repeated verbatim is nearly always a sign that there's a better
  way to do it.  (and yes, there still are instances of this; I've
  been cleaning them out as opportunities arise).  Define new
  functions or rearrange control flow.  You can roll `for(;;)` loops
  so that they exit in the middle.  Also, C has `goto`.  Alsoalso: In
  C it can be completely legal to jump **into** a loop or between the
  branches of an if-then-else or a switch.  Not something to do
  lightly, but every so often it's the Right Thing for readability.

  Same story for having to repeat the same information in different
  places, which is then asking for trouble when it comes time to
  change something and you forget to hit **all** of the places.
  The C preprocessor can be your friend, though, to be sure, inline
  functions are generally better if you can use them (but sometimes
  you can't).

  (Yes, this is the DRY [Don't Repeat Yourself] principle; I predate
  that, as it happens.)

* Statement keywords should typically be the first thing on the line.
  I **much** prefer
  ```
  if (cond) {
      …
  }
  else
  ```
  to
  ```
  if (cond) {
      …
  } else
  ```
  (this also has to do with why I'm less bothered by removing the
  braces around single statements, i.e., because it prevents this
  issue from coming up).  Current is usage is not completely
  consistent, though I believe the former is in the majority at the
  moment (and I've been admittedly working to get it that way...)

  The one exception to this is `do { … } while (…);` where having the
  `while` be the first thing on the line can be **horribly** confusing
  (because it looks like it might be its own statement when it's
  **not**).  So this one **should** be
  ```
  do {
      …
  } while (condition…)
  ```

### Declarations

* Now that C99 allows local declarations to not be at the beginning of
  the block, I'd just as soon they appear closer to their uses,
  especially if they're just random temporaries that are only needed
  for a few consecutive statements.

* Sub-blocks are sometimes clarifying but can also waste needed
  vertical space.  I'd much rather that the guts of an algorithm fit
  on a single screen where you can see it all in one place.

  Modern compilers are quite sophisticated about data-flow analysis
  and register allocation; you don't need to worry about declaring too
  many local variables, having them take up too much space on the
  stack, or having them last too long.  Also, this is not C++ where
  you have to worry about when destructors are getting invoked.

* It seems most function pointer type-specs here take the form
  ```
   rtype (*fname) (t1 arg1, t2 arg2,...)
  ```
  i.e., with a space before the argument list open paren, which I've
  never seen elsewhere, but seems to have its uses (it **does** serve
  to make cast expressions, `(type)(value)` with no space, look
  different).  I've decided I find this sufficiently non-evil that I"m
  going to follow it.
