/* sqUnixSdl2Window.c -- support for display via your sdl2 window system.
 * 
 * Last edited: 2006-04-17 16:57:12 by piumarta on margaux.local
 * 
 * This is a template for creating your own window drivers for Squeak:
 * 
 *   - copy the entire contents of this directory to some other name
 *   - rename this file to be something more appropriate
 *   - modify acinclude.m4, Makefile.in, and ../vm/sqUnixMain accordingly
 *   - implement all the stubs in this file that currently do nothing
 * 
 */

#include "sq.h"
#include "sqMemoryAccess.h"
#include "sqaio.h"

#include "sqUnixMain.h"
#include "sqUnixGlobals.h"
#include "sqUnixCharConv.h"		/* not required, but probably useful */
#include "aio.h"			/* ditto */

#include "SqDisplay.h"

#include <stdio.h>
#include <SDL2/SDL.h>

#include "sqUnixEvent.c"		/* see X11 and/or Quartz drivers for examples */

#include "common.h"

SDL_Window *window;
ApplicationState state;

#define trace() fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__)

int sdl_button_to_sq(int button) {
  switch (button) {
    case SDL_BUTTON_LEFT:
      return RedButtonBit;
    case SDL_BUTTON_RIGHT:
      return BlueButtonBit;
    case SDL_BUTTON_MIDDLE:
      return YellowButtonBit;
    case SDL_BUTTON_X1:
    case SDL_BUTTON_X2:
      return 0;
  }
  return 0;
}

int sdl_modifiers_to_sq(SDL_Keymod mod) {
  return
    (mod & KMOD_SHIFT ? ShiftKeyBit : 0) |
    (mod & KMOD_CTRL ? CtrlKeyBit : 0) |
    (mod & KMOD_ALT ? CommandKeyBit : 0) |
    (mod & KMOD_GUI ? OptionKeyBit : 0);
}

int current_modifiers() {
    return sdl_modifiers_to_sq(SDL_GetModState());
}

