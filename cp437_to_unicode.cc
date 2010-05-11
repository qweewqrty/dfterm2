#include "cp437_to_unicode.hpp"
#include <cstring>

unsigned int character_mappings[256];

int dfterm::mapCharacter(int character)
{
    if (character > 255) character = 255;
    if (character < 0) character = 0;
    character = character_mappings[character];
    if (character < 32) character = ' ';

    return character;
}

void dfterm::initCharacterMappings()
{
    /* The characters of Dwarf Fortress (as how they look like):
    0 - empty
    1 - civilian dwarf
    2 - military dwarf
    3 - heart
    4 - diamond
    5 - tree 1
    6 - tree 2
    7 - sphere
    8 - inverted sphere
    9 - ring
    10 - inverted ring
    11 - male symbol
    12 - female symbol
    13 - note
    14 - double note
    15 - star
    16 - triangle with flat side left
    17 - triangle with flat side right
    18 - arrow with both up and down directions
    19 - two exclamation marks
    20 - the thing that looks like mirrored P
    21 - §
    22 - wood log
    23 - arrow with both up and down directions and underlined
    24 - up arrow
    25 - down arrow
    26 - right arrow
    27 - left arrow
    28 - not sign
    29 - arrow with both left and right directions
    30 - triangle with flat side bottom
    31 - triangle with flat side top

    The characters are actually from IBM's CP437. This code maps them to unicode.
    */

    memset(character_mappings, 0, 256 * sizeof(unsigned int));

    int i1;
    // By default, just map 1:1
    for (i1 = 32; i1 < 127; i1++)
        character_mappings[i1] = i1;
    // Map other characters to '?'
    for (i1 = 0; i1 < 32; i1++)
        character_mappings[i1] = ' ';
    for (i1 = 127; i1 < 256; i1++)
        character_mappings[i1] = '?';
    // Remap characters below 32 to unicode characters.
    character_mappings[1] = 0x263A;
    character_mappings[2] = 0x263B;
    character_mappings[3] = 0x2665;
    character_mappings[4] = 0x2666;
    character_mappings[5] = 0x2663;
    character_mappings[6] = 0x2660;
    character_mappings[7] = 0x2022;
    character_mappings[8] = 0x25D8;
    character_mappings[9] = 0x25CB;
    character_mappings[10] = 0x25D9;
    character_mappings[11] = 0x2642;
    character_mappings[12] = 0x2640;
    character_mappings[13] = 0x266A;
    character_mappings[14] = 0x266B;
    character_mappings[15] = 0x263C;
    character_mappings[16] = 0x25BA;
    character_mappings[17] = 0x25C4;
    character_mappings[18] = 0x2195;
    character_mappings[19] = 0x203C;
    character_mappings[20] = 0x00B6;
    character_mappings[21] = 0x00A7;
    character_mappings[22] = 0x25AC;
    character_mappings[23] = 0x21A8;
    character_mappings[24] = 0x2191;
    character_mappings[25] = 0x2193;
    character_mappings[26] = 0x2192;
    character_mappings[27] = 0x2190;
    character_mappings[28] = 0x221F;
    character_mappings[29] = 0x2194;
    character_mappings[30] = 0x25B2;
    character_mappings[31] = 0x25BC;

    character_mappings[0x7f] =  0x2302  ;
    character_mappings[0x80]=	0x00c7	; //LATIN CAPITAL LETTER C WITH CEDILLA
    character_mappings[0x81]=	0x00fc	; //LATIN SMALL LETTER U WITH DIAERESIS
    character_mappings[0x82]=	0x00e9	; //LATIN SMALL LETTER E WITH ACUTE
    character_mappings[0x83]=	0x00e2	; //LATIN SMALL LETTER A WITH CIRCUMFLEX
    character_mappings[0x84]=	0x00e4	; //LATIN SMALL LETTER A WITH DIAERESIS
    character_mappings[0x85]=	0x00e0	; //LATIN SMALL LETTER A WITH GRAVE
    character_mappings[0x86]=	0x00e5	; //LATIN SMALL LETTER A WITH RING ABOVE
    character_mappings[0x87]=	0x00e7	; //LATIN SMALL LETTER C WITH CEDILLA
    character_mappings[0x88]=	0x00ea	; //LATIN SMALL LETTER E WITH CIRCUMFLEX
    character_mappings[0x89]=	0x00eb	; //LATIN SMALL LETTER E WITH DIAERESIS
    character_mappings[0x8a]=	0x00e8	; //LATIN SMALL LETTER E WITH GRAVE
    character_mappings[0x8b]=	0x00ef	; //LATIN SMALL LETTER I WITH DIAERESIS
    character_mappings[0x8c]=	0x00ee	; //LATIN SMALL LETTER I WITH CIRCUMFLEX
    character_mappings[0x8d]=	0x00ec	; //LATIN SMALL LETTER I WITH GRAVE
    character_mappings[0x8e]=	0x00c4	; //LATIN CAPITAL LETTER A WITH DIAERESIS
    character_mappings[0x8f]=	0x00c5	; //LATIN CAPITAL LETTER A WITH RING ABOVE
    character_mappings[0x90]=	0x00c9	; //LATIN CAPITAL LETTER E WITH ACUTE
    character_mappings[0x91]=	0x00e6	; //LATIN SMALL LIGATURE AE
    character_mappings[0x92]=	0x00c6	; //LATIN CAPITAL LIGATURE AE
    character_mappings[0x93]=	0x00f4	; //LATIN SMALL LETTER O WITH CIRCUMFLEX
    character_mappings[0x94]=	0x00f6	; //LATIN SMALL LETTER O WITH DIAERESIS
    character_mappings[0x95]=	0x00f2	; //LATIN SMALL LETTER O WITH GRAVE
    character_mappings[0x96]=	0x00fb	; //LATIN SMALL LETTER U WITH CIRCUMFLEX
    character_mappings[0x97]=	0x00f9	; //LATIN SMALL LETTER U WITH GRAVE
    character_mappings[0x98]=	0x00ff	; //LATIN SMALL LETTER Y WITH DIAERESIS
    character_mappings[0x99]=	0x00d6	; //LATIN CAPITAL LETTER O WITH DIAERESIS
    character_mappings[0x9a]=	0x00dc	; //LATIN CAPITAL LETTER U WITH DIAERESIS
    character_mappings[0x9b]=	0x00a2	; //CENT SIGN
    character_mappings[0x9c]=	0x00a3	; //POUND SIGN
    character_mappings[0x9d]=	0x00a5	; //YEN SIGN
    character_mappings[0x9e]=	0x20a7	; //PESETA SIGN
    character_mappings[0x9f]=	0x0192	; //LATIN SMALL LETTER F WITH HOOK
    character_mappings[0xa0]=	0x00e1	; //LATIN SMALL LETTER A WITH ACUTE
    character_mappings[0xa1]=	0x00ed	; //LATIN SMALL LETTER I WITH ACUTE
    character_mappings[0xa2]=	0x00f3	; //LATIN SMALL LETTER O WITH ACUTE
    character_mappings[0xa3]=	0x00fa	; //LATIN SMALL LETTER U WITH ACUTE
    character_mappings[0xa4]=	0x00f1	; //LATIN SMALL LETTER N WITH TILDE
    character_mappings[0xa5]=	0x00d1	; //LATIN CAPITAL LETTER N WITH TILDE
    character_mappings[0xa6]=	0x00aa	; //FEMININE ORDINAL INDICATOR
    character_mappings[0xa7]=	0x00ba	; //MASCULINE ORDINAL INDICATOR
    character_mappings[0xa8]=	0x00bf	; //INVERTED QUESTION MARK
    character_mappings[0xa9]=	0x2310	; //REVERSED NOT SIGN
    character_mappings[0xaa]=	0x00ac	; //NOT SIGN
    character_mappings[0xab]=	0x00bd	; //VULGAR FRACTION ONE HALF
    character_mappings[0xac]=	0x00bc	; //VULGAR FRACTION ONE QUARTER
    character_mappings[0xad]=	0x00a1	; //INVERTED EXCLAMATION MARK
    character_mappings[0xae]=	0x00ab	; //LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    character_mappings[0xaf]=	0x00bb	; //RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    character_mappings[0xb0]=	0x2591	; //LIGHT SHADE
    character_mappings[0xb1]=	0x2592	; //MEDIUM SHADE
    character_mappings[0xb2]=	0x2593	; //DARK SHADE
    character_mappings[0xb3]=	0x2502	; //BOX DRAWINGS LIGHT VERTICAL
    character_mappings[0xb4]=	0x2524	; //BOX DRAWINGS LIGHT VERTICAL AND LEFT
    character_mappings[0xb5]=	0x2561	; //BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
    character_mappings[0xb6]=	0x2562	; //BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
    character_mappings[0xb7]=	0x2556	; //BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
    character_mappings[0xb8]=	0x2555	; //BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
    character_mappings[0xb9]=	0x2563	; //BOX DRAWINGS DOUBLE VERTICAL AND LEFT
    character_mappings[0xba]=	0x2551	; //BOX DRAWINGS DOUBLE VERTICAL
    character_mappings[0xbb]=	0x2557	; //BOX DRAWINGS DOUBLE DOWN AND LEFT
    character_mappings[0xbc]=	0x255d	; //BOX DRAWINGS DOUBLE UP AND LEFT
    character_mappings[0xbd]=	0x255c	; //BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
    character_mappings[0xbe]=	0x255b	; //BOX DRAWINGS UP SINGLE AND LEFT DOUBLE
    character_mappings[0xbf]=	0x2510	; //BOX DRAWINGS LIGHT DOWN AND LEFT
    character_mappings[0xc0]=	0x2514	; //BOX DRAWINGS LIGHT UP AND RIGHT
    character_mappings[0xc1]=	0x2534	; //BOX DRAWINGS LIGHT UP AND HORIZONTAL
    character_mappings[0xc2]=	0x252c	; //BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
    character_mappings[0xc3]=	0x251c	; //BOX DRAWINGS LIGHT VERTICAL AND RIGHT
    character_mappings[0xc4]=	0x2500	; //BOX DRAWINGS LIGHT HORIZONTAL
    character_mappings[0xc5]=	0x253c	; //BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
    character_mappings[0xc6]=	0x255e	; //BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
    character_mappings[0xc7]=	0x255f	; //BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
    character_mappings[0xc8]=	0x255a	; //BOX DRAWINGS DOUBLE UP AND RIGHT
    character_mappings[0xc9]=	0x2554	; //BOX DRAWINGS DOUBLE DOWN AND RIGHT
    character_mappings[0xca]=	0x2569	; //BOX DRAWINGS DOUBLE UP AND HORIZONTAL
    character_mappings[0xcb]=	0x2566	; //BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
    character_mappings[0xcc]=	0x2560	; //BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
    character_mappings[0xcd]=	0x2550	; //BOX DRAWINGS DOUBLE HORIZONTAL
    character_mappings[0xce]=	0x256c	; //BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
    character_mappings[0xcf]=	0x2567	; //BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
    character_mappings[0xd0]=	0x2568	; //BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
    character_mappings[0xd1]=	0x2564	; //BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
    character_mappings[0xd2]=	0x2565	; //BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
    character_mappings[0xd3]=	0x2559	; //BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
    character_mappings[0xd4]=	0x2558	; //BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
    character_mappings[0xd5]=	0x2552	; //BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
    character_mappings[0xd6]=	0x2553	; //BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
    character_mappings[0xd7]=	0x256b	; //BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
    character_mappings[0xd8]=	0x256a	; //BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
    character_mappings[0xd9]=	0x2518	; //BOX DRAWINGS LIGHT UP AND LEFT
    character_mappings[0xda]=	0x250c	; //BOX DRAWINGS LIGHT DOWN AND RIGHT
    character_mappings[0xdb]=	0x2588	; //FULL BLOCK
    character_mappings[0xdc]=	0x2584	; //LOWER HALF BLOCK
    character_mappings[0xdd]=	0x258c	; //LEFT HALF BLOCK
    character_mappings[0xde]=	0x2590	; //RIGHT HALF BLOCK
    character_mappings[0xdf]=	0x2580	; //UPPER HALF BLOCK
    character_mappings[0xe0]=	0x03b1	; //GREEK SMALL LETTER ALPHA
    character_mappings[0xe1]=	0x00df	; //LATIN SMALL LETTER SHARP S
    character_mappings[0xe2]=	0x0393	; //GREEK CAPITAL LETTER GAMMA
    character_mappings[0xe3]=	0x03c0	; //GREEK SMALL LETTER PI
    character_mappings[0xe4]=	0x03a3	; //GREEK CAPITAL LETTER SIGMA
    character_mappings[0xe5]=	0x03c3	; //GREEK SMALL LETTER SIGMA
    character_mappings[0xe6]=	0x00b5	; //MICRO SIGN
    character_mappings[0xe7]=	0x03c4	; //GREEK SMALL LETTER TAU
    character_mappings[0xe8]=	0x03a6	; //GREEK CAPITAL LETTER PHI
    character_mappings[0xe9]=	0x0398	; //GREEK CAPITAL LETTER THETA
    character_mappings[0xea]=	0x03a9	; //GREEK CAPITAL LETTER OMEGA
    character_mappings[0xeb]=	0x03b4	; //GREEK SMALL LETTER DELTA
    character_mappings[0xec]=	0x221e	; //INFINITY
    character_mappings[0xed]=	0x03c6	; //GREEK SMALL LETTER PHI
    character_mappings[0xee]=	0x03b5	; //GREEK SMALL LETTER EPSILON
    character_mappings[0xef]=	0x2229	; //INTERSECTION
    character_mappings[0xf0]=	0x2261	; //IDENTICAL TO
    character_mappings[0xf1]=	0x00b1	; //PLUS-MINUS SIGN
    character_mappings[0xf2]=	0x2265	; //GREATER-THAN OR EQUAL TO
    character_mappings[0xf3]=	0x2264	; //LESS-THAN OR EQUAL TO
    character_mappings[0xf4]=	0x2320	; //TOP HALF INTEGRAL
    character_mappings[0xf5]=	0x2321	; //BOTTOM HALF INTEGRAL
    character_mappings[0xf6]=	0x00f7	; //DIVISION SIGN
    character_mappings[0xf7]=	0x2248	; //ALMOST EQUAL TO
    character_mappings[0xf8]=	0x00b0	; //DEGREE SIGN
    character_mappings[0xf9]=	0x2219	; //BULLET OPERATOR
    character_mappings[0xfa]=	0x00b7	; //MIDDLE DOT
    character_mappings[0xfb]=	0x221a	; //SQUARE ROOT
    character_mappings[0xfc]=	0x207f	; //SUPERSCRIPT LATIN SMALL LETTER N
    character_mappings[0xfd]=	0x00b2	; //SUPERSCRIPT TWO
    character_mappings[0xfe]=	0x25a0	; //BLACK SQUARE
    character_mappings[0xff]=	0x00a0	; //NO-BREAK SPACE
}

