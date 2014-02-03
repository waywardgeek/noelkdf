// I copied this file from Catena's src/catena_test_vectors.c and modified it to call
// NoelKDF.  It was written by the Catena team and slightly changed by me.
// It therefore falls under Catena's MIT license.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "noelkdf.h"

void print_hex(char *message, uint8_t *x, int len) {
    int i;
    puts(message);
    for(i=0; i< len; i++) {
	if((i!=0) && (i%8 == 0)) {
            puts("");
        }
	printf("%02x ",x[i]);
    }
    printf("     %d (octets)\n\n", len);
}

/*******************************************************************/

void test_output(uint8_t hashlen,
                 uint8_t *pwd,   uint32_t pwdlen,
		 uint8_t *salt,  uint8_t saltlen,
		 uint8_t *data,  uint32_t datalen,
		 uint32_t memlen, uint8_t garlic,
                 uint32_t blocklen, uint32 parallelism,
                 uint32_t repetitions)
{
  uint8_t hash[hashlen];

  print_hex("Password: ",pwd, pwdlen);
  print_hex("Salt: ",salt, saltlen);
  print_hex("Associated data:", data, datalen);
  printf("Memlen:  %u\n", memlen);
  printf("Garlic:  %u\n", garlic);
  printf("Blocklen:  %u\n", blocklen);
  printf("Parallelism:  %u\n", parallelism);
  printf("Repetitions:  %u\n", repetitions);

  if(!NoelKDF_HashPassword(hash, hashlen, pwd, pwdlen,
        salt, saltlen, memlen, garlic, data, datalen,
        blocklen, parallelism, repetitions, false)) {
      fprintf(stderr, "Password hashing failed!\n");
      exit(1);
  }

  print_hex("\nOutput: ", hash, hashlen);
  puts("\n\n");
}


/*******************************************************************/

void simpletest(char *password, char *salt, char *data, uint32_t memlen)
{
  test_output(64, (uint8_t *)password, strlen(password), (uint8_t *)salt, strlen(salt),
    (uint8_t *)data, strlen(data), memlen, 0, 4096, 1, 1);
}

/*******************************************************************/

void PHC_test()
{
  int i;
  uint8_t j = 0;

  printf("****************************************** Test passwords\n");
  for(i=0; i < 256; i++) {
    test_output(32, (uint8_t *) &i, 1, &j, 1, NULL, 0, 10, 0, 4096, 1, 1);
  }
  printf("****************************************** Test salt\n");
  for(i=0; i < 256; i++) {
    test_output(32, &j, 1, (uint8_t *) &i, 1, NULL, 0, 10, 0, 4096, 1, 1);
  }
  printf("****************************************** Test data\n");
  for(i=0; i < 256; i++) {
    test_output(32, &j, 1, &j, 1, (uint8_t *) &i, 1, 10, 0, 4096, 1, 1);
  }
  printf("****************************************** Test garlic\n");
  for(i=0; i < 6; i++) {
    test_output(32, &j, 1, &j, 1, NULL, 0, 1, i, 4096, 1, 1);
  }
  printf("****************************************** Test parallelism\n");
  for(i=1; i < 10; i++) {
    test_output(32, &j, 1, &j, 1, NULL, 0, 1, 0, 4096, i, 1);
  }
  printf("****************************************** Test repetitions\n");
  for(i=1; i < 10; i++) {
    test_output(32, &j, 1, &j, 1, NULL, 0, 1, 0, 4096, 1, i);
  }
  printf("****************************************** Test blocklen\n");
  for(i=4; i < 256; i += 4) {
    test_output(32, &j, 1, &j, 1, NULL, 0, 1, 0, i, 1, 1);
  }
  printf("****************************************** Test memlen\n");
  for(i=1; i < 16; i += 4) {
    test_output(32, &j, 1, &j, 1, NULL, 0, i, 0, 4096, 1, 1);
  }
  printf("****************************************** Test collisions\n");
  for(i=0; i < 10000000; i++) {
    test_output(4, (uint8_t *) &i, 4, &j, 1, NULL, 0, 1, 0, 4096, 1, 1);
  }
}

/*******************************************************************/

int main()
{
  printf("****************************************** Basic tests\n");

  simpletest("password", "salt", "", 1);
  simpletest("password", "salt", "", 1024);
  simpletest("password", "salt", "data", 1024);
  simpletest("passwordPASSWORDpassword",
	     "saltSALTsaltSALTsaltSALTsaltSALTsalt","", 1);


  PHC_test();

  printf("****************************************** Misc tests\n");
  test_output(128, (uint8_t *)"password", strlen("password"), (uint8_t *)"salt",
    strlen("salt"), NULL, 0, 1024, 0, 4096, 2, 1);
  test_output(128, (uint8_t *)"password", strlen("password"), (uint8_t *)"salt",
    strlen("salt"), NULL, 0, 10, 0, 64, 1, 64);
  test_output(64, (uint8_t *)"password", strlen("password"), (uint8_t *)"salt",
    strlen("salt"), NULL, 0, 10, 4, 128, 1, 1);

  return 0;
}