static int xkeysym2ucs4(unsigned short keysym)
{
    /* Latin 2 Mappings */
  static unsigned short const ucs4_01a1_01ff[] = {
            0x0104, 0x02d8, 0x0141, 0x0000, 0x013d, 0x015a, 0x0000, /* 0x01a0-0x01a7 */
    0x0000, 0x0160, 0x015e, 0x0164, 0x0179, 0x0000, 0x017d, 0x017b, /* 0x01a8-0x01af */
    0x0000, 0x0105, 0x02db, 0x0142, 0x0000, 0x013e, 0x015b, 0x02c7, /* 0x01b0-0x01b7 */
    0x0000, 0x0161, 0x015f, 0x0165, 0x017a, 0x02dd, 0x017e, 0x017c, /* 0x01b8-0x01bf */
    0x0154, 0x0000, 0x0000, 0x0102, 0x0000, 0x0139, 0x0106, 0x0000, /* 0x01c0-0x01c7 */
    0x010c, 0x0000, 0x0118, 0x0000, 0x011a, 0x0000, 0x0000, 0x010e, /* 0x01c8-0x01cf */
    0x0110, 0x0143, 0x0147, 0x0000, 0x0000, 0x0150, 0x0000, 0x0000, /* 0x01d0-0x01d7 */
    0x0158, 0x016e, 0x0000, 0x0170, 0x0000, 0x0000, 0x0162, 0x0000, /* 0x01d8-0x01df */
    0x0155, 0x0000, 0x0000, 0x0103, 0x0000, 0x013a, 0x0107, 0x0000, /* 0x01e0-0x01e7 */
    0x010d, 0x0000, 0x0119, 0x0000, 0x011b, 0x0000, 0x0000, 0x010f, /* 0x01e8-0x01ef */
    0x0111, 0x0144, 0x0148, 0x0000, 0x0000, 0x0151, 0x0000, 0x0000, /* 0x01f0-0x01f7 */
    0x0159, 0x016f, 0x0000, 0x0171, 0x0000, 0x0000, 0x0163, 0x02d9  /* 0x01f8-0x01ff */
  };

    /* Latin 3 Mappings */
  static unsigned short const ucs4_02a1_02fe[] = {
            0x0126, 0x0000, 0x0000, 0x0000, 0x0000, 0x0124, 0x0000, /* 0x02a0-0x02a7 */
    0x0000, 0x0130, 0x0000, 0x011e, 0x0134, 0x0000, 0x0000, 0x0000, /* 0x02a8-0x02af */
    0x0000, 0x0127, 0x0000, 0x0000, 0x0000, 0x0000, 0x0125, 0x0000, /* 0x02b0-0x02b7 */
    0x0000, 0x0131, 0x0000, 0x011f, 0x0135, 0x0000, 0x0000, 0x0000, /* 0x02b8-0x02bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x010a, 0x0108, 0x0000, /* 0x02c0-0x02c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x02c8-0x02cf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0120, 0x0000, 0x0000, /* 0x02d0-0x02d7 */
    0x011c, 0x0000, 0x0000, 0x0000, 0x0000, 0x016c, 0x015c, 0x0000, /* 0x02d8-0x02df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x010b, 0x0109, 0x0000, /* 0x02e0-0x02e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x02e8-0x02ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0121, 0x0000, 0x0000, /* 0x02f0-0x02f7 */
    0x011d, 0x0000, 0x0000, 0x0000, 0x0000, 0x016d, 0x015d          /* 0x02f8-0x02ff */
  };

    /* Latin 4 Mappings */
  static unsigned short const ucs4_03a2_03fe[] = {
                    0x0138, 0x0156, 0x0000, 0x0128, 0x013b, 0x0000, /* 0x03a0-0x03a7 */
    0x0000, 0x0000, 0x0112, 0x0122, 0x0166, 0x0000, 0x0000, 0x0000, /* 0x03a8-0x03af */
    0x0000, 0x0000, 0x0000, 0x0157, 0x0000, 0x0129, 0x013c, 0x0000, /* 0x03b0-0x03b7 */
    0x0000, 0x0000, 0x0113, 0x0123, 0x0167, 0x014a, 0x0000, 0x014b, /* 0x03b8-0x03bf */
    0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012e, /* 0x03c0-0x03c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0116, 0x0000, 0x0000, 0x012a, /* 0x03c8-0x03cf */
    0x0000, 0x0145, 0x014c, 0x0136, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x03d0-0x03d7 */
    0x0000, 0x0172, 0x0000, 0x0000, 0x0000, 0x0168, 0x016a, 0x0000, /* 0x03d8-0x03df */
    0x0101, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012f, /* 0x03e0-0x03e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0117, 0x0000, 0x0000, 0x012b, /* 0x03e8-0x03ef */
    0x0000, 0x0146, 0x014d, 0x0137, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x03f0-0x03f7 */
    0x0000, 0x0173, 0x0000, 0x0000, 0x0000, 0x0169, 0x016b          /* 0x03f8-0x03ff */
  };

    /* Katakana Mappings */
  static unsigned short const ucs4_04a1_04df[] = {
            0x3002, 0x3008, 0x3009, 0x3001, 0x30fb, 0x30f2, 0x30a1, /* 0x04a0-0x04a7 */
    0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7, 0x30c3, /* 0x04a8-0x04af */
    0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa, 0x30ab, 0x30ad, /* 0x04b0-0x04b7 */
    0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd, /* 0x04b8-0x04bf */
    0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, 0x30ca, 0x30cb, 0x30cc, /* 0x04c0-0x04c7 */
    0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de, /* 0x04c8-0x04cf */
    0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9, /* 0x04d0-0x04d7 */
    0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c  /* 0x04d8-0x04df */
  };

    /* Arabic mappings */
  static unsigned short const ucs4_0590_05fe[] = {
    0x06f0, 0x06f1, 0x06f2, 0x06f3, 0x06f4, 0x06f5, 0x06f6, 0x06f7, /* 0x0590-0x0597 */
    0x06f8, 0x06f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x0598-0x059f */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x066a, 0x0670, 0x0679, /* 0x05a0-0x05a7 */
	
    0x067e, 0x0686, 0x0688, 0x0691, 0x060c, 0x0000, 0x06d4, 0x0000, /* 0x05ac-0x05af */
    0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, /* 0x05b0-0x05b7 */
    0x0668, 0x0669, 0x0000, 0x061b, 0x0000, 0x0000, 0x0000, 0x061f, /* 0x05b8-0x05bf */
    0x0000, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, /* 0x05c0-0x05c7 */
    0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e, 0x062f, /* 0x05c8-0x05cf */
    0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, /* 0x05d0-0x05d7 */
    0x0638, 0x0639, 0x063a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x05d8-0x05df */
    0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, /* 0x05e0-0x05e7 */
    0x0648, 0x0649, 0x064a, 0x064b, 0x064c, 0x064d, 0x064e, 0x064f, /* 0x05e8-0x05ef */
    0x0650, 0x0651, 0x0652, 0x0653, 0x0654, 0x0655, 0x0698, 0x06a4, /* 0x05f0-0x05f7 */
    0x06a9, 0x06af, 0x06ba, 0x06be, 0x06cc, 0x06d2, 0x06c1          /* 0x05f8-0x05fe */
  };

    /* Cyrillic mappings */
  static unsigned short ucs4_0680_06ff[] = {
    0x0492, 0x0496, 0x049a, 0x049c, 0x04a2, 0x04ae, 0x04b0, 0x04b2, /* 0x0680-0x0687 */
    0x04b6, 0x04b8, 0x04ba, 0x0000, 0x04d8, 0x04e2, 0x04e8, 0x04ee, /* 0x0688-0x068f */
    0x0493, 0x0497, 0x049b, 0x049d, 0x04a3, 0x04af, 0x04b1, 0x04b3, /* 0x0690-0x0697 */
    0x04b7, 0x04b9, 0x04bb, 0x0000, 0x04d9, 0x04e3, 0x04e9, 0x04ef, /* 0x0698-0x069f */
    0x0000, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457, /* 0x06a0-0x06a7 */
    0x0458, 0x0459, 0x045a, 0x045b, 0x045c, 0x0491, 0x045e, 0x045f, /* 0x06a8-0x06af */
    0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407, /* 0x06b0-0x06b7 */
    0x0408, 0x0409, 0x040a, 0x040b, 0x040c, 0x0490, 0x040e, 0x040f, /* 0x06b8-0x06bf */
    0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, /* 0x06c0-0x06c7 */
    0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, /* 0x06c8-0x06cf */
    0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, /* 0x06d0-0x06d7 */
    0x044c, 0x044b, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044a, /* 0x06d8-0x06df */
    0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, /* 0x06e0-0x06e7 */
    0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, /* 0x06e8-0x06ef */
    0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, /* 0x06f0-0x06f7 */
    0x042c, 0x042b, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042a  /* 0x06f8-0x06ff */
  };

    /* Greek mappings */
  static unsigned short const ucs4_07a1_07f9[] = {
            0x0386, 0x0388, 0x0389, 0x038a, 0x03aa, 0x0000, 0x038c, /* 0x07a0-0x07a7 */
    0x038e, 0x03ab, 0x0000, 0x038f, 0x0000, 0x0000, 0x0385, 0x2015, /* 0x07a8-0x07af */
    0x0000, 0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03ca, 0x0390, 0x03cc, /* 0x07b0-0x07b7 */
    0x03cd, 0x03cb, 0x03b0, 0x03ce, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x07b8-0x07bf */
    0x0000, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, /* 0x07c0-0x07c7 */
    0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f, /* 0x07c8-0x07cf */
    0x03a0, 0x03a1, 0x03a3, 0x0000, 0x03a4, 0x03a5, 0x03a6, 0x03a7, /* 0x07d0-0x07d7 */
    0x03a8, 0x03a9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x07d8-0x07df */
    0x0000, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, /* 0x07e0-0x07e7 */
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, /* 0x07e8-0x07ef */
    0x03c0, 0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7, /* 0x07f0-0x07f7 */
    0x03c8, 0x03c9                                                  /* 0x07f8-0x07ff */
  };

    /* Technical mappings */
  static unsigned short const ucs4_08a4_08fe[] = {
                                    0x2320, 0x2321, 0x0000, 0x231c, /* 0x08a0-0x08a7 */
    0x231d, 0x231e, 0x231f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08a8-0x08af */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08b0-0x08b7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x2264, 0x2260, 0x2265, 0x222b, /* 0x08b8-0x08bf */
    0x2234, 0x0000, 0x221e, 0x0000, 0x0000, 0x2207, 0x0000, 0x0000, /* 0x08c0-0x08c7 */
    0x2245, 0x2246, 0x0000, 0x0000, 0x0000, 0x0000, 0x22a2, 0x0000, /* 0x08c8-0x08cf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x221a, 0x0000, /* 0x08d0-0x08d7 */
    0x0000, 0x0000, 0x2282, 0x2283, 0x2229, 0x222a, 0x2227, 0x2228, /* 0x08d8-0x08df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08e0-0x08e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08e8-0x08ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0192, 0x0000, /* 0x08f0-0x08f7 */
    0x0000, 0x0000, 0x0000, 0x2190, 0x2191, 0x2192, 0x2193          /* 0x08f8-0x08ff */
  };

    /* Special mappings from the DEC VT100 Special Graphics Character Set */
  static unsigned short const ucs4_09df_09f8[] = {
                                                            0x2422, /* 0x09d8-0x09df */
    0x2666, 0x25a6, 0x2409, 0x240c, 0x240d, 0x240a, 0x0000, 0x0000, /* 0x09e0-0x09e7 */
    0x240a, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0x2500, /* 0x09e8-0x09ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x251c, 0x2524, 0x2534, 0x252c, /* 0x09f0-0x09f7 */
    0x2502                                                          /* 0x09f8-0x09ff */
  };

    /* Publishing Mappings ? */
  static unsigned short const ucs4_0aa1_0afe[] = {
            0x2003, 0x2002, 0x2004, 0x2005, 0x2007, 0x2008, 0x2009, /* 0x0aa0-0x0aa7 */
    0x200a, 0x2014, 0x2013, 0x0000, 0x0000, 0x0000, 0x2026, 0x2025, /* 0x0aa8-0x0aaf */
    0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215a, /* 0x0ab0-0x0ab7 */
    0x2105, 0x0000, 0x0000, 0x2012, 0x2039, 0x2024, 0x203a, 0x0000, /* 0x0ab8-0x0abf */
    0x0000, 0x0000, 0x0000, 0x215b, 0x215c, 0x215d, 0x215e, 0x0000, /* 0x0ac0-0x0ac7 */
    0x0000, 0x2122, 0x2120, 0x0000, 0x25c1, 0x25b7, 0x25cb, 0x25ad, /* 0x0ac8-0x0acf */
    0x2018, 0x2019, 0x201c, 0x201d, 0x211e, 0x0000, 0x2032, 0x2033, /* 0x0ad0-0x0ad7 */
    0x0000, 0x271d, 0x0000, 0x220e, 0x25c2, 0x2023, 0x25cf, 0x25ac, /* 0x0ad8-0x0adf */
    0x25e6, 0x25ab, 0x25ae, 0x25b5, 0x25bf, 0x2606, 0x2022, 0x25aa, /* 0x0ae0-0x0ae7 */
    0x25b4, 0x25be, 0x261a, 0x261b, 0x2663, 0x2666, 0x2665, 0x0000, /* 0x0ae8-0x0aef */
    0x2720, 0x2020, 0x2021, 0x2713, 0x2612, 0x266f, 0x266d, 0x2642, /* 0x0af0-0x0af7 */
    0x2640, 0x2121, 0x2315, 0x2117, 0x2038, 0x201a, 0x201e          /* 0x0af8-0x0aff */
  };

    /* Hebrew Mappings */
  static unsigned short const ucs4_0cdf_0cfa[] = {
                                                            0x2017, /* 0x0cd8-0x0cdf */
    0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, /* 0x0ce0-0x0ce7 */
    0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df, /* 0x0ce8-0x0cef */
    0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, /* 0x0cf0-0x0cf7 */
    0x05e8, 0x05e9, 0x05ea                                          /* 0x0cf8-0x0cff */
  };

    /* Thai Mappings */
  static unsigned short const ucs4_0da1_0df9[] = {
            0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07, /* 0x0da0-0x0da7 */
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f, /* 0x0da8-0x0daf */
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17, /* 0x0db0-0x0db7 */
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f, /* 0x0db8-0x0dbf */
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27, /* 0x0dc0-0x0dc7 */
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f, /* 0x0dc8-0x0dcf */
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37, /* 0x0dd0-0x0dd7 */
    0x0e38, 0x0e39, 0x0e3a, 0x0000, 0x0000, 0x0000, 0x0e3e, 0x0e3f, /* 0x0dd8-0x0ddf */
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47, /* 0x0de0-0x0de7 */
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0000, 0x0000, /* 0x0de8-0x0def */
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57, /* 0x0df0-0x0df7 */
    0x0e58, 0x0e59                                                  /* 0x0df8-0x0dff */
  };

    /* Hangul Mappings */
  static unsigned short const ucs4_0ea0_0eff[] = {
    0x0000, 0x1101, 0x1101, 0x11aa, 0x1102, 0x11ac, 0x11ad, 0x1103, /* 0x0ea0-0x0ea7 */
    0x1104, 0x1105, 0x11b0, 0x11b1, 0x11b2, 0x11b3, 0x11b4, 0x11b5, /* 0x0ea8-0x0eaf */
    0x11b6, 0x1106, 0x1107, 0x1108, 0x11b9, 0x1109, 0x110a, 0x110b, /* 0x0eb0-0x0eb7 */
    0x110c, 0x110d, 0x110e, 0x110f, 0x1110, 0x1111, 0x1112, 0x1161, /* 0x0eb8-0x0ebf */
    0x1162, 0x1163, 0x1164, 0x1165, 0x1166, 0x1167, 0x1168, 0x1169, /* 0x0ec0-0x0ec7 */
    0x116a, 0x116b, 0x116c, 0x116d, 0x116e, 0x116f, 0x1170, 0x1171, /* 0x0ec8-0x0ecf */
    0x1172, 0x1173, 0x1174, 0x1175, 0x11a8, 0x11a9, 0x11aa, 0x11ab, /* 0x0ed0-0x0ed7 */
    0x11ac, 0x11ad, 0x11ae, 0x11af, 0x11b0, 0x11b1, 0x11b2, 0x11b3, /* 0x0ed8-0x0edf */
    0x11b4, 0x11b5, 0x11b6, 0x11b7, 0x11b8, 0x11b9, 0x11ba, 0x11bb, /* 0x0ee0-0x0ee7 */
    0x11bc, 0x11bd, 0x11be, 0x11bf, 0x11c0, 0x11c1, 0x11c2, 0x0000, /* 0x0ee8-0x0eef */
    0x0000, 0x0000, 0x1140, 0x0000, 0x0000, 0x1159, 0x119e, 0x0000, /* 0x0ef0-0x0ef7 */
    0x11eb, 0x0000, 0x11f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x20a9, /* 0x0ef8-0x0eff */
  };

    /* Non existing range in keysymdef.h */
  static unsigned short ucs4_12a1_12fe[] = {
            0x1e02, 0x1e03, 0x0000, 0x0000, 0x0000, 0x1e0a, 0x0000, /* 0x12a0-0x12a7 */
    0x1e80, 0x0000, 0x1e82, 0x1e0b, 0x1ef2, 0x0000, 0x0000, 0x0000, /* 0x12a8-0x12af */
    0x1e1e, 0x1e1f, 0x0000, 0x0000, 0x1e40, 0x1e41, 0x0000, 0x1e56, /* 0x12b0-0x12b7 */
    0x1e81, 0x1e57, 0x1e83, 0x1e60, 0x1ef3, 0x1e84, 0x1e85, 0x1e61, /* 0x12b8-0x12bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12c0-0x12c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12c8-0x12cf */
    0x0174, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e6a, /* 0x12d0-0x12d7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0176, 0x0000, /* 0x12d8-0x12df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12e0-0x12e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12e8-0x12ef */
    0x0175, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e6b, /* 0x12f0-0x12f7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0177          /* 0x12f0-0x12ff */
  };
    
    /* Latin 9 Mappings */		
  static unsigned short const ucs4_13bc_13be[] = {
                                    0x0152, 0x0153, 0x0178          /* 0x13b8-0x13bf */
  };

    /* Non existing range in keysymdef.h */
  static unsigned short ucs4_14a1_14ff[] = {
            0x2741, 0x00a7, 0x0589, 0x0029, 0x0028, 0x00bb, 0x00ab, /* 0x14a0-0x14a7 */
    0x2014, 0x002e, 0x055d, 0x002c, 0x2013, 0x058a, 0x2026, 0x055c, /* 0x14a8-0x14af */
    0x055b, 0x055e, 0x0531, 0x0561, 0x0532, 0x0562, 0x0533, 0x0563, /* 0x14b0-0x14b7 */
    0x0534, 0x0564, 0x0535, 0x0565, 0x0536, 0x0566, 0x0537, 0x0567, /* 0x14b8-0x14bf */
    0x0538, 0x0568, 0x0539, 0x0569, 0x053a, 0x056a, 0x053b, 0x056b, /* 0x14c0-0x14c7 */
    0x053c, 0x056c, 0x053d, 0x056d, 0x053e, 0x056e, 0x053f, 0x056f, /* 0x14c8-0x14cf */
    0x0540, 0x0570, 0x0541, 0x0571, 0x0542, 0x0572, 0x0543, 0x0573, /* 0x14d0-0x14d7 */
    0x0544, 0x0574, 0x0545, 0x0575, 0x0546, 0x0576, 0x0547, 0x0577, /* 0x14d8-0x14df */
    0x0548, 0x0578, 0x0549, 0x0579, 0x054a, 0x057a, 0x054b, 0x057b, /* 0x14e0-0x14e7 */
    0x054c, 0x057c, 0x054d, 0x057d, 0x054e, 0x057e, 0x054f, 0x057f, /* 0x14e8-0x14ef */
    0x0550, 0x0580, 0x0551, 0x0581, 0x0552, 0x0582, 0x0553, 0x0583, /* 0x14f0-0x14f7 */
    0x0554, 0x0584, 0x0555, 0x0585, 0x0556, 0x0586, 0x2019, 0x0027, /* 0x14f8-0x14ff */
  };

    /* Non existing range in keysymdef.h */
  static unsigned short ucs4_15d0_15f6[] = {
    0x10d0, 0x10d1, 0x10d2, 0x10d3, 0x10d4, 0x10d5, 0x10d6, 0x10d7, /* 0x15d0-0x15d7 */
    0x10d8, 0x10d9, 0x10da, 0x10db, 0x10dc, 0x10dd, 0x10de, 0x10df, /* 0x15d8-0x15df */
    0x10e0, 0x10e1, 0x10e2, 0x10e3, 0x10e4, 0x10e5, 0x10e6, 0x10e7, /* 0x15e0-0x15e7 */
    0x10e8, 0x10e9, 0x10ea, 0x10eb, 0x10ec, 0x10ed, 0x10ee, 0x10ef, /* 0x15e8-0x15ef */
    0x10f0, 0x10f1, 0x10f2, 0x10f3, 0x10f4, 0x10f5, 0x10f6          /* 0x15f0-0x15f7 */
  };

    /* Non existing range in keysymdef.h */
  static unsigned short ucs4_16a0_16f6[] = {
    0x0000, 0x0000, 0xf0a2, 0x1e8a, 0x0000, 0xf0a5, 0x012c, 0xf0a7, /* 0x16a0-0x16a7 */
    0xf0a8, 0x01b5, 0x01e6, 0x0000, 0x0000, 0x0000, 0x0000, 0x019f, /* 0x16a8-0x16af */
    0x0000, 0x017e, 0xf0b2, 0x1e8b, 0x01d1, 0xf0b5, 0x012d, 0xf0b7, /* 0x16b0-0x16b7 */
    0xf0b8, 0x01b6, 0x01e7, 0x01d2, 0x0000, 0x0000, 0x0000, 0x0275, /* 0x16b8-0x16bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x018f, 0x0000, /* 0x16c0-0x16c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16c8-0x16cf */
    0x0000, 0x1e36, 0xf0d2, 0xf0d3, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16d0-0x16d7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16d8-0x16df */
    0x0000, 0x1e37, 0xf0e2, 0xf0e3, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16e0-0x16e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16e8-0x16ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0259          /* 0x16f0-0x16f6 */
  };

    /* Vietnamesse Mappings */
  static unsigned short const ucs4_1e9f_1eff[] = {
                                                            0x0303,
    0x1ea0, 0x1ea1, 0x1ea2, 0x1ea3, 0x1ea4, 0x1ea5, 0x1ea6, 0x1ea7, /* 0x1ea0-0x1ea7 */
    0x1ea8, 0x1ea9, 0x1eaa, 0x1eab, 0x1eac, 0x1ead, 0x1eae, 0x1eaf, /* 0x1ea8-0x1eaf */
    0x1eb0, 0x1eb1, 0x1eb2, 0x1eb3, 0x1eb4, 0x1eb5, 0x1eb6, 0x1eb7, /* 0x1eb0-0x1eb7 */
    0x1eb8, 0x1eb9, 0x1eba, 0x1ebb, 0x1ebc, 0x1ebd, 0x1ebe, 0x1ebf, /* 0x1eb8-0x1ebf */
    0x1ec0, 0x1ec1, 0x1ec2, 0x1ec3, 0x1ec4, 0x1ec5, 0x1ec6, 0x1ec7, /* 0x1ec0-0x1ec7 */
    0x1ec8, 0x1ec9, 0x1eca, 0x1ecb, 0x1ecc, 0x1ecd, 0x1ece, 0x1ecf, /* 0x1ec8-0x1ecf */
    0x1ed0, 0x1ed1, 0x1ed2, 0x1ed3, 0x1ed4, 0x1ed5, 0x1ed6, 0x1ed7, /* 0x1ed0-0x1ed7 */
    0x1ed8, 0x1ed9, 0x1eda, 0x1edb, 0x1edc, 0x1edd, 0x1ede, 0x1edf, /* 0x1ed8-0x1edf */
    0x1ee0, 0x1ee1, 0x1ee2, 0x1ee3, 0x1ee4, 0x1ee5, 0x1ee6, 0x1ee7, /* 0x1ee0-0x1ee7 */
    0x1ee8, 0x1ee9, 0x1eea, 0x1eeb, 0x1eec, 0x1eed, 0x1eee, 0x1eef, /* 0x1ee8-0x1eef */
    0x1ef0, 0x1ef1, 0x0300, 0x0301, 0x1ef4, 0x1ef5, 0x1ef6, 0x1ef7, /* 0x1ef0-0x1ef7 */
    0x1ef8, 0x1ef9, 0x01a0, 0x01a1, 0x01af, 0x01b0, 0x0309, 0x0323  /* 0x1ef8-0x1eff */
  };

    /* Currency */
  static unsigned short const ucs4_20a0_20ac[] = {
    0x20a0, 0x20a1, 0x20a2, 0x20a3, 0x20a4, 0x20a5, 0x20a6, 0x20a7, /* 0x20a0-0x20a7 */
    0x20a8, 0x20a9, 0x20aa, 0x20ab, 0x20ac                          /* 0x20a8-0x20af */
  };

    /* Keypad numbers mapping */
  static unsigned short const ucs4_ffb0_ffb9[] = { 0x30, 0x31, 0x32, 0x33, 0x34,
             0x35, 0x36, 0x37, 0x38, 0x39};
 
     /* Keypad operators mapping */
  static unsigned short const ucs4_ffaa_ffaf[] = { 
             0x2a, /* Multiply  */
             0x2b, /* Add       */
             0x2c, /* Separator */
             0x2d, /* Substract */
             0x2e, /* Decimal   */
             0x2f  /* Divide    */
     };

 	static unsigned short const sqSpecialKey[] = { 
             1,  /* HOME  */
             28, /* LEFT  */ 
             30, /* UP    */
             29, /* RIGHT */
             31, /* DOWN  */
             11, /* PRIOR (page up?) */
             12, /* NEXT (page down/new page?) */
             4,  /* END */
             1   /* HOME */
     };


  /* Latin-1 */
  if (   (keysym >= 0x0020 && keysym <= 0x007e)
      || (keysym >= 0x00a0 && keysym <= 0x00ff)) return keysym;

  /* 24-bit UCS */
  if ((keysym & 0xff000000) == 0x01000000) return keysym & 0x00ffffff;

  /* control keys with ASCII equivalents */
  if (keysym > 0xff00 && keysym < 0xff10) return keysym & 0x001f;
  if (keysym > 0xff4f && keysym < 0xff59)
    {
      return sqSpecialKey[keysym - 0xff50];
    }
  if (keysym > 0xff58 && keysym < 0xff5f) return keysym & 0x007f; /* could be return 0; */
  if (keysym > 0xff94 && keysym < 0xff9d)
    {
      return sqSpecialKey[keysym - 0xff95];
    }
  if (keysym          ==          0xff1b) return keysym & 0x001f;
  if (keysym          ==          0xffff) return keysym & 0x007f;

  /* Misc mappings */
  /*FIXME if (keysym == XK_Escape)
 	return keysym & 0x001f;
  if (keysym == XK_Delete)
 	return keysym & 0x007f;
  if (keysym == XK_KP_Equal)
    return XK_equal;*/


  /* explicitly mapped */
#define map(lo, hi) if (keysym >= 0x##lo && keysym <= 0x##hi) return ucs4_##lo##_##hi[keysym - 0x##lo];
  map(01a1, 01ff);  map(02a1, 02fe);  map(03a2, 03fe);  map(04a1, 04df);
  map(0590, 05fe);  map(0680, 06ff);  map(07a1, 07f9);  map(08a4, 08fe);
  map(09df, 09f8);  map(0aa1, 0afe);  map(0cdf, 0cfa);  map(0da1, 0df9);
  map(0ea0, 0eff);  map(12a1, 12fe);  map(13bc, 13be);  map(14a1, 14ff);
  map(15d0, 15f6);  map(16a0, 16f6);  map(1e9f, 1eff);  map(20a0, 20ac);
  map(ffb0, ffb9);
  map(ffaa, ffaf);
#undef map

  /* convert to chinese char noe-qwan-doo */
  return 0;
}

