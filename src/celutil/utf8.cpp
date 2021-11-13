// utf8.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cctype>
#include <cstring>
#include <wchar.h>
#include <climits>
#include <fmt/printf.h>
#include "stringutils.h"
#include "utf8.h"

uint16_t WGL4_Normalization_00[256] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
    0x0040, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
    0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
    0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
    0x0020, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x0020, 0x00a9, 0x0061, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x0020,
    0x00b0, 0x00b1, 0x0032, 0x0033, 0x0020, 0x03bc, 0x00b6, 0x00b7,
    0x0020, 0x0031, 0x006f, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x00c6, 0x0063,
    0x0065, 0x0065, 0x0065, 0x0065, 0x0069, 0x0069, 0x0069, 0x0069,
    0x00d0, 0x006e, 0x006f, 0x006f, 0x006f, 0x006f, 0x006f, 0x00d7,
    0x00d8, 0x0075, 0x0075, 0x0075, 0x0075, 0x0079, 0x00de, 0x00df,
    0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x00e6, 0x0063,
    0x0065, 0x0065, 0x0065, 0x0065, 0x0069, 0x0069, 0x0069, 0x0069,
    0x00f0, 0x006e, 0x006f, 0x006f, 0x006f, 0x006f, 0x006f, 0x00f7,
    0x00f8, 0x0075, 0x0075, 0x0075, 0x0075, 0x0079, 0x00fe, 0x0079,
};

uint16_t WGL4_Normalization_01[256] = {
    0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x0061, 0x0063, 0x0063,
    0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0064, 0x0064,
    0x0111, 0x0111, 0x0065, 0x0065, 0x0065, 0x0065, 0x0065, 0x0065,
    0x0065, 0x0065, 0x0065, 0x0065, 0x0067, 0x0067, 0x0067, 0x0067,
    0x0067, 0x0067, 0x0067, 0x0067, 0x0068, 0x0068, 0x0127, 0x0127,
    0x0069, 0x0069, 0x0069, 0x0069, 0x0069, 0x0069, 0x0069, 0x0069,
    0x0069, 0x0131, 0x0069, 0x0069, 0x006a, 0x006a, 0x006b, 0x006b,
    0x0138, 0x006c, 0x006c, 0x006c, 0x006c, 0x006c, 0x006c, 0x006c,
    0x006c, 0x0142, 0x0142, 0x006e, 0x006e, 0x006e, 0x006e, 0x006e,
    0x006e, 0x006e, 0x014a, 0x014b, 0x006f, 0x006f, 0x006f, 0x006f,
    0x006f, 0x006f, 0x0153, 0x0153, 0x0072, 0x0072, 0x0072, 0x0072,
    0x0072, 0x0072, 0x0073, 0x0073, 0x0073, 0x0073, 0x0073, 0x0073,
    0x0073, 0x0073, 0x0074, 0x0074, 0x0074, 0x0074, 0x0167, 0x0167,
    0x0075, 0x0075, 0x0075, 0x0075, 0x0075, 0x0075, 0x0075, 0x0075,
    0x0075, 0x0075, 0x0075, 0x0075, 0x0077, 0x0077, 0x0079, 0x0079,
    0x0079, 0x007a, 0x007a, 0x007a, 0x007a, 0x007a, 0x007a, 0x0073,
    0x0180, 0x0181, 0x0182, 0x0183, 0x0184, 0x0185, 0x0186, 0x0187,
    0x0188, 0x0189, 0x018a, 0x018b, 0x018c, 0x018d, 0x018e, 0x018f,
    0x0190, 0x0191, 0x0192, 0x0193, 0x0194, 0x0195, 0x0196, 0x0197,
    0x0198, 0x0199, 0x019a, 0x019b, 0x019c, 0x019d, 0x019e, 0x019f,
    0x01a0, 0x01a1, 0x01a2, 0x01a3, 0x01a4, 0x01a5, 0x01a6, 0x01a7,
    0x01a8, 0x01a9, 0x01aa, 0x01ab, 0x01ac, 0x01ad, 0x01ae, 0x01af,
    0x01b0, 0x01b1, 0x01b2, 0x01b3, 0x01b4, 0x01b5, 0x01b6, 0x01b7,
    0x01b8, 0x01b9, 0x01ba, 0x01bb, 0x01bc, 0x01bd, 0x01be, 0x01bf,
    0x01c0, 0x01c1, 0x01c2, 0x01c3, 0x01c4, 0x01c5, 0x01c6, 0x01c7,
    0x01c8, 0x01c9, 0x01ca, 0x01cb, 0x01cc, 0x01cd, 0x01ce, 0x01cf,
    0x01d0, 0x01d1, 0x01d2, 0x01d3, 0x01d4, 0x01d5, 0x01d6, 0x01d7,
    0x01d8, 0x01d9, 0x01da, 0x01db, 0x01dc, 0x01dd, 0x01de, 0x01df,
    0x01e0, 0x01e1, 0x01e2, 0x01e3, 0x01e4, 0x01e5, 0x01e6, 0x01e7,
    0x01e8, 0x01e9, 0x01ea, 0x01eb, 0x01ec, 0x01ed, 0x01ee, 0x01ef,
    0x01f0, 0x01f1, 0x01f2, 0x01f3, 0x01f4, 0x01f5, 0x01f6, 0x01f7,
    0x01f8, 0x01f9, 0x00e5, 0x00e5, 0x00e6, 0x00e6, 0x00f8, 0x00f8,
};

