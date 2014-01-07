NOELKDF
=======

This is my attempt at an efficient memory-hard key stretching algorithm based on scrypt.
Various members of the community at password-hashing.net have contributed so generously to
NoelKDF, that it is hard to claim as my own.

Features
--------

- Memory hard Key Derivation Function for password protection
- Simplicity -- the KISS rule at it's best
- Ultra-fast hashing, approaching the speed of memmove
- Avoids cache timing attacks
- User selectable parallelism, through threads and SIMD
- "client independent update" to make server hashes more secure
- Server relief support - hashing can be split between client and server
- Friendly to crypto-systems that need deniability, such as TrueCrypt.
- User-selectable initial rounds of hashing

Credit for Features "Borrowed"
------------------------------

Alexander, aka Solar Designer, is he brains behind escript.  He has been over-the-top
helpful.  Three ideas he fed me include the  parallelism parameter to limit parallelism on
purpose, the power-of-two sliding window, and random segment hashing within a "page".  He
also helped me understand modern CPU memory performance and SIMD capabilities, which has
an enormous impact.

Christian Forler, the inventor of Catena, showed me how to avoiding timing attacks through
fixed addressing access pattern, and how to believe it works through pebble proofs.  Also,
doubling memory and not just time with t_cost to provide "client independent update".
Catena helped make the Server Relief idea mainstream.

Scrypt, the basis for the whole thing.  Scrypt is awesome, and there are many ideas
carried over.

License
-------

I, Bill Cox, place this code into the public domain.  There are no hidden back-doors or
intentional weaknesses, and I blieve it violates no patents.  I will file no patents on
any material in this project.  I have included sha.c and sha.h which I copied from the
scrypt project, and which is available through the BSD license.

TODO
----

Dynamically adapt number of threads to maximize speed.