SDL_Surface *surface_from_sq_bits(sqInt bitsIndex, sqInt width, sqInt height, sqInt depth)
{
  char *bits = pointerForOop(bitsIndex);

  Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  rmask = 0xff000000;
  gmask = 0x000000ff;
  bmask = 0x0000ff00;
  amask = 0x00ff0000;
#else
  rmask = 0x00ff0000;
  gmask = 0x0000ff00;
  bmask = 0x000000ff;
  amask = 0xff000000;
#endif

  return SDL_CreateRGBSurfaceFrom(bits, width, height, depth, depth / 8 * width, rmask, gmask, bmask, amask);
}

#if 1
// TODO: found on the internet; replace this
int get_length(unsigned char ch) {
  int l;
  unsigned char c = ch;
  c >>= 3;
  // 6 => 0x7e
  // 5 => 0x3e
  if (c == 0x1e) { l = 4; }
  else {
    c >>= 1;
    if (c == 0xe) { l = 3; }
    else {
      c >>= 1;
      if (c == 0x6) { l = 2; }
      else { l = 1; }
    }
  }
  return l;
}
unsigned long *to_unicode(unsigned char *utf8, int len) {
  unsigned char *p = utf8;
  unsigned long ch;
  int x = 0;
  int l;
  unsigned long *result = (unsigned long *)malloc(sizeof(unsigned long)*len);
  unsigned long *r = result;
  if (!result) { return NULL; }
  while (*p) {
    l = get_length(*p);
    switch (l) {
      case 4:
	ch = (*p ^ 0xf0);
	break;
      case 3:
	ch = (*p ^ 0xe0);
	break;
      case 2:
	ch = (*p ^ 0xc0);
	break;
      case 1:
	ch = *p;
	break;
      default:
	printf("Len: %i\n", l);
    }
    ++p;
    int y;
    for (y = l; y > 1; y--) {
      ch <<= 6;
      ch |= (*p ^ 0x80);
      ++p;
    }
    x += l;
    *r = ch;
    r++;
  }
  *r = 0x0;
  return result;
}
#endif