uint16_t WGL4_Normalization_02[256] = {
    0x0200, 0x0201, 0x0202, 0x0203, 0x0204, 0x0205, 0x0206, 0x0207,
    0x0208, 0x0209, 0x020a, 0x020b, 0x020c, 0x020d, 0x020e, 0x020f,
    0x0210, 0x0211, 0x0212, 0x0213, 0x0214, 0x0215, 0x0216, 0x0217,
    0x0218, 0x0219, 0x021a, 0x021b, 0x021c, 0x021d, 0x021e, 0x021f,
    0x0220, 0x0221, 0x0222, 0x0223, 0x0224, 0x0225, 0x0226, 0x0227,
    0x0228, 0x0229, 0x022a, 0x022b, 0x022c, 0x022d, 0x022e, 0x022f,
    0x0230, 0x0231, 0x0232, 0x0233, 0x0234, 0x0235, 0x0236, 0x0237,
    0x0238, 0x0239, 0x023a, 0x023b, 0x023c, 0x023d, 0x023e, 0x023f,
    0x0240, 0x0241, 0x0242, 0x0243, 0x0244, 0x0245, 0x0246, 0x0247,
    0x0248, 0x0249, 0x024a, 0x024b, 0x024c, 0x024d, 0x024e, 0x024f,
    0x0250, 0x0251, 0x0252, 0x0253, 0x0254, 0x0255, 0x0256, 0x0257,
    0x0258, 0x0259, 0x025a, 0x025b, 0x025c, 0x025d, 0x025e, 0x025f,
    0x0260, 0x0261, 0x0262, 0x0263, 0x0264, 0x0265, 0x0266, 0x0267,
    0x0268, 0x0269, 0x026a, 0x026b, 0x026c, 0x026d, 0x026e, 0x026f,
    0x0270, 0x0271, 0x0272, 0x0273, 0x0274, 0x0275, 0x0276, 0x0277,
    0x0278, 0x0279, 0x027a, 0x027b, 0x027c, 0x027d, 0x027e, 0x027f,
    0x0280, 0x0281, 0x0282, 0x0283, 0x0284, 0x0285, 0x0286, 0x0287,
    0x0288, 0x0289, 0x028a, 0x028b, 0x028c, 0x028d, 0x028e, 0x028f,
    0x0290, 0x0291, 0x0292, 0x0293, 0x0294, 0x0295, 0x0296, 0x0297,
    0x0298, 0x0299, 0x029a, 0x029b, 0x029c, 0x029d, 0x029e, 0x029f,
    0x02a0, 0x02a1, 0x02a2, 0x02a3, 0x02a4, 0x02a5, 0x02a6, 0x02a7,
    0x02a8, 0x02a9, 0x02aa, 0x02ab, 0x02ac, 0x02ad, 0x02ae, 0x02af,
    0x02b0, 0x02b1, 0x02b2, 0x02b3, 0x02b4, 0x02b5, 0x02b6, 0x02b7,
    0x02b8, 0x02b9, 0x02ba, 0x02bb, 0x02bc, 0x02bd, 0x02be, 0x02bf,
    0x02c0, 0x02c1, 0x02c2, 0x02c3, 0x02c4, 0x02c5, 0x02c6, 0x02c7,
    0x02c8, 0x02c9, 0x02ca, 0x02cb, 0x02cc, 0x02cd, 0x02ce, 0x02cf,
    0x02d0, 0x02d1, 0x02d2, 0x02d3, 0x02d4, 0x02d5, 0x02d6, 0x02d7,
    0x0020, 0x0020, 0x0020, 0x0020, 0x02dc, 0x0020, 0x02de, 0x02df,
    0x02e0, 0x02e1, 0x02e2, 0x02e3, 0x02e4, 0x02e5, 0x02e6, 0x02e7,
    0x02e8, 0x02e9, 0x02ea, 0x02eb, 0x02ec, 0x02ed, 0x02ee, 0x02ef,
    0x02f0, 0x02f1, 0x02f2, 0x02f3, 0x02f4, 0x02f5, 0x02f6, 0x02f7,
    0x02f8, 0x02f9, 0x02fa, 0x02fb, 0x02fc, 0x02fd, 0x02fe, 0x02ff,
};

uint16_t WGL4_Normalization_03[256] = {
    0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 0x0307,
    0x0308, 0x0309, 0x030a, 0x030b, 0x030c, 0x030d, 0x030e, 0x030f,
    0x0310, 0x0311, 0x0312, 0x0313, 0x0314, 0x0315, 0x0316, 0x0317,
    0x0318, 0x0319, 0x031a, 0x031b, 0x031c, 0x031d, 0x031e, 0x031f,
    0x0320, 0x0321, 0x0322, 0x0323, 0x0324, 0x0325, 0x0326, 0x0327,
    0x0328, 0x0329, 0x032a, 0x032b, 0x032c, 0x032d, 0x032e, 0x032f,
    0x0330, 0x0331, 0x0332, 0x0333, 0x0334, 0x0335, 0x0336, 0x0337,
    0x0338, 0x0339, 0x033a, 0x033b, 0x033c, 0x033d, 0x033e, 0x033f,
    0x0340, 0x0341, 0x0342, 0x0343, 0x0344, 0x0345, 0x0346, 0x0347,
    0x0348, 0x0349, 0x034a, 0x034b, 0x034c, 0x034d, 0x034e, 0x034f,
    0x0350, 0x0351, 0x0352, 0x0353, 0x0354, 0x0355, 0x0356, 0x0357,
    0x0358, 0x0359, 0x035a, 0x035b, 0x035c, 0x035d, 0x035e, 0x035f,
    0x0360, 0x0361, 0x0362, 0x0363, 0x0364, 0x0365, 0x0366, 0x0367,
    0x0368, 0x0369, 0x036a, 0x036b, 0x036c, 0x036d, 0x036e, 0x036f,
    0x0370, 0x0371, 0x0372, 0x0373, 0x0374, 0x0375, 0x0376, 0x0377,
    0x0378, 0x0379, 0x037a, 0x037b, 0x037c, 0x037d, 0x037e, 0x037f,
    0x0380, 0x0381, 0x0382, 0x0383, 0x0020, 0x00a8, 0x03b1, 0x00b7,
    0x03b5, 0x03b7, 0x03b9, 0x038b, 0x03bf, 0x038d, 0x03c5, 0x03c9,
    0x03ca, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
    0x03a0, 0x03a1, 0x03a2, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7,
    0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03ae, 0x03af,
    0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
    0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7,
    0x03c8, 0x03c9, 0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0x03cf,
    0x03d0, 0x03d1, 0x03d2, 0x03d3, 0x03d4, 0x03d5, 0x03d6, 0x03d7,
    0x03d8, 0x03d9, 0x03da, 0x03db, 0x03dc, 0x03dd, 0x03de, 0x03df,
    0x03e0, 0x03e1, 0x03e2, 0x03e3, 0x03e4, 0x03e5, 0x03e6, 0x03e7,
    0x03e8, 0x03e9, 0x03ea, 0x03eb, 0x03ec, 0x03ed, 0x03ee, 0x03ef,
    0x03f0, 0x03f1, 0x03f2, 0x03f3, 0x03f4, 0x03f5, 0x03f6, 0x03f7,
    0x03f8, 0x03f9, 0x03fa, 0x03fb, 0x03fc, 0x03fd, 0x03fe, 0x03ff,
};

