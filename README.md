
### Cauchy Prime Field Erasure Coding

Quick start:

Create file ```input.dat``` with ```256KiB``` of random data:

```
dd if=/dev/urandom of=input.dat bs=512 count=512
```

Encode file ```input.dat``` to a hundred chunk files, each of ```5380``` bytes in size:

```
./encode input.dat 5380 chunk{00..99}.cpf
```

Output should be ```CPF(100, 49)```, which means we only need any 49 chunks of the 100 encoded.

Randomly delete (erase) 51 chunk files to only keep 49:

```
ls chunk*.cpf | sort -R | head -n 51 | xargs rm
```

Decode ```output.dat``` from remaining chunk files:

```
./decode output.dat chunk*.cpf
```

Compare original ```input.dat``` with ```output.dat```:

```
diff -q -s input.dat output.dat
```