static inline int
withMetaSet(int code, int meta,  int notmeta, int *modStatp, SDL_Event *evt) {
  if (modStatp)
    *modStatp = evt->type == SDL_KEYDOWN
      ? sdl_modifiers_to_sq(evt->key.state) | meta
      : sdl_modifiers_to_sq(evt->key.state) & ~notmeta;
  return code;
}

static int
translateCode(SDL_Keycode sym, int *modp, SDL_Event *evt)
{
  switch (sym)
    {
    case SDLK_LEFT:	return 28;
    case SDLK_UP:	return 30;
    case SDLK_RIGHT:	return 29;
    case SDLK_DOWN:	return 31;
    case SDLK_INSERT:	return  5;
    case SDLK_PRIOR:
    case SDLK_PAGEUP:	return 11;
    case SDLK_PAGEDOWN:	return 12;
    case SDLK_HOME:	return  1;
    case SDLK_END:	return  4;
    case SDLK_RETURN:	return 13;
    case SDLK_DELETE:	return 127;

    /*TODO case XK_KP_Left:	return 28;
    case XK_KP_Up:	return 30;
    case XK_KP_Right:	return 29;
    case XK_KP_Down:	return 31;
    case XK_KP_Insert:	return  5;
    case XK_KP_Prior:	return 11;
    case XK_KP_Next:	return 12;
    case XK_KP_Home:	return  1;
    case XK_KP_End:	return  4;*/

    // FIXME: F keys?

// # if defined(XK_Control_L)
	/* For XK_Shift_L, XK_Shift_R, XK_Caps_Lock & XK_Shift_Lock we can't just
	 * use the SHIFT metastate since it would generate key codes. We use
	 * META + SHIFT as these are all meta keys (meta == OptionKeyBit).
	 */
	case SDLK_LSHIFT:
		return withMetaSet(255,OptionKeyBit+ShiftKeyBit,ShiftKeyBit,modp,evt);
	case SDLK_RSHIFT:
		return withMetaSet(254,OptionKeyBit+ShiftKeyBit,ShiftKeyBit,modp,evt);
	case SDLK_CAPSLOCK:
		return withMetaSet(253,OptionKeyBit+ShiftKeyBit,ShiftKeyBit,modp,evt);
	/* FIXME case XK_Shift_Lock:
		return withMetaSet(252,OptionKeyBit+ShiftKeyBit,ShiftKeyBit,modp,evt);*/
	case SDLK_LCTRL:
		return withMetaSet(251,OptionKeyBit+CtrlKeyBit,CtrlKeyBit,modp,evt);
	case SDLK_RCTRL:
		return withMetaSet(250,OptionKeyBit+CtrlKeyBit,CtrlKeyBit,modp,evt);
	case SDLK_LGUI:
		return withMetaSet(249,OptionKeyBit,0,modp,evt);
	case SDLK_RGUI:
		return withMetaSet(248,OptionKeyBit,0,modp,evt);
	case SDLK_LALT:
		return withMetaSet(247,OptionKeyBit+CommandKeyBit,OptionKeyBit,modp,evt);
	case SDLK_RALT:
		return withMetaSet(246,OptionKeyBit+CommandKeyBit,OptionKeyBit,modp,evt);
// # endif

    default:;
    }
  return -1;
}