uint16_t WGL4_Normalization_04[256] = {
    0x0400, 0x0435, 0x0452, 0x0433, 0x0454, 0x0455, 0x0456, 0x0456,
    0x0458, 0x0459, 0x045a, 0x045b, 0x043a, 0x040d, 0x0443, 0x045f,
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0438, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x044f,
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0438, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x044f,
    0x0450, 0x0435, 0x0452, 0x0433, 0x0454, 0x0455, 0x0456, 0x0456,
    0x0458, 0x0459, 0x045a, 0x045b, 0x043a, 0x045d, 0x0443, 0x045f,
    0x0460, 0x0461, 0x0462, 0x0463, 0x0464, 0x0465, 0x0466, 0x0467,
    0x0468, 0x0469, 0x046a, 0x046b, 0x046c, 0x046d, 0x046e, 0x046f,
    0x0470, 0x0471, 0x0472, 0x0473, 0x0474, 0x0475, 0x0476, 0x0477,
    0x0478, 0x0479, 0x047a, 0x047b, 0x047c, 0x047d, 0x047e, 0x047f,
    0x0480, 0x0481, 0x0482, 0x0483, 0x0484, 0x0485, 0x0486, 0x0487,
    0x0488, 0x0489, 0x048a, 0x048b, 0x048c, 0x048d, 0x048e, 0x048f,
    0x0491, 0x0491, 0x0492, 0x0493, 0x0494, 0x0495, 0x0496, 0x0497,
    0x0498, 0x0499, 0x049a, 0x049b, 0x049c, 0x049d, 0x049e, 0x049f,
    0x04a0, 0x04a1, 0x04a2, 0x04a3, 0x04a4, 0x04a5, 0x04a6, 0x04a7,
    0x04a8, 0x04a9, 0x04aa, 0x04ab, 0x04ac, 0x04ad, 0x04ae, 0x04af,
    0x04b0, 0x04b1, 0x04b2, 0x04b3, 0x04b4, 0x04b5, 0x04b6, 0x04b7,
    0x04b8, 0x04b9, 0x04ba, 0x04bb, 0x04bc, 0x04bd, 0x04be, 0x04bf,
    0x04c0, 0x04c1, 0x04c2, 0x04c3, 0x04c4, 0x04c5, 0x04c6, 0x04c7,
    0x04c8, 0x04c9, 0x04ca, 0x04cb, 0x04cc, 0x04cd, 0x04ce, 0x04cf,
    0x04d0, 0x04d1, 0x04d2, 0x04d3, 0x04d4, 0x04d5, 0x04d6, 0x04d7,
    0x04d8, 0x04d9, 0x04da, 0x04db, 0x04dc, 0x04dd, 0x04de, 0x04df,
    0x04e0, 0x04e1, 0x04e2, 0x04e3, 0x04e4, 0x04e5, 0x04e6, 0x04e7,
    0x04e8, 0x04e9, 0x04ea, 0x04eb, 0x04ec, 0x04ed, 0x04ee, 0x04ef,
    0x04f0, 0x04f1, 0x04f2, 0x04f3, 0x04f4, 0x04f5, 0x04f6, 0x04f7,
    0x04f8, 0x04f9, 0x04fa, 0x04fb, 0x04fc, 0x04fd, 0x04fe, 0x04ff,
};

