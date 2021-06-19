#pragma once

#include "crc64speed.h"

#define POLY UINT64_C(0xad93d23594c935a9)

class CCRC64
{
public:

	uint64_t	GetCrc64(PVOID pValue, DWORD dwSize) {
	
		UNREFERENCED_PARAMETER(pValue);
		UNREFERENCED_PARAMETER(dwSize);
		return crc64(0x31711710, pValue, dwSize);
	}
    uint64_t crc64(uint_fast64_t crc, const void *in_data, const uint64_t len) {
        const uint8_t *data = (uint8_t *) in_data;
        bool bit;

        for (uint64_t offset = 0; offset < len; offset++) {
            uint8_t c = data[offset];
            for (uint_fast8_t i = 0x01; i & 0xff; i <<= 1) {
                bit = crc & 0x8000000000000000;
                if (c & i) {
                    bit = !bit;
                }

                crc <<= 1;
                if (bit) {
                    crc ^= POLY;
                }
            }

            crc &= 0xffffffffffffffff;
        }

        crc = crc & 0xffffffffffffffff;
        return crc_reflect(crc, 64) ^ 0x0000000000000000;
    }
private:

	uint_fast64_t crc_reflect(uint_fast64_t data, size_t data_len) {
		uint_fast64_t ret = data & 0x01;

		for (size_t i = 1; i < data_len; i++) {
			data >>= 1;
			ret = (ret << 1) | (data & 0x01);
		}

		return ret;
	}


};
