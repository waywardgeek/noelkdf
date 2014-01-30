NOELKDF
=======

NoelKDF is a sequential memory and compute time hard key derivation function that
maximizes an attackers time*memory cost for guessing passwords.  Christian Forler and
Alexander Peslyak (aka SolarDesigner) provided most of the ideas that I have combined in
NoelKDF.  While they may not want credit for this work, it belongs to them more than me.

Please read NoelKDF.odt for a description of the algorithm and credits for ideas.

License
-------

I, Bill Cox, wrote this code in January of 2014, and place this code into the public
domain.  There are no hidden back-doors or intentional weaknesses, and I believe it
violates no patents.  I will file no patents on any material in this project.

NoelKDF includes sha.c and sha.h which I copied from the scrypt source code, and which is
released under the BSD license, and noelkdf-test.c was copied from Catena's
catena_test_vectors.c and is released under the MIT license.

TODO
----

Write automatic parameter picker
