NOELKDF
=======

My attempt at an efficient memory-hard key stretching algorithm based on scrypt

Goals
-----

- Simplicity -- the KISS rule
- Faster hashing, increaseing area*time because we'll be able to fill more RAM
- Protect the key with user-selectable initial rounds of SHA-256 and then clear the
  password
- Operate well in a deniable mode, meaning only random parameters like salt can be stored
- Benefit from 64-bit CPU widths without trashing 32-bit performance

We simultaneously fill memory and hash.  When we're done filling memory, we're done.  This
is more friendly for users who want to pick a time to run, rather than specifying the
memory.

Scrypt relies on Salsa20/8, which is a well known algorithm.  It's fast, but still several
times slower than just filling memory with a counter.  At the same time, there seems to be
little need for such a secure RNG.  It has only three goals, AFAIK:

- It should generate data efficiently on a CPU, so we can make memory bandwidth the
  bottleneck, just like it is for an attacker
- It should not allow an attacker easily to compute V(i), without first computing
  V(0)...V(i-1).  Here V is memory and i is the ith memory location.
- It should have a large state, so the attacker can't just cache RNG states, and must fill
  memory instead.

Faster hashing should make use of 64-bit data paths without killing the performance on
32-bit machines.  It should focus on speed over proven security, but make it simple to
prove property 2.

Since we fill memory as we go, in addition to the salt, we could store a stop parameter,
of say from 128 to 256 bits.  When we see the stop parameter matches a thread key, we
would stop hashing.  This has not been implemented, but it is an intended mode of the
algorithm.  This could be particularly usefule for TrueCrypt or any other tool which
supports deniability.

License
-------

I, Bill Cox, place this code into the public domain.  There are no hidden back-doors or
intentional weaknesses, and I blieve it violates no patents.  I will file no patents on
any material in this project.  I have included sha.c and sha.h which I copied from the
scrypt project, and which is available through the BSD license.

TODO
----

Use the parallelism parameter for both max threads and SIMD.  Dynamically adapt number of
threads to maximize speed.