uint16_t WGL4_Normalization_1e[256] = {
    0x1e00, 0x1e01, 0x1e02, 0x1e03, 0x1e04, 0x1e05, 0x1e06, 0x1e07,
    0x1e08, 0x1e09, 0x1e0a, 0x1e0b, 0x1e0c, 0x1e0d, 0x1e0e, 0x1e0f,
    0x1e10, 0x1e11, 0x1e12, 0x1e13, 0x1e14, 0x1e15, 0x1e16, 0x1e17,
    0x1e18, 0x1e19, 0x1e1a, 0x1e1b, 0x1e1c, 0x1e1d, 0x1e1e, 0x1e1f,
    0x1e20, 0x1e21, 0x1e22, 0x1e23, 0x1e24, 0x1e25, 0x1e26, 0x1e27,
    0x1e28, 0x1e29, 0x1e2a, 0x1e2b, 0x1e2c, 0x1e2d, 0x1e2e, 0x1e2f,
    0x1e30, 0x1e31, 0x1e32, 0x1e33, 0x1e34, 0x1e35, 0x1e36, 0x1e37,
    0x1e38, 0x1e39, 0x1e3a, 0x1e3b, 0x1e3c, 0x1e3d, 0x1e3e, 0x1e3f,
    0x1e40, 0x1e41, 0x1e42, 0x1e43, 0x1e44, 0x1e45, 0x1e46, 0x1e47,
    0x1e48, 0x1e49, 0x1e4a, 0x1e4b, 0x1e4c, 0x1e4d, 0x1e4e, 0x1e4f,
    0x1e50, 0x1e51, 0x1e52, 0x1e53, 0x1e54, 0x1e55, 0x1e56, 0x1e57,
    0x1e58, 0x1e59, 0x1e5a, 0x1e5b, 0x1e5c, 0x1e5d, 0x1e5e, 0x1e5f,
    0x1e60, 0x1e61, 0x1e62, 0x1e63, 0x1e64, 0x1e65, 0x1e66, 0x1e67,
    0x1e68, 0x1e69, 0x1e6a, 0x1e6b, 0x1e6c, 0x1e6d, 0x1e6e, 0x1e6f,
    0x1e70, 0x1e71, 0x1e72, 0x1e73, 0x1e74, 0x1e75, 0x1e76, 0x1e77,
    0x1e78, 0x1e79, 0x1e7a, 0x1e7b, 0x1e7c, 0x1e7d, 0x1e7e, 0x1e7f,
    0x0077, 0x0077, 0x0077, 0x0077, 0x0077, 0x0077, 0x1e86, 0x1e87,
    0x1e88, 0x1e89, 0x1e8a, 0x1e8b, 0x1e8c, 0x1e8d, 0x1e8e, 0x1e8f,
    0x1e90, 0x1e91, 0x1e92, 0x1e93, 0x1e94, 0x1e95, 0x1e96, 0x1e97,
    0x1e98, 0x1e99, 0x1e9a, 0x1e9b, 0x1e9c, 0x1e9d, 0x1e9e, 0x1e9f,
    0x1ea0, 0x1ea1, 0x1ea2, 0x1ea3, 0x1ea4, 0x1ea5, 0x1ea6, 0x1ea7,
    0x1ea8, 0x1ea9, 0x1eaa, 0x1eab, 0x1eac, 0x1ead, 0x1eae, 0x1eaf,
    0x1eb0, 0x1eb1, 0x1eb2, 0x1eb3, 0x1eb4, 0x1eb5, 0x1eb6, 0x1eb7,
    0x1eb8, 0x1eb9, 0x1eba, 0x1ebb, 0x1ebc, 0x1ebd, 0x1ebe, 0x1ebf,
    0x1ec0, 0x1ec1, 0x1ec2, 0x1ec3, 0x1ec4, 0x1ec5, 0x1ec6, 0x1ec7,
    0x1ec8, 0x1ec9, 0x1eca, 0x1ecb, 0x1ecc, 0x1ecd, 0x1ece, 0x1ecf,
    0x1ed0, 0x1ed1, 0x1ed2, 0x1ed3, 0x1ed4, 0x1ed5, 0x1ed6, 0x1ed7,
    0x1ed8, 0x1ed9, 0x1eda, 0x1edb, 0x1edc, 0x1edd, 0x1ede, 0x1edf,
    0x1ee0, 0x1ee1, 0x1ee2, 0x1ee3, 0x1ee4, 0x1ee5, 0x1ee6, 0x1ee7,
    0x1ee8, 0x1ee9, 0x1eea, 0x1eeb, 0x1eec, 0x1eed, 0x1eee, 0x1eef,
    0x1ef0, 0x1ef1, 0x0079, 0x0079, 0x1ef4, 0x1ef5, 0x1ef6, 0x1ef7,
    0x1ef8, 0x1ef9, 0x1efa, 0x1efb, 0x1efc, 0x1efd, 0x1efe, 0x1eff,
};

uint16_t WGL4_Normalization_21[256] = {
    0x2100, 0x2101, 0x2102, 0x2103, 0x2104, 0x2105, 0x2106, 0x2107,
    0x2108, 0x2109, 0x210a, 0x210b, 0x210c, 0x210d, 0x210e, 0x210f,
    0x2110, 0x2111, 0x2112, 0x006c, 0x2114, 0x2115, 0x2116, 0x2117,
    0x2118, 0x2119, 0x211a, 0x211b, 0x211c, 0x211d, 0x211e, 0x211f,
    0x2120, 0x2121, 0x2122, 0x2123, 0x2124, 0x2125, 0x03c9, 0x2127,
    0x2128, 0x2129, 0x212a, 0x212b, 0x212c, 0x212d, 0x212e, 0x212f,
    0x2130, 0x2131, 0x2132, 0x2133, 0x2134, 0x2135, 0x2136, 0x2137,
    0x2138, 0x2139, 0x213a, 0x213b, 0x213c, 0x213d, 0x213e, 0x213f,
    0x2140, 0x2141, 0x2142, 0x2143, 0x2144, 0x2145, 0x2146, 0x2147,
    0x2148, 0x2149, 0x214a, 0x214b, 0x214c, 0x214d, 0x214e, 0x214f,
    0x2150, 0x2151, 0x2152, 0x2153, 0x2154, 0x2155, 0x2156, 0x2157,
    0x2158, 0x2159, 0x215a, 0x215b, 0x215c, 0x215d, 0x215e, 0x215f,
    0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167,
    0x2168, 0x2169, 0x216a, 0x216b, 0x216c, 0x216d, 0x216e, 0x216f,
    0x2170, 0x2171, 0x2172, 0x2173, 0x2174, 0x2175, 0x2176, 0x2177,
    0x2178, 0x2179, 0x217a, 0x217b, 0x217c, 0x217d, 0x217e, 0x217f,
    0x2180, 0x2181, 0x2182, 0x2183, 0x2184, 0x2185, 0x2186, 0x2187,
    0x2188, 0x2189, 0x218a, 0x218b, 0x218c, 0x218d, 0x218e, 0x218f,
    0x2190, 0x2191, 0x2192, 0x2193, 0x2194, 0x2195, 0x2196, 0x2197,
    0x2198, 0x2199, 0x219a, 0x219b, 0x219c, 0x219d, 0x219e, 0x219f,
    0x21a0, 0x21a1, 0x21a2, 0x21a3, 0x21a4, 0x21a5, 0x21a6, 0x21a7,
    0x21a8, 0x21a9, 0x21aa, 0x21ab, 0x21ac, 0x21ad, 0x21ae, 0x21af,
    0x21b0, 0x21b1, 0x21b2, 0x21b3, 0x21b4, 0x21b5, 0x21b6, 0x21b7,
    0x21b8, 0x21b9, 0x21ba, 0x21bb, 0x21bc, 0x21bd, 0x21be, 0x21bf,
    0x21c0, 0x21c1, 0x21c2, 0x21c3, 0x21c4, 0x21c5, 0x21c6, 0x21c7,
    0x21c8, 0x21c9, 0x21ca, 0x21cb, 0x21cc, 0x21cd, 0x21ce, 0x21cf,
    0x21d0, 0x21d1, 0x21d2, 0x21d3, 0x21d4, 0x21d5, 0x21d6, 0x21d7,
    0x21d8, 0x21d9, 0x21da, 0x21db, 0x21dc, 0x21dd, 0x21de, 0x21df,
    0x21e0, 0x21e1, 0x21e2, 0x21e3, 0x21e4, 0x21e5, 0x21e6, 0x21e7,
    0x21e8, 0x21e9, 0x21ea, 0x21eb, 0x21ec, 0x21ed, 0x21ee, 0x21ef,
    0x21f0, 0x21f1, 0x21f2, 0x21f3, 0x21f4, 0x21f5, 0x21f6, 0x21f7,
    0x21f8, 0x21f9, 0x21fa, 0x21fb, 0x21fc, 0x21fd, 0x21fe, 0x21ff,
};