int sdl_to_key(SDL_Event *event, int modifiers) {
  char *name = SDL_GetKeyName(event->key.keysym.sym);
  int not_found = !name[0];
  int charCode = name[0];

  if (!not_found)
    charCode = translateCode(event->key.keysym.sym, &modifiers, event);
  if (charCode < 0)
    return -1;

  return charCode;
  /*return not_found && (modifiers & (CommandKeyBit+CtrlKeyBit+OptionKeyBit))
    ? charCode
    : recode(charCode);*/
}

static void handleEvent(SDL_Event *event)
{
  unsigned int sym;
  int was_translated;
  int ucs4;
  int modifiers;
  int keyCode;
  char charCode;
  char name;
  switch (event->type) {
    case SDL_WINDOWEVENT:
      switch (event->window.event) {
	case SDL_WINDOWEVENT_EXPOSED:
	case SDL_WINDOWEVENT_SHOWN:
	  fullDisplayUpdate();
	  break;
	case SDL_WINDOWEVENT_CLOSE:
	  recordWindowEvent(WindowEventClose, 0, 0, 0, 0, 1); /* windowIndex 1 is main window */
	  break;
	case SDL_WINDOWEVENT_RESIZED:
	  destroy_context(&state);
	  init_context(&state);
	  break;
      }
      break;

#define IS_MOD(x) x == SDLK_LCTRL || x == SDLK_LSHIFT || x == SDLK_LALT || x == SDLK_LGUI

    case SDL_KEYDOWN:
      modifiers = current_modifiers();
      keyCode = sdl_to_key(event, modifiers);
      ucs4 = xkeysym2ucs4(event->key.keysym.sym);

      // name = was_translated ? 0 : tolower(SDL_GetKeyName(sym)[0]);
      // printf("DOWN %d %c\n", sym, name);
      // if (IS_MOD(sym)) break;
      if (keyCode >= 0 || ucs4 > 0) {
	recordKeyboardEvent(keyCode, EventKeyDown, modifiers, ucs4);
	if (ucs4)
	  recordKeyboardEvent(keyCode, EventKeyChar, modifiers, ucs4);
      }
      /*if (sym < 32 || sym > 2000 ||
	  sym == SDLK_RETURN ||
	  sym == SDLK_ESCAPE ||
	  sym == SDLK_TAB ||
	  sym == SDLK_BACKSPACE ||
	  sym == SDLK_DELETE) {
	printf("Report char\n");
	recordKeyboardEvent(sym, EventKeyChar, current_modifiers(), 0);
      }*/
      break;

    case SDL_KEYUP:
      modifiers = current_modifiers();
      keyCode = sdl_to_key(event, modifiers);
      ucs4 = xkeysym2ucs4(event->key.keysym.sym);

      // sym = translateCode(event->key.keysym.sym, &was_translated);
      // name = was_translated ? 0 : tolower(SDL_GetKeyName(sym)[0]);
      // if (IS_MOD(sym)) break;
      if (keyCode >= 0 || ucs4 > 0)
	recordKeyboardEvent(keyCode, EventKeyUp, modifiers, ucs4);
      break;

    case SDL_TEXTINPUT:
    {
      printf("Input: %s %u\n", event->text.text, current_modifiers());
      unsigned long *res = to_unicode(event->text.text, 2);
      recordKeyboardEvent(res[0], EventKeyChar, current_modifiers(), res[0]);
      break;
    }

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      modifierState = current_modifiers();
      mousePosition.x = event->button.x;
      mousePosition.y = event->button.y;
      if (event->type == SDL_MOUSEBUTTONDOWN)
	buttonState |= sdl_button_to_sq(event->button.button);
      else
	buttonState &= ~sdl_button_to_sq(event->button.button);
      recordMouseEvent();
      break;

    case SDL_MOUSEWHEEL:
      recordMouseWheelEvent(event->wheel.x * 120, event->wheel.y * 120, current_modifiers());
      break;

    case SDL_MOUSEMOTION:
      modifierState = current_modifiers();
      mousePosition.x = event->motion.x;
      mousePosition.y = event->motion.y;
      recordMouseEvent();
      break;

    case SDL_DROPFILE:
      if (uxDropFileCount > 0) {
	SDL_free(uxDropFileNames[0]);
      }

      uxDropFileCount = 1;
      if (!uxDropFileNames)
	uxDropFileNames = SDL_malloc(sizeof(char *));
      uxDropFileNames[0] = event->drop.file;
      recordDragEvent(SQDragDrop, 1);
      break;
    /* only from sdl 2.0.5 onwards
    case SDL_DROPTEXT:
      printf("Drop text\n");
      SDL_Free(event->drop.file);
      break;
    case SDL_DROPBEGIN:
      printf("Drop begin\n");
      break;
    case SDL_DROPCOMPLETE:
      printf("Drop begin\n");
      break;*/
  }
}

