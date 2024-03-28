/*
Cauchy Prime Field Erasure Coding

Copyright 2024 Ahmet Inan <inan@aicodix.de>
*/

#include <set>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include "crc.hh"
#include "prime_field.hh"
#include "cauchy_prime_field_erasure_coding.hh"

int main(int argc, char **argv)
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0] << " OUTPUT CHUNKS.." << std::endl;
		return 1;
	}
	int chunk_count = argc - 2;
	typedef CODE::PrimeField<uint64_t, 65537> PF;
	const int MAX_LEN = PF::P - 2;
	uint16_t *chunk_ident = nullptr;
	uint16_t *chunk_subst = nullptr;
	uint16_t *chunk_data = nullptr;
	int block_values = 0;
	int block_count = 0;
	int block_index = 0;
	int output_bytes = 0;
	uint32_t crc32_value = 0;
	bool first = true;
	std::set<uint16_t> list;
	for (int i = 0; i < chunk_count; ++i) {
		const char *chunk_name = argv[2+i];
		std::ifstream chunk_file(chunk_name, std::ios::binary);
		if (chunk_file.bad()) {
			std::cerr << "Couldn't open file \"" << chunk_name << "\" for reading." << std::endl;
			return 1;
		}
		char magic[3];
		chunk_file.read(magic, 3);
		uint16_t splits;
		chunk_file.read(reinterpret_cast<char *>(&splits), 2);
		uint16_t ident;
		chunk_file.read(reinterpret_cast<char *>(&ident), 2);
		uint16_t sub;
		chunk_file.read(reinterpret_cast<char *>(&sub), 2);
		int32_t size = 0;
		chunk_file.read(reinterpret_cast<char *>(&size), 3);
		uint32_t crc32;
		chunk_file.read(reinterpret_cast<char *>(&crc32), 4);
		int length = (size + 2 * splits + 2) / (2 * splits + 2);
		if (!chunk_file || magic[0] != 'C' || magic[1] != 'P' || magic[2] != 'F' || splits >= 1024 || length > MAX_LEN || ident <= splits || list.count(ident)) {
			std::cerr << "Skipping file \"" << chunk_name << "\"." << std::endl;
			continue;
		}
		if (first) {
			first = false;
			block_count = splits + 1;
			output_bytes = size + 1;
			block_values = length;
			crc32_value = crc32;
			chunk_ident = new uint16_t[block_count];
			chunk_subst = new uint16_t[block_count];
			chunk_data = new uint16_t[block_count * block_values];
		} else if (block_count != splits + 1 || output_bytes != size + 1 || crc32_value != crc32) {
			std::cerr << "Skipping file \"" << chunk_name << "\"." << std::endl;
			continue;
		}
		list.insert(ident);
		chunk_ident[block_index] = ident;
		chunk_subst[block_index] = sub;
		chunk_file.read(reinterpret_cast<char *>(chunk_data + block_index * block_values), 2 * block_values);
		if (++block_index >= block_count)
			break;
	}
	if (block_index != block_count) {
		std::cerr << "Need " << block_count << " valid chunks but only got " << block_index << "." << std::endl;
		return 1;
	}
	const char *output_name = argv[1];
	if (output_name[0] == '-' && output_name[1] == 0)
		output_name = "/dev/stdout";
	std::ofstream output_file(output_name, std::ios::binary | std::ios::trunc);
	if (output_file.bad()) {
		std::cerr << "Couldn't open file \"" << output_name << "\" for writing." << std::endl;
		return 1;
	}
	uint16_t *output_data = new uint16_t[block_values];
	CODE::CauchyPrimeFieldErasureCoding<PF, uint16_t, MAX_LEN> cpf;
	static CODE::CRC<uint32_t> crc(0x8F6E37A0);
	for (int i = 0, j = 0; i < block_count; ++i) {
		cpf.decode(output_data, chunk_data, chunk_subst, chunk_ident, i, block_values, block_count);
		int copy_bytes = 2 * block_values;
		j += copy_bytes;
		if (j > output_bytes)
			copy_bytes -= j - output_bytes;
		output_file.write(reinterpret_cast<char *>(output_data), copy_bytes);
		for (int k = 0; k < copy_bytes; ++k)
			crc(reinterpret_cast<uint8_t *>(output_data)[k]);
	}
	delete[] chunk_subst;
	delete[] chunk_ident;
	delete[] output_data;
	delete[] chunk_data;
	if (crc() != crc32_value) {
		std::cerr << "CRC value does not match!" << std::endl;
		return 1;
	}
	return 0;
}

