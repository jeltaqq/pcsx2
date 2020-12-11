#pragma once

#include "Pcsx2Types.h"
#include <wx/gdicmn.h>

class res_dp_bottom
{
public:
	static const uint Length = 278;
	static const u8 Data[Length];
	static wxBitmapType GetFormat() { return wxBITMAP_TYPE_PNG; }
};

const u8 res_dp_bottom::Data[Length] =
{
		0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,0x00,
		0x00,0x00,0x17,0x00,0x00,0x00,0x1a,0x08,0x06,0x00,0x00,0x00,0x5c,0xb4,0xc7,0x7e,0x00,
		0x00,0x00,0x06,0x62,0x4b,0x47,0x44,0x00,0xff,0x00,0xff,0x00,0xff,0xa0,0xbd,0xa7,0x93,
		0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0b,0x13,0x00,0x00,0x0b,0x13,0x01,
		0x00,0x9a,0x9c,0x18,0x00,0x00,0x00,0x07,0x74,0x49,0x4d,0x45,0x07,0xdf,0x0b,0x1c,0x10,
		0x0f,0x02,0x37,0x13,0x03,0xff,0x00,0x00,0x00,0xa3,0x49,0x44,0x41,0x54,0x48,0xc7,0xed,
		0x95,0x3b,0x0e,0x02,0x31,0x0c,0x05,0xc7,0xc9,0x31,0x80,0x85,0x05,0x3a,0x38,0x12,0x87,
		0x5b,0xc4,0xa7,0xe3,0x4c,0x70,0x06,0x10,0xb5,0x69,0x00,0x51,0x04,0xad,0xb3,0x38,0x0d,
		0x8a,0x25,0x17,0x29,0x32,0x7e,0x7a,0x4e,0x6c,0xc1,0x10,0x0a,0xfa,0x79,0x16,0x10,0xcb,
		0x3d,0xc9,0x05,0xe7,0x14,0x09,0x43,0xc1,0x00,0xbb,0x76,0xa6,0x0c,0x09,0x05,0xb5,0xe4,
		0x71,0xb9,0xd0,0x2c,0x5b,0xfa,0x14,0x5b,0x2d,0x12,0x0f,0xf0,0xb7,0x02,0xe2,0x05,0x4e,
		0x15,0x08,0x00,0x5d,0x33,0x51,0x0f,0x70,0x52,0xa0,0xb5,0x79,0xd6,0xdc,0x3f,0x5f,0x91,
		0x78,0x29,0x4e,0xd9,0x13,0x28,0x18,0xc5,0x94,0x1f,0xe6,0x6d,0x39,0x78,0xef,0xf7,0xaf,
		0xf0,0x0a,0xaf,0xf0,0x7f,0x85,0x77,0xe3,0x11,0x6c,0xa7,0x8d,0x7a,0xcf,0xf3,0xd7,0xbc,
		0x7a,0xaf,0xa4,0xd3,0x7a,0xa5,0x31,0xc6,0xdf,0xb6,0x90,0xc2,0xfd,0x76,0x65,0x73,0xbe,
		0x08,0xc0,0x03,0xc6,0x9f,0x8f,0xdf,0x06,0x32,0xf9,0x74,0x00,0x00,0x00,0x00,0x49,0x45,
		0x4e,0x44,0xae,0x42,0x60,0x82
};
