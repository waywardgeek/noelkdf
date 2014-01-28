def NoelKDF_SimpleHashPassword(hashsize, password, salt, memsize):
    hash = H(hashsize, salt, password)
    return NoelKDF(hash, memsize, 0, 0, 4096, 1, 1)

def NoelKDF_HashPassword(hashsize, password, salt, memsize, garlic, data, blocksize,
        parallelism, repetitions):
    derivedSalt = H(hashsize, salt, data)
    hash = H(hashsize, derivedSalt, password)
    return NoelKDF(hash, memsize, 0, garlic, blocksize, parallelism, repetitions)

def NoelKDF_UpdatePasswordHash(hash, password, salt, memsize, oldGarlic, newGarlic,
        blocksize, parallelism, repetitions):
    return NoelKDF(hash, memsize, oldGarlic, newGarlic, blocksize, parallelism, repetitions)

def NoelKDF(hash, memsize, startGarlic, stopGarlic, blocksize, parallelism, repetitions):
    wordhash = toUint32Array(hash)
    memlen = memsize/4
    blocklen = blocksize/4
    numblocks = memlen/(2*parallelism*blocklen)
    mem = array of 2*numblocks*blocklen*parallelism*2^garlic 32-bit integers
    for i = 0 .. garlic:
        for j = 1 .. repetitions:
            value = 0
            for p = 0 .. parallelism-1:
                hashWithoutPassword(p, wordHash, mem, blocklen, numblocks)
                value += mem[(p+1)*numblocks*blocklen-1]
            for p = 0 .. parallelism-1:
                hashWithPassword(p, mem, blocklen, numblocks, parallelism,
                         value)
            xorIntoHash(wordHash, mem, blocklen, numblocks, parallelism)
        numblocks *= 2
    return toUint8Array(wordHash)

def hashWithoutPassword(p, wordHash, mem, blocklen, numblocks):
    start = 2*p*numblocks*blocklen
    mem[start .. start+blocklen-1] = toUint32Array(H(blocklen*4, p, toUint8Array(hash)))
    value = 1
    mask = 1
    prevAddr = start
    toAddr = start + blocklen
    for i = 1 .. numblocks:
        if mask << 1 <= i:
            mask = mask << 1
        reversePos = bitReverse(i, mask)
        if reversePos + mask < i:
            reversePos += mask
        fromAddr = start + blocklen*reversePos
        for j = 0 .. blocklen-1:
            value  = value*(mem[prevAddr++] | 3) + mem[fromAddr++]
            mem[toAddr++] = value

def bitReverse(value, mask):
    result = 0
    while mask != 1:
        result = (result << 1) | (value & 1)
        value >>= 1
        mask >>= 1
    return result

def hashWithPassword(p, hash, mem, blocklen, numblocks, parallelism, value):
    startBlock = 2*p*numblocks + numblocks
    start = startBlock*blocklen
    prevAddr = start - blocklen
    toAddr = start
    for i = 0 .. numblocks-1:
        v = (uint64)value
        v2 = v*v >> 32
        v3 = v*v2 >> 32
        distance = (i + numblocks – 1)*v3 >> 32
        if distance <= i:
            fromAddr = start + (i   – distance)*blocklen
        else:
            q = (p + i) % parallelism
            b = numblocks – (distance - i)
            fromAddr =  (2*numblocks*q + b)*blocklen
        for j = 0 .. blocklen:
            value  = value*(mem[prevAddr++] | 3) + mem[fromAddr++]
            mem[toAddr++] = value

def xorIntoHash(wordHash, mem, blocklen, numblocks, parallelism):
    for p = 0 .. parallelism-1:
        pos = 2*(p+1)*numblocks*blocklength - length(wordHash)
        for i = 0 .. length(wordHash):
            wordHash[pos + i] ^= value
