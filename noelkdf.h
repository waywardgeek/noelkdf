// This is the prototype required for the password hashing competition.
// t_cost is an integer multiplier on CPU work.  m_cost is an integer number of MB of memory to hash.
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen,
    unsigned int t_cost, unsigned int m_cost);

// This version allows for some more options than PHS.  They are:
//  num_threads     - the number of threads to run in parallel
//  page_size       - length of memory blocks hashed at a time, in KB
//  num_hash_rounds - number of SHA256 rounds to compute the intermediate key
//  parallelism     - number of inner loops allowed to run in parallel - should match
//                    user's machine for best protection
//  clear_in        - when true, overwrite the in buffer with 0's early on
int NoelKDF(void *out, size_t outlen, void *in, size_t inlen, const void *salt, size_t saltlen,
        unsigned int t_cost, unsigned int m_cost, unsigned int num_hash_rounds, unsigned int parallelism,
        unsigned int num_threads, unsigned int page_length, int clear_in);
