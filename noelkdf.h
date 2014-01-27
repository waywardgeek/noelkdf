// This is the prototype required for the password hashing competition.
// t_cost is an integer multiplier on CPU work.  m_cost is an integer number of MB of memory to hash.
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen,
    unsigned int t_cost, unsigned int m_cost);

// This version allows for some more options than PHS.  They are:
//  data            - arbitrary "local parameters" that get hashed with the password.
//                    include second authentication factors such as key file hashes or
//                    password dependent secrets decrypted on the server with it's master
//                    key.
//  parallelism     - the number of threads to run in parallel
//  block_size      - length of memory blocks hashed at a time
//  num_hash_rounds - number of SHA256 rounds to compute the intermediate key
//  parallelism     - number of inner loops allowed to run in parallel - should match
//                    user's machine for best protection
//  clear_in        - when true, overwrite the in buffer with 0's early on
//  return_memory   - when true, the hash data stored in memory is returned without being
//                    freed in the memPtr variable
int NoelKDF(void *out, size_t outlen, void *in, size_t inlen, const void *salt, size_t saltlen,
        void *data, size_t datalen, unsigned int t_cost, unsigned int m_cost, unsigned int
        num_hash_rounds, unsigned int repeat_count, unsigned int block_size, int clear_in,
        int return_memory, unsigned int **mem_ptr);