static int handleEvents(void)
{
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    handleEvent(&event);
  }

  return 1;	/* 1 if events processed */
}

static void xHandler(int fd, void *data, int flags)
{
  handleEvents();
  aioHandle(1, xHandler, AIO_RX);
}

static sqInt display_dndOutStart(char *types, int ntypes)	{ return 0; }
static void  display_dndOutSend (char *bytes, int nbytes)	{ return  ; }
static sqInt display_dndOutAcceptedType(char *buf, int nbuf)	{ return 0; }
static sqInt display_dndReceived(char *fileName)	{ return 0; }
/*static sqInt display_ioGetButtonState(void)		{ return 0; }
static sqInt display_ioPeekKeystroke(void)		{ return 0; }
static sqInt display_ioGetKeystroke(void)		{ return 0; }
static sqInt display_ioGetNextEvent(sqInputEvent *evt)	{ return 0; }
static sqInt display_ioMousePoint(void)			{ return 0; }*/

static sqInt display_clipboardSize(void)
{
  trace();
  return 0;
}

static sqInt display_clipboardWriteFromAt(sqInt count, sqInt byteArrayIndex, sqInt startIndex)
{
  trace();
  return 0;
}

static sqInt display_clipboardReadIntoAt(sqInt count, sqInt byteArrayIndex, sqInt startIndex)
{
  trace();
  return 0;
}