uint16_t* WGL4NormalizationTables[256] = {
    WGL4_Normalization_00,
    WGL4_Normalization_01,
    WGL4_Normalization_02,
    WGL4_Normalization_03,
    WGL4_Normalization_04,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, WGL4_Normalization_1e, nullptr,
    nullptr, WGL4_Normalization_21, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};


//! Decode the UTF-8 characters in string str beginning at position pos.
//! The decoded character is returned in ch; the return value of the function
//! is true if a valid UTF-8 sequence was successfully decoded.
bool UTF8Decode(const std::string& str, int pos, wchar_t& ch)
{
    auto c0 = (unsigned int) str[pos];
    int charlen = UTF8EncodedSizeFromFirstByte(c0);

    // Bad UTF-8 character that extends past end of string
    if (pos + charlen > (int)str.length())
        return false;

    // TODO: Should check that the bytes of characters after the first are all
    // of the form 01xxxxxx
    // TODO: Need to reject overlong encoding sequences

    switch (charlen)
    {
    case 1:
        ch = c0;
        return true;

    case 2:
        ch = ((c0 & 0x1f) << 6) | ((unsigned int) str[pos + 1] & 0x3f);
        return true;

    case 3:
        ch = ((c0 & 0x0f) << 12) |
            (((unsigned int) str[pos + 1] & 0x3f) << 6) |
            ((unsigned int)  str[pos + 2] & 0x3f);
        return true;

    case 4:
        ch = ((c0 & 0x07) << 18) |
            (((unsigned int) str[pos + 1] & 0x3f) << 12) |
            (((unsigned int) str[pos + 2] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 3] & 0x3f);
        return true;

    case 5:
        ch = ((c0 & 0x03) << 24) |
            (((unsigned int) str[pos + 1] & 0x3f) << 18) |
            (((unsigned int) str[pos + 2] & 0x3f) << 12) |
            (((unsigned int) str[pos + 3] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 4] & 0x3f);
        return true;

    case 6:
        ch = ((c0 & 0x01) << 30) |
            (((unsigned int) str[pos + 1] & 0x3f) << 24) |
            (((unsigned int) str[pos + 2] & 0x3f) << 18) |
            (((unsigned int) str[pos + 3] & 0x3f) << 12) |
            (((unsigned int) str[pos + 4] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 5] & 0x3f);
        return true;

    default:
        return false;
    }
}


//! Decode the UTF-8 characters in string str beginning at position pos.
//! The decoded character is returned in ch; the return value of the function
//! is true if a valid UTF-8 sequence was successfully decoded.
bool UTF8Decode(const char* str, int pos, int length, wchar_t& ch)
{
    auto c0 = (unsigned int) str[pos];
    int charlen = UTF8EncodedSizeFromFirstByte(c0);

    // Bad UTF-8 character that extends past end of string
    if (pos + charlen > length)
        return false;

    // TODO: Should check that the bytes of characters after the first are all
    // of the form 01xxxxxx
    // TODO: Need to reject overlong encoding sequences

    switch (charlen)
    {
    case 1:
        ch = c0;
        return true;

    case 2:
        ch = ((c0 & 0x1f) << 6) | ((unsigned int) str[pos + 1] & 0x3f);
        return true;

    case 3:
        ch = ((c0 & 0x0f) << 12) |
            (((unsigned int) str[pos + 1] & 0x3f) << 6) |
            ((unsigned int)  str[pos + 2] & 0x3f);
        return true;

    case 4:
        ch = ((c0 & 0x07) << 18) |
            (((unsigned int) str[pos + 1] & 0x3f) << 12) |
            (((unsigned int) str[pos + 2] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 3] & 0x3f);
        return true;

    case 5:
        ch = ((c0 & 0x03) << 24) |
            (((unsigned int) str[pos + 1] & 0x3f) << 18) |
            (((unsigned int) str[pos + 2] & 0x3f) << 12) |
            (((unsigned int) str[pos + 3] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 4] & 0x3f);
        return true;

    case 6:
        ch = ((c0 & 0x01) << 30) |
            (((unsigned int) str[pos + 1] & 0x3f) << 24) |
            (((unsigned int) str[pos + 2] & 0x3f) << 18) |
            (((unsigned int) str[pos + 3] & 0x3f) << 12) |
            (((unsigned int) str[pos + 4] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 5] & 0x3f);
        return true;

    default:
        return false;
    }
}


