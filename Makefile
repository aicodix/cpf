
CXXFLAGS = -std=c++17 -W -Wall -Ofast -fno-exceptions -fno-rtti -I../code
CXX = clang++ -stdlib=libc++ -march=native
#CXX = g++ -march=native

#CXX = armv7a-hardfloat-linux-gnueabi-g++ -static -mfpu=neon -march=armv7-a
#QEMU = qemu-arm

#CXX = aarch64-unknown-linux-gnu-g++ -static -march=armv8-a+crc+simd -mtune=cortex-a72
#QEMU = qemu-aarch64

CHUNKS := $(shell seq -f "chunk%02g.cpf" 0 99)
ERASED := $(shell seq -f "chunk%02g.cpf" 0 99 | sort -R | head -n 49)

.PHONY: all

all: encode decode

test: encode decode
	dd if=/dev/urandom of=input.dat bs=512 count=512
	$(QEMU) ./encode input.dat 5380 $(CHUNKS)
	$(QEMU) ./decode output.dat $(ERASED)
	diff -q -s input.dat output.dat
	rm input.dat output.dat $(CHUNKS)

encode: encode.cc
	$(CXX) $(CXXFLAGS) $< -o $@

decode: decode.cc
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -f encode decode