static char **display_clipboardGetTypeNames(void)
{
  return 0;
}

static sqInt display_clipboardSizeWithType(char *typeName, int nTypeName)
{
  return 0;
}

static void display_clipboardWriteWithType(char *data, size_t ndata, char *typeName, size_t nTypeName, int isDnd, int isClaiming)
{
  return;
}

static sqInt display_ioFormPrint(sqInt bitsIndex, sqInt width, sqInt height, sqInt depth, double hScale, double vScale, sqInt landscapeFlag)
{
  trace();
  return false;
}

static sqInt display_ioBeep(void)
{
  trace();
  return 0;
}

static sqInt display_ioRelinquishProcessorForMicroseconds(sqInt microSeconds)
{
  aioSleepForUsecs(handleEvents() ? 0 : microSeconds);
  return 0;
}

static sqInt display_ioProcessEvents(void)
{
  handleEvents();
  aioPoll(0);
  return 0;
}

static double display_ioScreenScaleFactor(void)
{
  trace();
  float dpi;
  if (SDL_GetDisplayDPI(0 /*FIXME display index*/, NULL, &dpi, NULL))
    return dpi / 96.0;

  return 1.0;
}

static sqInt display_ioScreenDepth(void)
{
  return 32;
}

static sqInt display_ioScreenSize(void)
{
  int width, height;
  SDL_GetWindowSize(window, &width, &height);
  return (width << 16) | height;
}

static unsigned char swapBits(unsigned char in)
{
  unsigned char out= 0;
  int i;
  for (i= 0; i < 8; i++)
    {
      out= (out << 1) + (in & 1);
      in >>= 1;
    }
  return out;
}

static sqInt display_ioSetCursorWithMask(sqInt cursorBitsIndex, sqInt cursorMaskIndex, sqInt offsetX, sqInt offsetY)
{
  unsigned int *cursorBits = (unsigned char *) pointerForOop(cursorBitsIndex);
  unsigned int *cursorMask = (unsigned char *) pointerForOop(cursorMaskIndex);

  int i;
  unsigned char data[32], mask[32];	/* cursors are always 16x16 */
  for (i= 0; i < 16; i++)
  {
    data[i*2+0]= (cursorBits[i] >> 24) & 0xFF;
    data[i*2+1]= (cursorBits[i] >> 16) & 0xFF;
    mask[i*2+0]= (cursorMask[i] >> 24) & 0xFF;
    mask[i*2+1]= (cursorMask[i] >> 16) & 0xFF;
  }

  static SDL_Cursor *cursor;
  if (cursor)
    SDL_FreeCursor(cursor);

  cursor = SDL_CreateCursor(data, mask, 16, 16, -offsetX, -offsetY);
  if (!cursor)
    printf("Creating cursor failed: %s\n", SDL_GetError());
  SDL_SetCursor(cursor);
  return 0;
}

static sqInt display_ioSetCursorARGB(sqInt cursorBitsIndex, sqInt extentX, sqInt extentY, sqInt offsetX, sqInt offsetY)
{
  trace();
  static SDL_Surface *cursor_surface;
  static SDL_Cursor *cursor;

  if (cursor)
    SDL_FreeCursor(cursor);
  if (cursor_surface)
    SDL_FreeSurface(cursor_surface);

  cursor_surface = surface_from_sq_bits(cursorBitsIndex, extentX, extentY, 32);
  cursor = SDL_CreateColorCursor(cursor_surface, -offsetX, -offsetY);
  SDL_SetCursor(cursor);
  return 0;
}

