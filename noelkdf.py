from pbkdf2 import PBKDF2
from Crypto.Hash import SHA256
import os

def toHex(s):
    return ":".join("{0:x}".format(ord(c)) for c in s)

def toUint32Array(bytes):
    b = bytearray(bytes)
    words = []
    for i in range(len(b)/4):
        value = b[i]
        value = (value << 8) | b[i+1]
        value = (value << 8) | b[i+2]
        value = (value << 8) | b[i+3]
        words.append(value)
    return words

def toUint8(word):
    b3 = word & 0xff
    word >>= 8
    b2 = word & 0xff
    word >>= 8
    b1 = word & 0xff
    word >>= 8
    b0 = word & 0xff
    b = []
    b.append(b0)
    b.append(b1)
    b.append(b2)
    b.append(b3)
    return bytearray(b)

def toUint8Array(words):
    b = bytearray()
    for i in range(len(words)):
        word = words[i]
        b3 = word & 0xff
        word >>= 8
        b2 = word & 0xff
        word >>= 8
        b1 = word & 0xff
        word >>= 8
        b0 = word & 0xff
        b.append(b0)
        b.append(b1)
        b.append(b2)
        b.append(b3)
    return b

def H(password, salt, size):
    return PBKDF2(password, salt).read(size)

def NoelKDF_SimpleHashPassword(password, salt, hashsize, memsize):
    hash = H(password, salt, hashsize)
    return NoelKDF(hash, memsize, 0, 0, 4096, 1, 1)

def NoelKDF_HashPassword(password, salt, hashsize, memsize, garlic, data, blocksize,
        parallelism, repetitions):
    derivedSalt = H(data, salt, hashsize)
    hash = H(password, derivedSalt, hashsize)
    return NoelKDF(hash, memsize, 0, garlic, blocksize, parallelism, repetitions)

def NoelKDF_UpdatePasswordHash(hash, memsize, oldGarlic, newGarlic, blocksize, parallelism, repetitions):
    return NoelKDF(hash, memsize, oldGarlic, newGarlic, blocksize, parallelism, repetitions)

def NoelKDF(hash, memsize, startGarlic, stopGarlic, blocksize, parallelism, repetitions):
    wordHash = toUint32Array(hash)
    memlen = memsize/4
    blocklen = blocksize/4
    numblocks = memlen/(2*parallelism*blocklen)
    memlen = 2*parallelism*numblocks*blocklen
    mem = [0 for _ in range(memlen)]
    for i in range(startGarlic, stopGarlic+1):
        for j in range(repetitions):
            value = 0
            for p in range(parallelism):
                hashWithoutPassword(p, wordHash, mem, blocklen, numblocks)
                value += (mem[2*p*numblocks*blocklen + blocklen-1]) & 0xffffffff
            for p in range(parallelism):
                hashWithPassword(p, mem, blocklen, numblocks, parallelism, value)
            xorIntoHash(wordHash, mem, blocklen, numblocks, parallelism)
        numblocks *= 2
    return toUint8Array(wordHash)

def hashWithoutPassword(p, wordHash, mem, blocklen, numblocks):
    start = 2*p*numblocks*blocklen
    mem[start : start+blocklen] = toUint32Array(H(str(toUint8Array(wordHash)), str(toUint8(p)), blocklen*4))
    value = 1
    mask = 1
    prevAddr = start
    toAddr = start + blocklen
    for i in range(1, numblocks):
        if mask << 1 <= i:
            mask = mask << 1
        reversePos = bitReverse(i, mask)
        if reversePos + mask < i:
            reversePos += mask
        fromAddr = start + blocklen*reversePos
        for j in range(blocklen):
            value = (value*(mem[prevAddr] | 3) + mem[fromAddr]) & 0xffffffff
            mem[toAddr] = value
            prevAddr += 1
            fromAddr += 1
            toAddr += 1

def bitReverse(value, mask):
    result = 0
    while mask != 1:
        result = (result << 1) | (value & 1)
        value >>= 1
        mask >>= 1
    return result

def hashWithPassword(p, mem, blocklen, numblocks, parallelism, value):
    startBlock = 2*p*numblocks + numblocks
    start = startBlock*blocklen
    prevAddr = start - blocklen
    toAddr = start
    for i in range(numblocks):
        v = value
        v2 = v*v >> 32
        v3 = v*v2 >> 32
        distance = (i + numblocks - 1)*v3 >> 32
        if distance < i:
            fromAddr = start + (i - 1 - distance)*blocklen
        else:
            q = (p + i) % parallelism
            b = numblocks - 1 - (distance - i)
            fromAddr = (2*numblocks*q + b)*blocklen
        for j in range(blocklen):
            value  = (value*(mem[prevAddr] | 3) + mem[fromAddr]) & 0xffffffff
            mem[toAddr] = value
            prevAddr += 1
            fromAddr += 1
            toAddr += 1

def xorIntoHash(wordHash, mem, blocklen, numblocks, parallelism):
    for p in range(parallelism):
        pos = 2*(p+1)*numblocks*blocklen - len(wordHash)
        for i in range(len(wordHash)):
            wordHash[i] ^= mem[pos+i]

#import pdb; pdb.set_trace()
salt = os.urandom(16)
hash = NoelKDF_SimpleHashPassword("password", salt, 32, 1 << 20)
print toHex(str(hash))