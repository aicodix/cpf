/*
Cauchy Prime Field Erasure Coding

Copyright 2024 Ahmet Inan <inan@aicodix.de>
*/

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include "crc.hh"
#include "prime_field.hh"
#include "cauchy_prime_field_erasure_coding.hh"

int main(int argc, char **argv)
{
	if (argc < 4) {
		std::cerr << "usage: " << argv[0] << " INPUT SIZE CHUNKS.." << std::endl;
		return 1;
	}
	const char *input_name = argv[1];
	struct stat sb;
	if (stat(input_name, &sb) < 0 || sb.st_size < 1) {
		std::cerr << "Couldn't get size of file \"" << input_name << "\"." << std::endl;
		return 1;
	}
	if (sb.st_size > 16777216) {
		std::cerr << "Size of file \"" << input_name << "\" too large." << std::endl;
		return 1;
	}
	int input_bytes = sb.st_size;
	int chunk_bytes = std::atoi(argv[2]);
	int chunk_count = argc - 3;
	int cpf_overhead = 3 + 2 + 2 + 2 + 3 + 4; // CPF SPLITS IDENT SUB SIZE CRC32
	int avail_bytes = (chunk_bytes - cpf_overhead) & ~1;
	typedef CODE::PrimeField<uint64_t, 65537> PF;
	const int MAX_LEN = PF::P - 2;
	if (avail_bytes > MAX_LEN * 2) {
		std::cerr << "Size of chunks too large." << std::endl;
		return 1;
	}
	int block_count = (input_bytes + avail_bytes - 1) / avail_bytes;
	if (avail_bytes < 1 || block_count > 1024) {
		std::cerr << "Size of chunks too small." << std::endl;
		return 1;
	}
	if (chunk_count < block_count) {
		std::cerr << "Need at least " << block_count << " chunks." << std::endl;
		return 1;
	}
	std::cerr << "CPF(" << chunk_count << ", " << block_count << ")" << std::endl;
	std::ifstream input_file(input_name, std::ios::binary);
	if (input_file.bad()) {
		std::cerr << "Couldn't open file \"" << input_name << "\" for reading." << std::endl;
		return 1;
	}
	int block_values = (input_bytes + 2 * block_count - 1) / (2 * block_count);
	int total_values = block_values * block_count;
	uint16_t *input_data = new uint16_t[total_values];
	input_file.read(reinterpret_cast<char *>(input_data), input_bytes);
	static CODE::CRC<uint32_t> crc(0x8F6E37A0);
	for (int i = 0; i < input_bytes; ++i)
		crc(reinterpret_cast<uint8_t *>(input_data)[i]);
	for (int i = input_bytes; i < 2 * total_values; ++i)
		reinterpret_cast<uint8_t *>(input_data)[i] = 0;
	CODE::CauchyPrimeFieldErasureCoding<PF, uint16_t, MAX_LEN> cpf;
	uint16_t *chunk_data = new uint16_t[block_values];
	for (int i = 0; i < chunk_count; ++i) {
		int chunk_ident = block_count + i;
		int max_sub = cpf.encode(input_data, chunk_data, chunk_ident, block_values, block_count);
		const char *chunk_name = argv[3+i];
		std::ofstream chunk_file(chunk_name, std::ios::binary | std::ios::trunc);
		if (chunk_file.bad()) {
			std::cerr << "Couldn't open file \"" << chunk_name << "\" for writing." << std::endl;
			return 1;
		}
		chunk_file.write("CPF", 3);
		uint16_t splits = block_count - 1;
		chunk_file.write(reinterpret_cast<char *>(&splits), 2);
		uint16_t ident = chunk_ident;
		chunk_file.write(reinterpret_cast<char *>(&ident), 2);
		uint16_t sub = max_sub;
		chunk_file.write(reinterpret_cast<char *>(&sub), 2);
		int32_t size = input_bytes - 1;
		chunk_file.write(reinterpret_cast<char *>(&size), 3);
		uint32_t crc32 = crc();
		chunk_file.write(reinterpret_cast<char *>(&crc32), 4);
		chunk_file.write(reinterpret_cast<char *>(chunk_data), 2 * block_values);
	}
	delete[] input_data;
	delete[] chunk_data;
	return 0;
}