static sqInt display_ioSetFullScreen(sqInt fullScreen)
{
  return SDL_SetWindowFullscreen(window, fullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

static sqInt display_ioForceDisplayUpdate(void)
{
  // trace();
  // SDL_UpdateWindowSurface(window);

  /*glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  draw_window(&state);
  SDL_GL_SwapWindow(window);*/
  return 0;
}

static void draw_test() {
  int dw, dh;
  SDL_GL_GetDrawableSize(window, &dw, &dh);

  glViewport(0, 0, dw, dh);
  glClearColor(1, 1, 1, 1);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

}

static sqInt display_ioShowDisplay(sqInt dispBitsIndex, sqInt width, sqInt height, sqInt depth,
				   sqInt affectedL, sqInt affectedR, sqInt affectedT, sqInt affectedB)
{
  // trace();
  // printf("DRAW DEPTH %d (%dx%d) %d %i %i %i %i\n", depth, width, height, depth / 8 * width, affectedL, affectedR, affectedT, affectedB);
  if (width < 1 || height < 1)
    return 0;

  if (affectedL > width)  affectedL= width;
  if (affectedR > width)  affectedR= width;
  if (affectedT > height) affectedT= height;
  if (affectedB > height) affectedB= height;

  // SDL_Rect rect;
  // rect.x = affectedL;
  // rect.y = affectedT;
  // rect.w = affectedR - affectedL;
  // rect.h = affectedB - affectedT;

  // SDL_Surface *screen = SDL_GetWindowSurface(window);
  // printf("b %i\n", screen->locked);

  // TODO optimize: can we create a smaller surface?
  // SDL_Surface *image = surface_from_sq_bits(dispBitsIndex, width, height, depth);
  // SDL_BlitSurface(image, &rect, screen, &rect);
  // SDL_UpdateWindowSurface(window);

  // SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, image);
  // SDL_FreeSurface(image);

  blit(&state, pointerForOop(dispBitsIndex), width, height, depth,
      affectedL, affectedR, affectedT, affectedB);

  // draw_text(&state, 200, 200);
  SDL_GL_SwapWindow(state.window);

  return 0;
}

static void *display_ioSkiaContext() {
  return NULL;
}

static sqInt display_ioHasDisplayDepth(sqInt i)
{
  return 32 == i;
}

static sqInt display_ioSetDisplayMode(sqInt width, sqInt height, sqInt depth, sqInt fullscreenFlag)
{
  trace();
  setSavedWindowSize((width << 16) + (height & 0xFFFF));
  setFullScreenFlag(fullscreenFlag);
  return 0;
}

static void display_winSetName(char *imageName)
{
  SDL_SetWindowTitle(window, imageName);
}

static void *display_ioGetDisplay(void)	{ return 0; }
static void *display_ioGetWindow(void)	{ return 0; }

static sqInt display_ioGLinitialise(void) { trace();  return 0; }
static sqInt display_ioGLcreateRenderer(glRenderer *r, sqInt x, sqInt y, sqInt w, sqInt h, sqInt flags) { trace();  return 0; }
static void  display_ioGLdestroyRenderer(glRenderer *r) { trace(); }
static void  display_ioGLswapBuffers(glRenderer *r) { trace(); }
static sqInt display_ioGLmakeCurrentRenderer(glRenderer *r) { trace();  return 0; }
static void  display_ioGLsetBufferRect(glRenderer *r, sqInt x, sqInt y, sqInt w, sqInt h) { trace(); }

static char *display_winSystemName(void)
{
  return "SDL2";
}

#define handle_sdl_error(msg)

static void display_winInit(void)
{
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  static const int kStencilBits = 8;  // Skia needs 8 stencil bits
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, kStencilBits);

  SDL_GL_SetSwapInterval(1);

  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    handle_sdl_error("Could not init sdl");
  }
}

void get_window_size(int *width, int *height)
{
  int s = getSavedWindowSize();
  if (s) {
    *width = s >> 16;
    *height = s & 0xffff;
  } else {
    *width = 640;
    *height = 480;
  }
}

static void display_winOpen(int argc, char *dropFiles[])
{
  int width, height;
  get_window_size(&width, &height);

  window = SDL_CreateWindow(
      "Squeak SDL2",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      SDL_WINDOW_OPENGL /*| SDL_WINDOW_ALLOW_HIGHDPI*/ | SDL_WINDOW_RESIZABLE);

  SDL_StartTextInput();

  if (!window) {
    handle_sdl_error("Could not create window");
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (!gl_context) {
    handle_sdl_error("Could not create gl contexts");
  }
  if (SDL_GL_MakeCurrent(window, gl_context)) {
    handle_sdl_error("could not activate gl context")
  }

  state.window = window;
  init_context(&state);

  aioEnable(1, 0, AIO_EXT);
  aioHandle(1, xHandler, AIO_RX);

  (void) recordKeystroke;
  (void) recordDragEvent;
}

static void display_winExit(void)
{
  aioDisable(1);
  SDL_StopTextInput();
  SDL_DestroyWindow(window);
  SDL_Quit();
}

static long  display_winImageFind(char *buf, int len)		{ trace();  return 0; }
static void display_winImageNotFound(void)			{ trace(); }

/*
 * Browser Plugin
 */
static sqInt display_primitivePluginBrowserReady(void)		{ return primitiveFail(); }
static sqInt display_primitivePluginRequestURLStream(void)	{ return primitiveFail(); }
static sqInt display_primitivePluginRequestURL(void)		{ return primitiveFail(); }
static sqInt display_primitivePluginPostURL(void)		{ return primitiveFail(); }
static sqInt display_primitivePluginRequestFileHandle(void)	{ return primitiveFail(); }
static sqInt display_primitivePluginDestroyRequest(void)	{ return primitiveFail(); }
static sqInt display_primitivePluginRequestState(void)		{ return primitiveFail(); }

/*
 * Host window
 */
#if (SqDisplayVersionMajor >= 1 && SqDisplayVersionMinor >= 2)
static long display_hostWindowClose(long index)             { return 0; }
static long display_hostWindowCreate(long w, long h, long x, long y,
  char *list, long attributeListLength)                     { return 0; }
static long display_hostWindowShowDisplay(unsigned *dispBitsIndex, long width, long height, long depth,
  long affectedL, long affectedR, long affectedT, long affectedB, long windowIndex)              { return 0; }
static long display_hostWindowGetSize(long windowIndex)     { return -1; }
static long display_hostWindowSetSize(long windowIndex, long w, long h) { return -1; }
static long display_hostWindowGetPosition(long windowIndex) { return -1; }
static long display_hostWindowSetPosition(long windowIndex, long x, long y) { return -1; }
static long display_hostWindowSetTitle(long windowIndex, char *newTitle, long sizeOfTitle)     { return -1; }
static long display_hostWindowCloseAll(void)                { return 0; }
#endif

#if (SqDisplayVersionMajor >= 1 && SqDisplayVersionMinor >= 3)
long display_ioSetCursorPositionXY(long x, long y) { return 0; }
long display_ioPositionOfScreenWorkArea (long windowIndex) { return -1; }
long display_ioSizeOfScreenWorkArea (long windowIndex) { return -1; }
void *display_ioGetWindowHandle() { return 0; }
long display_ioPositionOfNativeDisplay(void *windowHandle) { return -1; }
long display_ioSizeOfNativeDisplay(void *windowHandle) { return -1; }
long display_ioPositionOfNativeWindow(void *windowHandle) { return -1; }
long display_ioSizeOfNativeWindow(void *windowHandle) { return -1; }
#endif /* (SqDisplayVersionMajor >= 1 && SqDisplayVersionMinor >= 3) */

SqDisplayDefine(sdl2);	/* name must match that in makeInterface() below */


/*** module ***/


static void display_printUsage(void)
{
  printf("\nCustom Window <option>s: (none)\n");
  /* otherwise... */
}

static void display_printUsageNotes(void)
{
  trace();
}

static void display_parseEnvironment(void)
{
  trace();
}

static int display_parseArgument(int argc, char **argv)
{
  return 0;	/* arg not recognised */
}

static void *display_makeInterface(void)
{
  return &display_sdl2_itf;		/* name must match that in SqDisplayDefine() above */
}

#include "SqModule.h"

SqModuleDefine(display, sdl2);	/* name must match that in sqUnixMain.c's moduleDescriptions */