//! Appends the UTF-8 encoded version of the code point ch to the
//! destination string
void UTF8Encode(std::uint32_t ch, std::string& dest)
{
    if (ch < 0x80)
    {
        dest.push_back(static_cast<char>(ch));
    }
    else if (ch < 0x800)
    {
        dest.push_back(static_cast<char>(0xc0 | (ch >> 6)));
        dest.push_back(static_cast<char>(0x80 | (ch & 0x3f)));
    }
    else if (ch < 0x10000)
    {
        if (ch < 0xd800 || ch >= 0xe000)
        {
            dest.push_back(static_cast<char>(0xe0 | (ch >> 12)));
            dest.push_back(static_cast<char>(0x80 | ((ch & 0xfff) >> 6)));
            dest.push_back(static_cast<char>(0x80 | (ch & 0x3f)));
        }
        else
        {
            // disallow surrogates
            dest.append(UTF8_REPLACEMENT_CHAR);
        }
    }
#if WCHAR_MAX > 0xFFFFu
    else if (ch < 0x110000)
    {
        dest.push_back(static_cast<char>(0xf0 | (ch >> 18)));
        dest.push_back(static_cast<char>(0x80 | ((ch & 0x3ffff) >> 12)));
        dest.push_back(static_cast<char>(0x80 | ((ch & 0xfff) >> 6)));
        dest.push_back(static_cast<char>(0x80 | (ch & 0x3f)));
    }
#endif
    else
    {
        // not a valid Unicode code point, or we only support BMP characters,
        // so fall back to U+FFFD REPLACEMENT CHARACTER
        dest.append(UTF8_REPLACEMENT_CHAR);
    }
}


//! Return the number of characters encoded by a UTF-8 string
int UTF8Length(const std::string& s)
{
    int len = s.length();
    int count = 0;
    for (int i = 0; i < len; i++)
    {
        auto c = (unsigned int) ((unsigned char) s[i]);
        if ((c < 0x80) || (c >= 0xc2 && c <= 0xf4))
            count++;
    }

    return count;
}


inline wchar_t UTF8Normalize(wchar_t ch)
{
    auto page = (unsigned int) ch >> 8;
    if (page >= 256)
        return ch;

    uint16_t* normTable = WGL4NormalizationTables[page];
    if (normTable == nullptr)
        return ch;

    return (wchar_t) normTable[(unsigned int) ch & 0xff];
}


//! Perform a normalized comparison of two UTF-8 strings.  The normalization
//! only works for characters in the WGL-4 subset, and no multicharacter
//! translations are performed.
int UTF8StringCompare(const std::string& s0, const std::string& s1)
{
    int len0 = s0.length();
    int len1 = s1.length();
    int i0 = 0;
    int i1 = 0;
    while (i0 < len0 && i1 < len1)
    {
        wchar_t ch0 = 0;
        wchar_t ch1 = 0;
        if (!UTF8Decode(s0, i0, ch0))
            return 1;
        if (!UTF8Decode(s1, i1, ch1))
            return -1;

        i0 += UTF8EncodedSize(ch0);
        i1 += UTF8EncodedSize(ch1);
        ch0 = UTF8Normalize(ch0);
        ch1 = UTF8Normalize(ch1);

        if (ch0 < ch1)
            return -1;
        if (ch0 > ch1)
            return 1;
    }

    if (i0 == len0 && i1 == len1)
        return 0;

    len0 = UTF8Length(s0);
    len1 = UTF8Length(s1);
    if (len0 > len1)
        return 1;
    if (len0 < len1)
        return -1;
    else
        return 0;
}

int UTF8StringCompare(const std::string& s0, const std::string& s1, size_t n, bool ignoreCase)
{
    int len0 = s0.length();
    int len1 = s1.length();
    int i0 = 0;
    int i1 = 0;
    while (i0 < len0 && i1 < len1 && n > 0)
    {
        wchar_t ch0 = 0;
        wchar_t ch1 = 0;
        if (!UTF8Decode(s0, i0, ch0))
            return 1;
        if (!UTF8Decode(s1, i1, ch1))
            return -1;

        i0 += UTF8EncodedSize(ch0);
        i1 += UTF8EncodedSize(ch1);
        ch0 = UTF8Normalize(ch0);
        ch1 = UTF8Normalize(ch1);

        if (ignoreCase)
        {
            ch0 = std::tolower(ch0);
            ch1 = std::tolower(ch1);
        }
        if (ch0 < ch1)
            return -1;
        if (ch0 > ch1)
            return 1;

        n--;
    }

    if (n == 0)
        return 0;

    len0 = UTF8Length(s0);
    len1 = UTF8Length(s1);
    if (len0 > len1)
        return 1;
    if (len0 < len1)
        return -1;
    else
        return 0;
}


#if 0
//! Currently incomplete, but could be a helpful class for dealing with
//! UTF-8 streams
class UTF8StringIterator
{
public:
    UTF8StringIterator(const std::string& _str) : str(_str) {};
    UTF8StringIterator(const UTF8StringIterator& iter) = default;

    UTF8StringIterator& operator++();
    UTF8StringIterator& operator++(int);

private:
    const std::string& str;
    int position{ 0 };
};


UTF8StringIterator& UTF8StringIterator::operator++()
{
    return *this;
}


UTF8StringIterator& UTF8StringIterator::operator++(int)
{
    return *this;
}
#endif


static const char *greekAlphabet[] =
{
    "Alpha",
    "Beta",
    "Gamma",
    "Delta",
    "Epsilon",
    "Zeta",
    "Eta",
    "Theta",
    "Iota",
    "Kappa",
    "Lambda",
    "Mu",
    "Nu",
    "Xi",
    "Omicron",
    "Pi",
    "Rho",
    "Sigma",
    "Tau",
    "Upsilon",
    "Phi",
    "Chi",
    "Psi",
    "Omega"
};

static const char* greekAlphabetUTF8[] =
{
    "\316\261",
    "\316\262",
    "\316\263",
    "\316\264",
    "\316\265",
    "\316\266",
    "\316\267",
    "\316\270",
    "\316\271",
    "\316\272",
    "\316\273",
    "\316\274",
    "\316\275",
    "\316\276",
    "\316\277",
    "\317\200",
    "\317\201",
    "\317\203",
    "\317\204",
    "\317\205",
    "\317\206",
    "\317\207",
    "\317\210",
    "\317\211",
};

static const char* canonicalAbbrevs[] =
{
    "ALF", "BET", "GAM", "DEL", "EPS", "ZET", "ETA", "TET",
    "IOT", "KAP", "LAM", "MU" , "NU" , "XI" , "OMI", "PI" ,
    "RHO", "SIG", "TAU", "UPS", "PHI", "CHI", "PSI", "OME",
};

static std::string noAbbrev;

// Greek alphabet crud . . . should probably moved to it's own module.

static size_t greekChunkLength(const std::string&);

Greek* Greek::m_instance = nullptr;

Greek* Greek::getInstance()
{
    if (m_instance == nullptr)
        m_instance = new Greek();
    return m_instance;
}

Greek::Greek()
{
    nLetters = sizeof(greekAlphabet) / sizeof(greekAlphabet[0]);
    names = new std::string[nLetters];
    abbrevs = new std::string[nLetters];

    for (int i = 0; i < nLetters; i++)
    {
        names[i] = std::string(greekAlphabet[i]);
        abbrevs[i] = std::string(canonicalAbbrevs[i]);
    }
}

Greek::~Greek()
{
    delete[] names;
    delete[] abbrevs;
}

const std::string& Greek::canonicalAbbreviation(const std::string& letter)
{
    Greek *instance = Greek::getInstance();
    int i;
    for (i = 0; i < instance->nLetters; i++)
    {
        if (compareIgnoringCase(letter, instance->names[i]) == 0)
            return instance->abbrevs[i];
    }

    for (i = 0; i < instance->nLetters; i++)
    {
        if (compareIgnoringCase(letter, instance->abbrevs[i]) == 0)
            return instance->abbrevs[i];
    }

    if (letter.length() == 2)
    {
        for (i = 0; i < instance->nLetters; i++)
        {
            if (letter[0] == greekAlphabetUTF8[i][0] &&
                letter[1] == greekAlphabetUTF8[i][1])
            {
                return instance->abbrevs[i];
            }
        }
    }

    return noAbbrev;
}

static const char* toSuperscript(char c)
{
    switch (c)
    {
    case '0':
        return UTF8_SUPERSCRIPT_0;
    case '1':
        return UTF8_SUPERSCRIPT_1;
    case '2':
        return UTF8_SUPERSCRIPT_2;
    case '3':
        return UTF8_SUPERSCRIPT_3;
    case '4':
        return UTF8_SUPERSCRIPT_4;
    case '5':
        return UTF8_SUPERSCRIPT_5;
    case '6':
        return UTF8_SUPERSCRIPT_6;
    case '7':
        return UTF8_SUPERSCRIPT_7;
    case '8':
        return UTF8_SUPERSCRIPT_8;
    case '9':
        return UTF8_SUPERSCRIPT_9;
    default:
        return nullptr;
    }
}

//! Replaces the Greek letter abbreviation at the beginning
//! of a string by the UTF-8 representation of that letter.
//! Also, replace digits following Greek letters with UTF-8
//! superscripts.
std::string ReplaceGreekLetterAbbr(const std::string& str)
{
    Greek *instance = Greek::getInstance();
    size_t len = greekChunkLength(str);

    if (str[0] >= 'A' && str[0] <= 'Z')
    {
        // Linear search through all letter abbreviations
        for (int i = 0; i < instance->nLetters; i++)
        {
            std::string prefix = instance->abbrevs[i];
            if (len != prefix.length() || UTF8StringCompare(str, prefix, len, true) != 0)
            {
                prefix = instance->names[i];
                if (len != prefix.length() || UTF8StringCompare(str, prefix, len, true) != 0)
                    continue;
            }

            std::string ret = greekAlphabetUTF8[i];
            auto len = prefix.length();
            for (; str.length() > len && isdigit(str[len]); len++)
                ret += toSuperscript(str[len]);
            ret += str.substr(len);
            return ret;
        }
    }

    return str;
}

//! Replaces the Greek letter abbreviation at the beginning
//! of a string by the UTF-8 representation of that letter.
//! Also, replace digits following Greek letters with UTF-8
//! superscripts. Operates on char* instead of strings--less
//! convenient, but more efficient. Return the number of
//! characters copied to the destination string, not
//! including the zero terminator.
#if 0
unsigned int
ReplaceGreekLetterAbbr(char *dst, unsigned int dstSize, const char* src, unsigned int srcLength)
{
    Greek *instance = Greek::getInstance();
    if (src[0] >= 'A' && src[0] <= 'Z' &&
        src[1] >= 'A' && src[1] <= 'Z')
    {
        // Linear search through all letter abbreviations
        for (unsigned int i = 0; i < (unsigned int) instance->nLetters; i++)
        {
            const char* abbr = canonicalAbbrevs[i];
            unsigned int j = 0;
            while (abbr[j] == src[j] && abbr[j] != '\0' && src[j] != '\0')
                j++;

            // It's a match if we reached the end of the abbreviation string
            if (abbr[j] == '\0')
            {
                unsigned int abbrevLength = j;
                unsigned int srcIndex = j;
                const char *superscript = toSuperscript(src[abbrevLength]);

                const char* utfGreek = greekAlphabetUTF8[i];
                unsigned int utfGreekLength = strlen(utfGreek);

                unsigned int requiredLength = srcLength;
                if (utfGreekLength > abbrevLength)
                    requiredLength += utfGreekLength - abbrevLength;
                if (superscript != nullptr)
                {
                    requiredLength += strlen(superscript) - 1;
                    srcIndex++;
                }

                // If there's not enough room, give up translating and just copy as much as possible
                if (requiredLength + 1 > dstSize)
                    break;

                unsigned int dstIndex = 0;
                j = 0;
                while (utfGreek[j] != 0)
                {
                    dst[dstIndex++] = utfGreek[j];
                    j++;
                }

                if (superscript != nullptr)
                {
                    j = 0;
                    while (superscript[j] != 0)
                    {
                        dst[dstIndex++] = superscript[j];
                        j++;
                    }
                }

                while (src[srcIndex] != 0)
                {
                    dst[dstIndex++] = src[srcIndex++];
                }
                dst[dstIndex] = '\0';

                return dstIndex;
            }
        }
    }

    strncpy(dst, src, dstSize);
    if (dstSize > srcLength)
        return srcLength;


    if (dstSize > 0)
    {
        dst[dstSize - 1] = '\0';
        return dstSize - 1;
    }

    return 0;
}
#endif

static int findGreekNameIndexBySubstr(const std::string &, int = 0, unsigned int = UINT_MAX);
#if 0
static std::string firstGreekAbbrCompletion(const std::string &);
#endif

bool inline isSubstringIgnoringCase(const std::string &s0, const std::string &s1, size_t n)
{
    return UTF8StringCompare(s0, s1, n, true) == 0;
}

static int findGreekNameIndexBySubstr(const std::string &s, int start, unsigned int n)
{
    Greek *instance = Greek::getInstance();

    if (s.empty())
        return -1;

    for (int i = start; i < instance->nLetters; i++)
    {
        if (isSubstringIgnoringCase(instance->names[i], s, n))
            return i;
    }

    for (int i = start; i < instance->nLetters; i++)
    {
        if (isSubstringIgnoringCase(instance->abbrevs[i], s, n))
            return i;
    }

    return -1;
}

static size_t greekChunkLength(const std::string& str)
{
    bool npos = false;
    size_t sp = str.find_first_of(' ');
    if (sp == std::string::npos)
    {
        sp = str.length();
        npos = true;
    }

    if (sp != 0 && isdigit(static_cast<unsigned char>(str[sp - 1])))
        while(sp != 0 && isdigit(static_cast<unsigned char>(str[sp - 1]))) sp--;
    else if (npos)
        sp = std::string::npos;
    return sp;
}

#if 0
static std::string firstGreekAbbrCompletion(const std::string &s)
{
    std::string ret;
    size_t sp = greekChunkLength(s);
    if (sp == std::string::npos)
    {
        int i = findGreekNameIndexBySubstr(s);
        return (i >= 0) ? Greek::getInstance()->abbrevs[i] : s;
    }
    else
    {
        std::string prefix = s.substr(0, sp);
        ret = Greek::canonicalAbbreviation(prefix);
        return ret.empty() ? s : prefix + s.substr(sp);
    }

    return ret;
}
#endif

std::vector<std::string> getGreekCompletion(const std::string &s)
{
    std::vector<std::string> ret;
    if (s.empty())
        return ret;

    size_t sp = greekChunkLength(s);
    if (sp == std::string::npos)
    {
        sp = UTF8Length(s);
        for(int i = 0; i >= 0;)
        {
            std::string rets;
            i = findGreekNameIndexBySubstr(s, i, sp);
            if (i >= 0)
            {
                rets = Greek::getInstance()->abbrevs[i];
                rets += " ";
                ret.emplace_back(ReplaceGreekLetterAbbr(rets));
                i++;
            }
        }
    }
    else
    {
        std::string prefix = s.substr(0, sp);
        std::string rets = Greek::canonicalAbbreviation(prefix);
        if (!rets.empty())
        {
            rets += s.substr(sp);
            ret.emplace_back(ReplaceGreekLetterAbbr(rets));
        }
    }

    return ret;
}

bool
UTF8Validator::check(char c)
{
    return check(static_cast<unsigned char>(c));
}

bool
UTF8Validator::check(unsigned char c)
{
    switch (state)
    {
    case State::Initial:
        if (c < 0x80) { return true; }
        if (c < 0xc2) { return false; }
        if (c < 0xe0) { state = State::Continuation1; return true; }
        if (c == 0xe0) { state = State::E0Continuation; return true; }
        if (c < 0xed) { state = State::Continuation2; return true; }
        if (c== 0xed) { state = State::EDContinuation; return true; }
        if (c < 0xf0) { state = State::Continuation2; return true; }
        if (c == 0xf0) { state = State::F0Continuation; return true; }
        if (c < 0xf4) { state = State::Continuation3; return true; }
        if (c == 0xf4) { state = State::F4Continuation; return true; }
        return false;
    case State::Continuation1:
        if (c >= 0x80 && c < 0xc0) { state = State::Initial; return true; }
        break;
    case State::Continuation2:
        if (c >= 0x80 && c < 0xc0) { state = State::Continuation1; return true; }
        break;
    case State::Continuation3:
        if (c >= 0x80 && c < 0xc0) { state = State::Continuation2; return true; }
        break;
    case State::E0Continuation: // disallow overlong sequences
        if (c >= 0xa0 && c < 0xc0) { state = State::Continuation1; return true; }
        break;
    case State::EDContinuation: // disallow surrogate pairs
        if (c >= 0x80 && c < 0xa0) { state = State::Continuation1; return true; }
        break;
    case State::F0Continuation: // disallow overlong sequences
        if (c >= 0x90 && c < 0xc0) { state = State::Continuation2; return true; }
        break;
    case State::F4Continuation: // disallow out-of-range
        if (c >= 0x80 && c < 0x90) { state = State::Continuation2; return true; }
    }
    state = State::Initial;
    return false;
}
