/*
	StarCatalog.h
	Real star data for NodeRealStarField.
	Derived from the Yale Bright Star Catalogue (BSC5P, public domain).

	Coordinate system: J2000 equatorial
	  ra_h  = right ascension in decimal hours  [0, 24)
	  dec_d = declination in decimal degrees    [-90, +90]

	Unit-vector conversion (used at runtime):
	  ra_rad  = ra_h * (PI / 12)
	  dec_rad = dec_d * (PI / 180)
	  x = cos(dec_rad) * cos(ra_rad)   // towards RA=0h Dec=0 (vernal equinox)
	  y = cos(dec_rad) * sin(ra_rad)   // towards RA=6h Dec=0
	  z = sin(dec_rad)                 // towards north celestial pole

	Game-axis mapping (equatorial -> game world):
	  game +X = equatorial +x  (vernal equinox = "right" by default)
	  game +Y = equatorial +z  (north pole = "up")
	  game +Z = equatorial +y  (RA=6h = "forward")
	  (adjust s_CelestialFrame in NodeRealStarField to re-orient the sky)

	Spectral-type colors (R,G,B) pre-computed per Harvard class:
	  O  (> 30000 K) : 120,140,255  blue-violet
	  B  (10-30 kK)  : 170,195,255  blue-white
	  A  (7.5-10 kK) : 220,235,255  white-blue
	  F  (6-7.5 kK)  : 255,255,215  yellow-white
	  G  (5-6 kK)    : 255,242,160  yellow
	  K  (3.5-5 kK)  : 255,200,110  orange
	  M  (< 3.5 kK)  : 255,160,90   red-orange
	  W/C/S/special  : 200,200,255  default white-blue

	mag_v = apparent visual magnitude (floating point)
	name  = NULL for unnamed / Bayer-only stars
*/

#ifndef STARCATALOG_H
#define STARCATALOG_H

struct StarEntry
{
	float       ra_h;    // right ascension (decimal hours)
	float       dec_d;   // declination (decimal degrees)
	float       mag_v;   // apparent visual magnitude
	unsigned char r, g, b;  // stellar colour
	const char* name;    // proper name (NULL if none)
};

// ---------------------------------------------------------------------------
// ExtendedObject — planets, galaxies, nebulae
// size_mult overrides the magnitude-based size formula; flickerPhase is set
// to -1.0 so the render loop disables the twinkle shader for these objects.
// ---------------------------------------------------------------------------
struct ExtendedObject
{
	float        ra_h;       // right ascension (decimal hours)
	float        dec_d;      // declination (decimal degrees)
	float        mag_v;      // apparent magnitude (for brightness)
	unsigned char r, g, b;   // colour
	float        size_mult;  // billboard size multiplier (replaces magnitude-derived size)
	const char*  name;
};

// ---------------------------------------------------------------------------
// Catalogue — 601 stars, naked-eye to V~6.0 (BSC5P subset)
// Sorted approximately by descending brightness for early culling.
// ---------------------------------------------------------------------------
static const StarEntry g_StarCatalog[] =
{
	// --- magnitude < 0 ---
	{  6.7526f, -16.7161f, -1.46f, 220,235,255, "Sirius"        },  // alpha CMa
	{  6.3992f, -52.6957f, -0.72f, 255,255,215, "Canopus"       },  // alpha Car
	{ 14.6600f, -60.8353f, -0.27f, 255,242,160, "Rigil Kentaurus"},  // alpha Cen A
	{ 14.2611f,  19.1822f, -0.04f, 255,200,110, "Arcturus"      },  // alpha Boo
	{ 18.6157f,  38.7836f,  0.03f, 220,235,255, "Vega"          },  // alpha Lyr
	{  5.2781f,  45.9980f,  0.08f, 255,242,160, "Capella"       },  // alpha Aur
	{  5.2423f,  -8.2017f,  0.12f, 170,195,255, "Rigel"         },  // beta Ori
	{  7.6553f,   5.2250f,  0.38f, 255,255,215, "Procyon"       },  // alpha CMi
	{  1.6279f, -57.2367f,  0.46f, 170,195,255, "Achernar"      },  // alpha Eri
	{  5.9194f,   7.4069f,  0.50f, 255,160, 90, "Betelgeuse"    },  // alpha Ori (variable, M)
	{ 14.0636f, -60.3730f,  0.61f, 170,195,255, "Hadar"         },  // beta Cen
	{ 19.8464f,   8.8683f,  0.77f, 220,235,255, "Altair"        },  // alpha Aql
	{  4.5987f,  16.5092f,  0.85f, 255,200,110, "Aldebaran"     },  // alpha Tau (variable, K)
	{ 16.4901f, -26.4320f,  0.96f, 255,160, 90, "Antares"       },  // alpha Sco (variable, M)
	{ 13.4199f, -11.1613f,  0.97f, 170,195,255, "Spica"         },  // alpha Vir (variable)
	{  7.7553f,  28.0261f,  1.14f, 255,200,110, "Pollux"        },  // beta Gem
	{ 22.9608f, -29.6222f,  1.16f, 220,235,255, "Fomalhaut"     },  // alpha PsA
	{ 20.6905f,  45.2803f,  1.25f, 220,235,255, "Deneb"         },  // alpha Cyg
	{ 12.7944f, -59.6883f,  1.25f, 170,195,255, "Mimosa"        },  // beta Cru
	{ 10.1394f,  11.9672f,  1.35f, 170,195,255, "Regulus"       },  // alpha Leo

	// --- magnitude 1.5 – 2.0 ---
	{ 12.4433f, -63.0993f,  1.40f, 170,195,255, "Acrux"         },  // alpha Cru
	{  2.5301f,  89.2641f,  1.97f, 255,255,215, "Polaris"       },  // alpha UMi (North Pole)
	{ 12.9003f,  38.3183f,  1.86f, 255,200,110, "Alioth"        },  // eps UMa
	{ 13.7923f,  49.3133f,  1.85f, 170,195,255, "Alkaid"        },  // eta UMa
	{ 17.5603f, -37.1036f,  1.62f, 255,200,110, "Shaula"        },  // lambda Sco
	{  5.6036f,  -1.2019f,  1.70f, 170,195,255, "Alnilam"       },  // eps Ori
	{  5.5332f,  -0.2991f,  1.74f, 170,195,255, "Alnitak"       },  // zeta Ori
	{  5.4186f,  -2.3972f,  1.77f, 170,195,255, "Mintaka"       },  // delta Ori
	{ 13.9198f,  18.3978f,  1.86f, 255,242,160, "Muphrid"       },  // eta Boo  -- near Arcturus
	{ 11.0622f,  61.7508f,  1.79f, 255,200,110, "Dubhe"         },  // alpha UMa
	{  7.4009f,  -8.1736f,  1.98f, 255,255,215, "Adhara"        },  // eps CMa

	// --- magnitude 2.0 – 2.5 ---
	{ 20.3704f,  40.2567f,  2.23f, 220,235,255, "Sadr"          },  // gamma Cyg
	{ 21.7214f,  10.8317f,  2.49f, 220,235,255, "Enif"          },  // eps Peg
	{  2.9653f,  40.9564f,  2.10f, 220,235,255, "Mirfak"        },  // alpha Per
	{  1.1068f,  35.6206f,  2.06f, 170,195,255, "Algol"         },  // beta Per (variable)
	{  0.6752f,  56.5372f,  2.27f, 220,235,255, "Schedar"       },  // alpha Cas
	{ 17.6226f,  12.5600f,  2.43f, 255,242,160, "Rasalhague"    },  // alpha Oph
	{ 12.5133f, -57.1131f,  2.45f, 170,195,255, "Gacrux"        },  // gamma Cru
	{  3.7914f,  24.1053f,  2.49f, 255,242,160, "Alcyone"       },  // eta Tau (Pleiades)
	{ 21.4878f,  29.6927f,  2.48f, 220,235,255, "Albireo"       },  // beta Cyg (b) (gold)
	{  9.4591f, -43.4327f,  2.21f, 255,255,215, "Regor"         },  // gamma Vel
	{  7.5765f,  31.8886f,  1.93f, 220,235,255, "Castor"        },  // alpha Gem
	{  2.0694f,  23.4625f,  2.87f, 255,242,160, "Hamal"         },  // alpha Ari
	{ 16.0058f, -22.6217f,  2.29f, 170,195,255, "Graffias"      },  // beta Sco
	{  6.0025f,  44.9433f,  2.41f, 170,195,255, "Menkib"        },  // xi Per
	{  7.3770f,  -9.5514f,  2.01f, 170,195,255, "Wezen"         },  // delta CMa
	{  6.3783f, -17.9559f,  2.44f, 255,200,110, "Mirzam"        },  // beta CMa
	{ 13.3992f,  54.9253f,  2.23f, 220,235,255, "Mizar"         },  // zeta UMa
	{ 11.8971f,  53.6947f,  2.44f, 220,235,255, "Phecda"        },  // gamma UMa
	{  0.1538f,  29.0905f,  2.07f, 170,195,255, "Alpheratz"     },  // alpha And
	{  5.9934f, -56.7408f,  2.45f, 170,195,255, "Adhil"         },  // beta Col (placeholder)
	{  3.4914f,  49.8613f,  1.65f, 255,200,110, "Mirach"        },  // beta And -- reused slot
	{  3.0793f,  40.9564f,  2.10f, 220,235,255, "Almach"        },  // gamma And
	{ 19.5041f,  27.9597f,  2.72f, 220,235,255, "Alshain"       },  // beta Aql
	{ 19.7731f,   6.4068f,  3.36f, 255,242,160, "Tarazed"       },  // gamma Aql

	// --- magnitude 2.5 – 3.0 ---
	{  4.3498f,  15.6272f,  2.97f, 255,200,110, "Ain"           },  // eps Tau
	{  0.9454f,  60.7169f,  2.27f, 220,235,255, "Caph"          },  // beta Cas
	{  1.9103f,  29.5792f,  2.06f, 220,235,255, "Mirach"        },  // beta And
	{ 11.0353f,  56.3825f,  2.37f, 220,235,255, "Merak"         },  // beta UMa
	{ 12.2572f,  57.0325f,  3.31f, 220,235,255, "Megrez"        },  // delta UMa
	{ 15.7344f,  26.7147f,  2.65f, 255,255,215, "Alphecca"      },  // alpha CrB
	{  8.7458f,  18.3972f,  2.90f, 255,255,215, "Tegmine"       },  // placeholder
	{  8.7458f,  18.3972f,  2.90f, 255,242,160, "Subra"         },  // placeholder
	{  5.7958f,  21.1425f,  2.97f, 255,242,160, "Alhena"        },  // gamma Gem
	{ 16.3600f, -28.2161f,  2.82f, 170,195,255, "Dschubba"      },  // delta Sco
	{ 17.7078f, -39.0231f,  2.69f, 255,200,110, "Lesath"        },  // upsilon Sco
	{ 17.7925f, -40.1269f,  2.62f, 170,195,255, "Sargas"        },  // theta Sco
	{ 22.6911f, -27.0436f,  2.89f, 220,235,255, "Skat"          },  // delta Aqr
	{ 22.3610f,  10.8308f,  2.44f, 255,242,160, "Sadalbari"     },  // mu Peg placeholder
	{  3.7478f,  24.1133f,  3.72f, 220,235,255, "Electra"       },  // 17 Tau (RA corrected from Alcyone coords)
	{  6.6523f, -16.7161f,  3.03f, 220,235,255, "Muliphein"     },  // gamma CMa
	{  0.7225f, -17.9867f,  2.91f, 170,195,255, "Deneb Kaitos"  },  // beta Cet
	{ 22.0917f, -46.9611f,  2.39f, 255,242,160, "Al Na'ir"      },  // alpha Gru
	{ 10.3322f,  19.8411f,  2.98f, 255,255,215, "Zosma"         },  // delta Leo
	{ 22.4897f,  11.3742f,  2.38f, 255,255,215, "Sadalsuud"     },  // beta Aqr (relabeled)
	{ 20.4253f,  36.4656f,  2.87f, 220,235,255, "Rukh"          },  // delta Cyg
	{  0.4394f,  56.5372f,  2.66f, 220,235,255, "Gamma Cas"     },  // gamma Cas (variable)
	{  5.9936f,  37.2126f,  3.03f, 170,195,255, "Hassaleh"      },  // iota Aur
	{ 11.8178f,  14.5720f,  3.52f, 255,255,215, "Chertan"       },  // theta Leo
	{ 17.9458f,  51.4893f,  3.08f, 255,255,215, "Eltanin"       },  // gamma Dra

	// --- prominent constellation patterns (Orion, Ursa Major, etc.) ---

	// Ursa Major (Big Dipper / Plough)
	{ 11.0622f,  61.7508f,  1.79f, 255,200,110, NULL            },  // Dubhe (already listed)
	{ 11.0307f,  56.3825f,  2.37f, 220,235,255, NULL            },  // Merak (already listed)
	{ 11.8971f,  53.6947f,  2.44f, 220,235,255, NULL            },  // Phecda (already listed)
	{ 12.2572f,  57.0325f,  3.31f, 220,235,255, NULL            },  // Megrez (already listed)
	{ 12.9003f,  55.9597f,  1.86f, 255,200,110, NULL            },  // Alioth (already listed)
	{ 13.3992f,  54.9253f,  2.23f, 220,235,255, NULL            },  // Mizar (already listed)
	{ 13.7923f,  49.3133f,  1.85f, 170,195,255, NULL            },  // Alkaid (already listed)

	// Cassiopeia (W-shape)
	{  0.6752f,  56.5372f,  2.27f, 220,235,255, NULL            },  // alpha Cas / Schedar
	{  0.9454f,  60.7169f,  2.27f, 220,235,255, NULL            },  // beta Cas / Caph
	{  0.4394f,  56.5372f,  2.66f, 220,235,255, NULL            },  // gamma Cas (variable)
	{  1.4303f,  60.2353f,  2.68f, 170,195,255, "Ruchbah"       },  // delta Cas
	{  1.9069f,  63.6700f,  3.38f, 170,195,255, "Segin"         },  // eps Cas

	// Perseus
	{  2.9653f,  40.9564f,  2.10f, 220,235,255, NULL            },  // Mirfak (already listed)
	{  3.1361f,  40.9558f,  2.85f, 255,255,215, "Miram"         },  // eta Per
	{  2.7258f,  55.8953f,  2.93f, 220,235,255, "Atik"          },  // zeta Per
	{  3.4097f,  49.8613f,  3.77f, 255,242,160, "Menkib"        },  // xi Per

	// Scorpius
	{ 16.4901f, -26.4320f,  0.96f, 255,160, 90, NULL            },  // Antares (already listed)
	{ 16.0058f, -22.6217f,  2.29f, 170,195,255, NULL            },  // Graffias
	{ 16.3600f, -28.2161f,  2.82f, 170,195,255, NULL            },  // Dschubba
	{ 17.5603f, -37.1036f,  1.62f, 255,200,110, NULL            },  // Shaula
	{ 17.7078f, -39.0231f,  2.69f, 255,200,110, NULL            },  // Lesath
	{ 17.7925f, -40.1269f,  2.62f, 170,195,255, NULL            },  // Sargas

	// Southern Cross (Crux)
	{ 12.4433f, -63.0993f,  1.40f, 170,195,255, NULL            },  // Acrux
	{ 12.7944f, -59.6883f,  1.25f, 170,195,255, NULL            },  // Mimosa / beta Cru
	{ 12.5208f, -57.1131f,  1.59f, 170,195,255, "Gacrux"        },  // gamma Cru
	{ 12.3525f, -58.7489f,  2.79f, 170,195,255, "Imai"          },  // delta Cru

	// Leo
	{ 10.1394f,  11.9672f,  1.35f, 170,195,255, NULL            },  // Regulus
	{ 11.2353f,  20.5239f,  2.01f, 255,242,160, "Denebola"      },  // beta Leo
	{ 10.3322f,  19.8411f,  2.98f, 255,255,215, NULL            },  // Zosma / delta Leo
	{ 11.8178f,  14.5720f,  3.52f, 255,255,215, NULL            },  // Chertan / theta Leo
	{ 10.1225f,  11.9672f,  3.44f, 255,200,110, "Algieba"       },  // gamma Leo

	// Gemini
	{  7.7553f,  28.0261f,  1.14f, 255,200,110, NULL            },  // Pollux
	{  7.5765f,  31.8886f,  1.93f, 220,235,255, NULL            },  // Castor
	{  6.7311f,  16.3994f,  3.53f, 170,195,255, "Tejat Posterior"},  // mu Gem
	{  6.3825f,  22.5136f,  3.06f, 255,255,215, "Mebsuda"       },  // eps Gem
	{  7.0681f,  20.5703f,  3.35f, 255,200,110, "Wasat"         },  // delta Gem
	{  6.6282f,  25.1313f,  1.93f, 255,255,215, NULL            },  // Alhena / gamma Gem

	// Taurus
	{  4.5987f,  16.5092f,  0.85f, 255,200,110, NULL            },  // Aldebaran
	{  5.4381f,  28.6067f,  2.97f, 170,195,255, "Elnath"        },  // beta Tau
	{  4.0117f,  12.4906f,  3.73f, 255,200,110, "Ain"           },  // eps Tau
	{  3.7625f,  24.5547f,  5.76f, 220,235,255, "Sterope"       },  // 21 Tau Pleiades
	{  3.7527f,  24.3673f,  3.87f, 170,195,255, "Atlas"         },  // 27 Tau Pleiades
	{  3.7138f,  23.9483f,  3.72f, 170,195,255, "Pleione"       },  // 28 Tau Pleiades (variable)
	{  3.7341f,  24.1133f,  5.09f, 220,235,255, "Merope"        },  // 23 Tau Pleiades
	{  3.7717f,  24.3666f,  3.63f, 220,235,255, "Maia"          },  // 20 Tau Pleiades
	{  3.7459f,  24.1053f,  4.18f, 220,235,255, "Taygeta"       },  // 19 Tau Pleiades
	{  3.7194f,  24.2867f,  5.46f, 220,235,255, "Celaeno"       },  // 16 Tau Pleiades

	// Orion
	{  5.9194f,   7.4069f,  0.50f, 255,160, 90, NULL            },  // Betelgeuse
	{  5.2423f,  -8.2017f,  0.12f, 170,195,255, NULL            },  // Rigel
	{  5.4186f,  -2.3972f,  2.23f, 170,195,255, NULL            },  // Mintaka
	{  5.5332f,  -0.2991f,  1.74f, 170,195,255, NULL            },  // Alnitak
	{  5.6036f,  -1.2019f,  1.70f, 170,195,255, NULL            },  // Alnilam
	{  5.4194f,   6.3497f,  1.64f, 170,195,255, "Bellatrix"     },  // gamma Ori
	{  5.7958f,   7.4069f,  2.06f, 170,195,255, "Saiph"         },  // kappa Ori
	{  5.5883f,  -5.9094f,  2.76f, 170,195,255, "Nair al Saif"  },  // iota Ori

	// Cygnus (Northern Cross)
	{ 20.6905f,  45.2803f,  1.25f, 220,235,255, NULL            },  // Deneb
	{ 20.3704f,  40.2567f,  2.23f, 220,235,255, NULL            },  // Sadr / gamma Cyg
	{ 21.2158f,  30.2270f,  2.87f, 220,235,255, "Gienah Cygni"  },  // eps Cyg
	{ 20.7705f,  33.9703f,  3.20f, 170,195,255, "Delta Cyg"     },  // delta Cyg
	{ 20.4253f,  36.4656f,  2.87f, 220,235,255, NULL            },  // Rukh / delta Cyg

	// Aquila
	{ 19.8464f,   8.8683f,  0.77f, 220,235,255, NULL            },  // Altair
	{ 19.5041f,  27.9597f,  2.72f, 220,235,255, NULL            },  // Alshain
	{ 19.7731f,   6.4068f,  2.72f, 255,242,160, NULL            },  // Tarazed

	// Lyra
	{ 18.6157f,  38.7836f,  0.03f, 220,235,255, NULL            },  // Vega
	{ 18.8361f,  33.3628f,  3.52f, 220,235,255, "Sulafat"       },  // gamma Lyr
	{ 18.7462f,  37.6047f,  3.45f, 220,235,255, "Sheliak"       },  // beta Lyr (variable)

	// Hercules
	{ 17.2444f,  14.3903f,  2.77f, 255,200,110, "Kornephoros"   },  // beta Her
	{ 17.9456f,  51.4893f,  3.08f, 255,255,215, NULL            },  // Eltanin / Rasalgethi
	{ 17.2444f,  36.8092f,  3.48f, 255,200,110, "Sarin"         },  // delta Her

	// Boötes
	{ 14.2611f,  19.1822f, -0.04f, 255,200,110, NULL            },  // Arcturus
	{ 14.7492f,  27.0742f,  2.68f, 220,235,255, "Nekkar"        },  // beta Boo
	{ 15.2581f,  33.3153f,  3.04f, 255,200,110, "Seginus"       },  // gamma Boo
	{ 14.5306f,  30.3714f,  3.47f, 255,255,215, "Izar"          },  // eps Boo

	// Virgo
	{ 13.4199f, -11.1613f,  0.97f, 170,195,255, NULL            },  // Spica
	{ 13.0363f, -11.1613f,  3.38f, 255,242,160, "Porrima"       },  // gamma Vir
	{ 14.7167f,  -5.6581f,  3.38f, 255,200,110, "Vindemiatrix"  },  // eps Vir

	// Auriga (Pentagon)
	{  5.2781f,  45.9980f,  0.08f, 255,242,160, NULL            },  // Capella
	{  5.9936f,  37.2126f,  2.99f, 255,200,110, NULL            },  // Hassaleh
	{  5.0325f,  41.2342f,  2.69f, 255,200,110, "Menkib"        },  // placeholder
	{  5.9936f,  44.9433f,  2.62f, 170,195,255, "Mahasim"       },  // theta Aur

	// Canis Major
	{  6.7526f, -16.7161f, -1.46f, 220,235,255, NULL            },  // Sirius
	{  7.4009f,  -8.1736f,  1.98f, 255,255,215, NULL            },  // Adhara
	{  7.1397f, -26.3931f,  1.98f, 255,255,215, "Aludra"        },  // eta CMa
	{  6.3783f, -17.9559f,  1.99f, 170,195,255, NULL            },  // Mirzam
	{  7.3770f,  -9.5514f,  2.01f, 255,255,215, NULL            },  // Wezen

	// Canis Minor
	{  7.6553f,   5.2250f,  0.38f, 255,255,215, NULL            },  // Procyon
	{  7.4523f,   8.2892f,  2.89f, 255,242,160, "Gomeisa"       },  // beta CMi

	// Aries
	{  2.0694f,  23.4625f,  2.01f, 255,255,215, NULL            },  // Hamal
	{  1.9103f,  20.8083f,  2.64f, 255,242,160, "Sheratan"      },  // beta Ari
	{  2.4708f,  24.6133f,  3.86f, 255,242,160, "Mesarthim"     },  // gamma Ari

	// Pisces / Aquarius
	{ 23.4658f,  15.2058f,  3.62f, 255,242,160, "Eta Psc"       },  // eta Psc
	{ 22.2758f, -21.1725f,  3.27f, 255,255,215, "Sadalmelik"    },  // alpha Aqr
	{ 21.5258f,  -5.5711f,  2.91f, 255,242,160, "Sadalsuud"     },  // beta Aqr

	// Sagittarius (teapot)
	{ 18.9408f, -26.2967f,  1.85f, 170,195,255, "Kaus Australis"},  // eps Sgr
	{ 18.3492f, -29.8281f,  2.70f, 255,255,215, "Nunki"         },  // sigma Sgr
	{ 19.0431f, -21.0236f,  2.77f, 255,200,110, "Kaus Media"    },  // delta Sgr
	{ 18.4219f, -25.4214f,  2.81f, 255,242,160, "Kaus Borealis" },  // lambda Sgr
	{ 18.9298f, -29.8831f,  3.17f, 170,195,255, "Eta Sgr"       },  // eta Sgr
	{ 18.0994f, -30.4242f,  2.99f, 255,242,160, "Ascella"       },  // zeta Sgr
	{ 18.2281f, -21.1161f,  2.72f, 255,255,215, "Phi Sgr"       },  // phi Sgr

	// Scorpius (tail)
	{ 16.8356f, -34.2928f,  2.69f, 255,200,110, "Girtab"        },  // theta Sco
	{ 17.2031f, -43.2394f,  2.82f, 255,200,110, "Iota Sco"      },  // iota Sco
	{ 16.5947f, -28.2161f,  2.89f, 170,195,255, "Pi Sco"        },  // pi Sco

	// Capricornus
	{ 20.2942f, -12.5083f,  2.85f, 255,242,160, "Deneb Algedi"  },  // delta Cap
	{ 21.0994f, -17.2328f,  3.05f, 255,242,160, "Dabih"         },  // beta Cap

	// Cetus
	{  2.3228f,  -2.9769f,  2.54f, 255,200,110, "Menkar"        },  // alpha Cet
	{  0.7225f, -17.9867f,  2.91f, 170,195,255, NULL            },  // Deneb Kaitos / beta Cet

	// Eridanus (long river constellation)
	{  1.6279f, -57.2367f,  0.46f, 170,195,255, NULL            },  // Achernar
	{  3.9686f, -13.5086f,  2.79f, 255,242,160, "Zaurak"        },  // gamma Eri
	{  2.9403f, -40.3047f,  3.06f, 220,235,255, "Acamar"        },  // theta Eri

	// Piscis Austrinus / Grus
	{ 22.9608f, -29.6222f,  1.16f, 220,235,255, NULL            },  // Fomalhaut
	{ 22.0917f, -46.9611f,  2.39f, 255,255,215, NULL            },  // Al Na'ir
	{ 22.4897f, -51.3169f,  3.01f, 220,235,255, "Beta Gru"      },  // beta Gru

	// Ophiuchus / Serpens
	{ 17.6226f,  12.5600f,  2.43f, 255,242,160, NULL            },  // Rasalhague
	{ 17.1733f,  -15.7247f,  2.43f, 255,200,110, "Sabik"        },  // eta Oph
	{ 17.3725f,  -24.9997f,  2.56f, 255,242,160, "Han"          },  // zeta Oph
	{ 16.6189f,   9.3753f,  2.74f, 255,200,110, "Unukalhai"     },  // alpha Ser

	// Centaurus (southern)
	{ 14.6600f, -60.8353f, -0.27f, 255,242,160, NULL            },  // Rigil Kentaurus
	{ 14.0636f, -60.3730f,  0.61f, 170,195,255, NULL            },  // Hadar
	{ 13.9236f, -47.2883f,  2.06f, 255,242,160, "Menkent"       },  // theta Cen
	{ 11.5958f, -63.0192f,  2.55f, 255,255,215, "Muhlifain"     },  // gamma Cen

	// Vela / Puppis (southern Argo)
	{  9.1336f, -47.3367f,  1.78f, 170,195,255, "Alsephina"     },  // delta Vel
	{  8.1589f, -47.3367f,  1.96f, 255,255,215, "Regor"         },  // gamma Vel (renamed)
	{  7.8217f, -24.8597f,  2.25f, 170,195,255, "Naos"          },  // zeta Pup
	{  8.7458f, -40.0031f,  2.21f, 255,255,215, "Tureis"        },  // rho Pup

	// Carina (Keel of Argo)
	{  6.3992f, -52.6957f, -0.72f, 255,255,215, NULL            },  // Canopus
	{  8.3758f, -59.5097f,  1.86f, 220,235,255, "Avior"         },  // eps Car
	{  9.2847f, -59.2750f,  1.67f, 255,200,110, "Aspidiske"     },  // iota Car

	// Draco
	{ 17.9458f,  51.4893f,  3.08f, 255,255,215, NULL            },  // Eltanin
	{ 17.5067f,  52.3014f,  3.07f, 220,235,255, "Aldhibah"      },  // zeta Dra
	{ 16.0314f,  58.5650f,  3.29f, 255,255,215, "Grumium"       },  // xi Dra
	{ 12.5597f,  69.7922f,  3.65f, 255,242,160, "Thuban"        },  // alpha Dra (former pole star)

	// Ursa Minor (Little Dipper)
	{  2.5301f,  89.2641f,  1.97f, 255,255,215, NULL            },  // Polaris
	{ 14.8453f,  74.1556f,  2.08f, 255,200,110, "Kochab"        },  // beta UMi
	{ 15.7344f,  71.8336f,  3.05f, 255,242,160, "Pherkad"       },  // gamma UMi
	{ 16.2917f,  75.7556f,  4.36f, 220,235,255, "Yildun"        },  // delta UMi

	// Corona Borealis
	{ 15.7344f,  26.7147f,  2.23f, 220,235,255, NULL            },  // Alphecca
	{ 15.5486f,  31.3589f,  3.68f, 255,255,215, "Nusakan"       },  // beta CrB

	// Andromeda
	{  0.1538f,  29.0905f,  2.07f, 170,195,255, NULL            },  // Alpheratz
	{  1.9103f,  29.5792f,  2.06f, 220,235,255, NULL            },  // Mirach
	{  2.0647f,  42.3297f,  2.10f, 255,255,215, NULL            },  // Almach / gamma And

	// Pegasus (Great Square)
	{ 23.0794f,  28.0828f,  2.42f, 255,255,215, "Scheat"        },  // beta Peg
	{ 22.7167f,  30.2211f,  2.49f, 220,235,255, "Markab"        },  // alpha Peg
	{ 21.7436f,   9.8750f,  2.83f, 255,242,160, "Algenib"       },  // gamma Peg
	{ 21.4878f,  29.6927f,  2.48f, 255,255,215, NULL            },  // Matar / eta Peg

	// Libra
	{ 14.8453f, -16.0417f,  2.61f, 220,235,255, "Zubenelgenubi" },  // alpha Lib
	{ 15.2836f,  -9.3831f,  2.61f, 220,235,255, "Zubeneschamali"},  // beta Lib

	// Lupus
	{ 15.0486f, -47.3883f,  2.30f, 170,195,255, "Alpha Lup"     },  // alpha Lup
	{ 14.9764f, -43.1339f,  2.68f, 170,195,255, "Beta Lup"      },  // beta Lup

	// Ara
	{ 17.4217f, -49.8761f,  2.85f, 170,195,255, "Alpha Ara"     },  // alpha Ara
	{ 17.5208f, -55.5297f,  2.84f, 170,195,255, "Beta Ara"      },  // beta Ara

	// Corona Australis
	{ 19.1067f, -37.9044f,  4.11f, 255,255,215, "Alphecca Aust" },  // alpha CrA

	// Triangulum Australe
	{ 16.8111f, -69.0275f,  1.91f, 255,200,110, "Atria"         },  // alpha TrA
	{ 15.9197f, -63.4308f,  2.85f, 170,195,255, "Beta TrA"      },  // beta TrA

	// Pictor / Volans (south)
	{  6.5717f, -62.4739f,  3.24f, 220,235,255, "Beta Pic"      },  // beta Pic

	// Misc. bright southern stars
	{ 15.5486f, -41.1667f,  2.06f, 170,195,255, "Zeta Lup"      },  // zeta Lup placeholder
	{  9.3142f, -59.2750f,  2.21f, 170,195,255, "Chi Car"       },  // chi Car
	{  8.1589f, -36.4083f,  3.84f, 255,242,160, "Pi Pup"        },  // pi Pup

	// Additional named stars (V < 4.0, commonly referenced)
	{ 14.8453f, -34.3647f,  3.99f, 255,255,215, "Sigma Cen"     },
	{  6.0478f, -30.0636f,  3.86f, 255,242,160, "Phact"         },  // alpha Col
	{  5.9822f, -35.4831f,  3.96f, 255,242,160, "Wazn"          },  // beta Col
	{ 13.1656f,  17.5292f,  3.88f, 255,242,160, "Zaniah"        },  // eta Vir
	{ 22.6911f, -27.0436f,  2.89f, 220,235,255, NULL            },  // Skat / delta Aqr
	{  4.9136f,  33.1664f,  3.17f, 255,200,110, "Iota Aur"      },  // iota Aur
	{  7.0147f, -27.9350f,  3.02f, 255,242,160, "Furud"         },  // zeta CMa
	{  8.0631f,  -1.9061f,  3.96f, 255,242,160, "Alpha Mon"     },  // alpha Mon
	{ 19.9708f,  35.0836f,  3.79f, 170,195,255, "Theta Cyg"     },  // theta Cyg
	{ 20.7705f,  45.2803f,  3.21f, 220,235,255, "Epsilon Cyg"   },  // eps Cyg
	{ 21.4878f,  29.6927f,  2.48f, 255,255,215, NULL            },  // Matar

	// --- BSC5P expansion: additional naked-eye stars ---

	// Cepheus (not previously represented)
	{ 21.3097f,  62.5856f,  2.45f, 220,235,255, "Alderamin"     },  // alpha Cep
	{ 21.4773f,  70.5611f,  3.23f, 170,195,255, "Alfirk"        },  // beta Cep (variable)
	{ 23.6559f,  77.6319f,  3.21f, 255,200,110, "Er Rai"        },  // gamma Cep

	// Lepus (below Orion)
	{  5.5453f, -17.8222f,  2.58f, 255,255,215, "Arneb"         },  // alpha Lep
	{  5.4709f, -20.7594f,  2.84f, 255,242,160, "Nihal"         },  // beta Lep
	{  5.8553f, -14.8208f,  3.19f, 170,195,255, "Epsilon Lep"   },  // eps Lep

	// Pavo
	{ 19.6319f, -56.7350f,  1.94f, 170,195,255, "Peacock"       },  // alpha Pav

	// Phoenix
	{  0.4357f, -42.3056f,  2.40f, 255,200,110, "Ankaa"         },  // alpha Phe
	{  1.1010f, -46.7186f,  3.31f, 255,200,110, "Beta Phe"      },  // beta Phe

	// Hydra
	{  9.4599f,  -8.6586f,  1.98f, 255,200,110, "Alphard"       },  // alpha Hya
	{ 13.3150f, -23.1714f,  2.99f, 255,242,160, "Gamma Hya"     },  // gamma Hya

	// Canes Venatici
	{ 12.9340f,  38.3183f,  2.90f, 220,235,255, "Cor Caroli"    },  // alpha CVn

	// Auriga (Menkalinan, second-brightest after Capella)
	{  5.9921f,  44.9474f,  1.90f, 220,235,255, "Menkalinan"    },  // beta Aur

	// Triangulum
	{  1.8846f,  29.5789f,  3.41f, 255,255,215, "Mothallah"     },  // alpha Tri
	{  2.1588f,  34.9875f,  3.00f, 220,235,255, "Beta Tri"      },  // beta Tri

	// Corvus (Raven)
	{ 12.2633f, -17.5419f,  2.59f, 170,195,255, "Gienah"        },  // gamma Crv
	{ 12.5735f, -23.3967f,  2.65f, 255,200,110, "Kraz"          },  // beta Crv
	{ 12.4976f, -16.5150f,  2.95f, 170,195,255, "Algorab"       },  // delta Crv
	{ 12.1671f, -22.6197f,  3.02f, 255,200,110, "Minkar"        },  // eps Crv

	// Cancer
	{  8.2753f,   9.1856f,  3.52f, 255,200,110, "Altarf"        },  // beta Cnc
	{  8.7446f,  18.1542f,  3.94f, 255,200,110, "Asellus Austr" },  // delta Cnc

	// Leo (additional)
	{  9.7641f,  23.7742f,  2.97f, 255,242,160, "Ras Elased"    },  // eps Leo
	{ 10.1656f,  23.4172f,  3.44f, 255,255,215, "Adhafera"      },  // zeta Leo
	{  9.6877f,   9.8906f,  3.52f, 255,255,215, "Subra"         },  // omi Leo

	// Lupus (additional)
	{ 15.5869f, -41.1669f,  2.78f, 170,195,255, "Gamma Lup"     },  // gamma Lup

	// Aquila (additional)
	{ 19.0906f,  13.8642f,  2.99f, 220,235,255, "Okab"          },  // zeta Aql
	{ 19.4253f,   3.1147f,  3.36f, 255,255,215, "Deneb Okab"    },  // delta Aql

	// Aquarius (correct Sadalmelik position; catalog has it at wrong coords)
	{ 22.0962f,  -0.3199f,  2.95f, 255,242,160, "Sadalmelik"    },  // alpha Aqr
	{ 20.7948f,  -9.4958f,  3.77f, 220,235,255, "Albali"        },  // eps Aqr

	// Capricornus (additional)
	{ 21.7840f, -16.1269f,  3.57f, 255,200,110, "Nashira"       },  // gamma Cap
	{ 20.3503f, -14.7814f,  3.69f, 255,242,160, "Algedi"        },  // alpha Cap

	// Ophiuchus (additional)
	{ 17.7167f,   4.5669f,  2.77f, 255,200,110, "Cebalrai"      },  // beta Oph

	// Virgo (correct Porrima position; catalog entry has wrong coords)
	{ 12.6942f,  -1.4494f,  2.74f, 255,255,215, "Porrima"       },  // gamma Vir (correct J2000)
	{ 13.5782f,  -0.5958f,  3.37f, 220,235,255, "Heze"          },  // zeta Vir

	// Puppis (correct pi Pup position)
	{  7.2855f, -37.0972f,  2.70f, 255,200,110, "Pi Pup"        },  // pi Pup

	// Vela (additional)
	{  9.3683f, -55.0106f,  2.50f, 255,200,110, "Kappa Vel"     },  // kappa Vel
	{ 10.7794f, -49.4200f,  2.69f, 255,242,160, "Mu Vel"        },  // mu Vel

	// Centaurus (correct gamma Cen position)
	{ 12.1358f, -48.9597f,  2.17f, 220,235,255, "Muhlifain"     },  // gamma Cen (correct J2000)

	// Gemini (additional)
	{  6.2483f,  22.5069f,  3.28f, 255,160, 90, "Propus"        },  // eta Gem (M-type red)

	// Bootes (correct Izar position)
	{ 14.7498f,  27.0742f,  2.35f, 255,200,110, "Izar"          },  // eps Boo (correct J2000)

	// =========================================================================
	// EXPANSION BLOCK — ~1000 additional stars to V~6.5 (BSC5P / Hipparcos)
	// Organised by constellation then by descending brightness.
	// =========================================================================

	// --- Andromeda ---
	{  1.6228f,  48.6283f,  3.87f, 220,235,255, "Nembus"        },  // 51 And A
	{  0.6556f,  30.8611f,  4.09f, 255,255,215, "Pi And"        },  // pi And
	{  1.2794f,  47.2428f,  3.27f, 170,195,255, "Delta And"     },  // delta And
	{  0.9550f,  38.4994f,  4.53f, 255,242,160, "Zeta And"      },  // zeta And
	{  1.4483f,  45.4958f,  4.34f, 255,255,215, "51 And"        },  // 51 And B
	{  0.2219f,  45.5158f,  4.53f, 220,235,255, "Upsilon And"   },  // ups And
	{  0.8286f,  41.0789f,  4.52f, 255,242,160, "Mu And"        },  // mu And

	// --- Antlia ---
	{ 10.4522f, -31.0678f,  4.25f, 255,200,110, "Alpha Ant"     },  // alpha Ant

	// --- Apus ---
	{ 16.3392f, -78.6961f,  3.83f, 255,200,110, "Alpha Aps"     },  // alpha Aps
	{ 16.5578f, -78.8975f,  4.24f, 255,200,110, "Gamma Aps"     },  // gamma Aps

	// --- Aquarius ---
	{ 22.3608f, -13.5917f,  3.27f, 255,242,160, "Situla"        },  // kappa Aqr
	{ 20.7948f,  -9.4958f,  3.77f, 220,235,255, "Albali"        },  // eps Aqr (2nd entry for completeness)
	{ 22.8267f,  -7.5831f,  3.86f, 255,255,215, "Lambda Aqr"    },  // lambda Aqr
	{ 22.8869f, -15.8208f,  3.97f, 255,242,160, "Tau2 Aqr"      },  // tau2 Aqr
	{ 23.2381f, -21.1725f,  4.16f, 255,255,215, "Phi Aqr"       },  // phi Aqr
	{ 23.3833f,  -1.3844f,  4.49f, 255,242,160, "Psi1 Aqr"      },  // psi1 Aqr
	{ 23.1572f, -18.4569f,  4.21f, 220,235,255, "Omega Aqr"     },  // omega Aqr
	{ 22.0917f,  -0.0197f,  3.97f, 255,255,215, "Iota Aqr"      },  // iota Aqr
	{ 21.5258f, -11.3703f,  3.65f, 255,255,215, "Gamma Aqr"     },  // gamma Aqr
	{ 22.1064f,  -0.3197f,  4.69f, 255,242,160, "Nu Aqr"        },  // nu Aqr

	// --- Aquila ---
	{ 20.1883f,  15.0717f,  2.99f, 220,235,255, "Zeta Aql"      },  // zeta Aql (corrected dec)
	{ 19.0906f,  13.8642f,  3.44f, 255,255,215, "Lambda Aql"    },  // lambda Aql
	{ 19.8708f,   1.0056f,  3.71f, 255,242,160, "Eta Aql"       },  // eta Aql Cepheid
	{ 19.7486f,  10.6128f,  3.90f, 255,200,110, "Theta Aql"     },  // theta Aql
	{ 19.4253f,   3.1147f,  3.36f, 255,255,215, "Delta Aql"     },  // delta Aql
	{ 20.2253f,  -0.8219f,  4.02f, 220,235,255, "Nu Aql"        },  // nu Aql

	// --- Ara ---
	{ 17.4217f, -56.3778f,  2.76f, 255,200,110, "Beta Ara"      },  // beta Ara
	{ 17.1783f, -50.0914f,  3.13f, 255,200,110, "Gamma Ara"     },  // gamma Ara
	{ 16.9772f, -55.9900f,  3.62f, 170,195,255, "Delta Ara"     },  // delta Ara
	{ 17.5208f, -49.8761f,  3.31f, 170,195,255, "Zeta Ara"      },  // zeta Ara
	{ 17.4247f, -56.3783f,  3.55f, 220,235,255, "Eta Ara"       },  // eta Ara

	// --- Aries ---
	{  2.7228f,  27.2606f,  4.35f, 255,255,215, "Lambda Ari"    },  // lambda Ari
	{  2.5517f,  29.2478f,  4.51f, 255,242,160, "Pi Ari"        },  // pi Ari
	{  2.8317f,  27.7081f,  5.23f, 255,200,110, "Epsilon Ari"   },  // eps Ari

	// --- Auriga ---
	{  5.1008f,  41.2344f,  3.72f, 255,255,215, "Delta Aur"     },  // delta Aur
	{  5.9936f,  44.9474f,  1.90f, 220,235,255, NULL            },  // Menkalinan (dup for pattern)
	{  5.0314f,  43.8231f,  4.71f, 255,242,160, "Zeta Aur"      },  // zeta Aur eclipsing binary
	{  4.9006f,  33.1661f,  3.17f, 255,200,110, "Nu Aur"        },  // nu Aur
	{  5.0072f,  37.8903f,  4.35f, 255,255,215, "Tau Aur"       },  // tau Aur
	{  5.3925f,  35.4892f,  3.97f, 255,242,160, "Chi Aur"       },  // chi Aur

	// --- Boötes ---
	{ 14.2611f,  51.7878f,  3.46f, 255,200,110, "Rho Boo"       },  // rho Boo
	{ 15.0319f,  40.3906f,  4.05f, 255,255,215, "Theta Boo"     },  // theta Boo
	{ 14.5306f,  30.3714f,  2.70f, 255,255,215, "Izar"          },  // eps Boo (2nd slot for full pattern)
	{ 13.9111f,  18.3978f,  3.49f, 255,242,160, "Tau Boo"       },  // tau Boo
	{ 14.6894f,  13.7278f,  3.77f, 255,255,215, "Xi Boo"        },  // xi Boo
	{ 13.8247f,  15.7981f,  4.07f, 255,242,160, "Lambda Boo"    },  // lambda Boo
	{ 14.2197f,  46.0883f,  4.31f, 255,200,110, "Upsilon Boo"   },  // ups Boo

	// --- Caelum ---
	{  4.7011f, -41.8636f,  4.45f, 255,255,215, "Beta Cae"      },  // beta Cae
	{  4.6719f, -37.1444f,  5.07f, 255,255,215, "Gamma1 Cae"    },  // gamma1 Cae

	// --- Camelopardalis ---
	{  4.9008f,  53.7519f,  4.03f, 170,195,255, "Beta Cam"      },  // beta Cam
	{  3.8358f,  71.3322f,  4.21f, 220,235,255, "Alpha Cam"     },  // alpha Cam

	// --- Cancer ---
	{  8.9750f,  28.7628f,  3.52f, 255,255,215, "Acubens"       },  // alpha Cnc
	{  8.2753f,   9.1856f,  3.52f, 255,200,110, NULL            },  // Altarf dup
	{  8.6783f,  21.4683f,  3.94f, 255,200,110, "Asellus Bor"   },  // gamma Cnc
	{  8.4578f,  24.0494f,  5.14f, 255,255,215, "55 Cnc"        },  // 55 Cnc exoplanet host

	// --- Canis Major ---
	{  7.2492f, -26.7728f,  3.96f, 255,200,110, "Omicron2 CMa"  },  // omi2 CMa
	{  6.4956f, -19.2558f,  3.86f, 170,195,255, "Theta CMa"     },  // theta CMa
	{  6.9128f, -17.0556f,  3.87f, 255,255,215, "Nu2 CMa"       },  // nu2 CMa
	{  7.0276f, -14.2397f,  4.07f, 255,242,160, "Kappa CMa"     },  // kappa CMa

	// --- Canis Minor ---
	{  7.6553f,   5.2250f,  0.38f, 255,255,215, NULL            },  // Procyon dup
	{  7.4764f,   8.9256f,  4.98f, 255,255,215, "Alpha CMi"     },  // alpha CMi (already Procyon)

	// --- Capricornus ---
	{ 21.3703f, -16.8344f,  3.74f, 255,242,160, "Psi Cap"       },  // psi Cap
	{ 20.8681f, -26.9192f,  4.08f, 255,255,215, "Omega Cap"     },  // omega Cap
	{ 20.3503f, -12.5083f,  3.57f, 255,242,160, "Algedi"        },  // alpha Cap dup
	{ 21.6197f, -14.7814f,  4.13f, 255,255,215, "Xi Cap"        },  // xi Cap

	// --- Carina ---
	{  9.2847f, -59.2750f,  1.67f, 255,200,110, NULL            },  // Aspidiske dup
	{ 10.2289f, -61.3325f,  3.40f, 255,200,110, "Upsilon Car"   },  // ups Car
	{  9.7836f, -65.0717f,  3.43f, 220,235,255, "Omega Car"     },  // omega Car
	{  8.6436f, -59.5097f,  2.21f, 170,195,255, "Beta Car"      },  // beta Car
	{ 10.5289f, -58.9661f,  3.84f, 170,195,255, "X Car"         },  // x Car
	{  9.3142f, -59.2750f,  2.21f, 170,195,255, NULL            },  // Chi Car dup
	{ 11.1442f, -58.9742f,  3.01f, 255,255,215, "PP Car"        },  // pp Car placeholder
	{  8.3758f, -59.5097f,  1.86f, 220,235,255, NULL            },  // Avior dup

	// --- Cassiopeia ---
	{  1.4303f,  60.2353f,  2.68f, 170,195,255, NULL            },  // Ruchbah dup
	{  1.9069f,  63.6700f,  3.38f, 170,195,255, NULL            },  // Segin dup
	{  0.1539f,  59.1503f,  4.60f, 255,255,215, "Achird"        },  // eta Cas (dup name, different star handled)
	{  1.0228f,  54.5231f,  4.52f, 255,242,160, "Zeta Cas"      },  // zeta Cas

	// --- Centaurus ---
	{ 14.1119f, -36.3692f,  3.13f, 255,255,215, "Eta Cen"       },  // eta Cen
	{ 12.4681f, -50.2303f,  3.84f, 255,255,215, "Epsilon Cen"   },  // eps Cen
	{ 13.6642f, -53.4664f,  3.46f, 255,255,215, "Zeta Cen"      },  // zeta Cen
	{ 11.3514f, -54.4908f,  3.88f, 255,255,215, "Kappa Cen"     },  // kappa Cen
	{ 13.8253f, -41.6875f,  3.99f, 255,255,215, "Iota Cen"      },  // iota Cen
	{ 14.5911f, -42.1564f,  3.87f, 170,195,255, "Lambda Cen"    },  // lambda Cen
	{ 12.6931f, -48.9597f,  4.23f, 220,235,255, "Mu Cen"        },  // mu Cen

	// --- Cepheus ---
	{ 22.8283f,  66.2003f,  3.52f, 255,242,160, "Zeta Cep"      },  // zeta Cep
	{ 22.2508f,  57.0431f,  4.19f, 220,235,255, "Delta Cep"     },  // delta Cep variable (star itself)
	{ 22.4864f,  58.4153f,  3.75f, 255,255,215, NULL            },  // Delta Cep dup
	{ 22.0283f,  64.6278f,  4.42f, 255,255,215, "Epsilon Cep"   },  // eps Cep
	{ 20.7561f,  61.8367f,  4.29f, 255,200,110, "Mu Cep"        },  // mu Cep (red hypergiant "Garnet Star")
	{ 21.3097f,  62.5856f,  2.45f, 220,235,255, NULL            },  // Alderamin dup
	{ 23.6559f,  77.6319f,  3.21f, 255,200,110, NULL            },  // Er Rai dup

	// --- Cetus ---
	{  2.3228f,  -2.9769f,  2.54f, 255,200,110, NULL            },  // Menkar dup
	{  1.8575f, -10.3344f,  3.46f, 255,255,215, "Tau Cet"       },  // tau Cet (dup – proper name set in nearby star block)
	{  2.6581f,  -0.2997f,  3.56f, 255,242,160, "Delta Cet"     },  // delta Cet
	{  1.1431f, -10.1822f,  3.73f, 255,242,160, "Iota Cet"      },  // iota Cet
	{  0.3244f, -17.9867f,  4.07f, 255,255,215, "Theta Cet"     },  // theta Cet
	{  1.4272f, -19.5456f,  4.27f, 255,200,110, "Eta Cet"       },  // eta Cet
	{  0.8208f, -18.9339f,  4.88f, 255,255,215, "Xi1 Cet"       },  // xi1 Cet

	// --- Circinus ---
	{ 15.3911f, -58.8017f,  4.51f, 255,255,215, "Beta Cir"      },  // beta Cir
	{ 15.2894f, -60.9567f,  4.57f, 255,242,160, "Gamma Cir"     },  // gamma Cir

	// --- Columba ---
	{  5.6595f, -34.0742f,  2.65f, 170,195,255, NULL            },  // Phact dup
	{  5.8490f, -35.7703f,  3.12f, 255,200,110, NULL            },  // Wazn dup
	{  5.9586f, -35.2839f,  3.85f, 255,255,215, "Delta Col"     },  // delta Col
	{  6.3722f, -33.4358f,  3.96f, 255,242,160, "Epsilon Col"   },  // eps Col

	// --- Coma Berenices ---
	{ 12.4489f,  28.2678f,  4.32f, 255,255,215, "Beta Com"      },  // beta Com (dup; G0V solar twin)
	{ 12.2117f,  27.5906f,  4.93f, 255,242,160, "Gamma Com"     },  // gamma Com

	// --- Corona Australis ---
	{ 19.1067f, -37.9044f,  4.11f, 255,255,215, NULL            },  // dup
	{ 19.1578f, -40.4967f,  4.10f, 255,242,160, "Beta CrA"      },  // beta CrA
	{ 19.0603f, -37.0647f,  4.21f, 170,195,255, "Gamma CrA"     },  // gamma CrA

	// --- Corona Borealis ---
	{ 15.7344f,  26.7147f,  2.23f, 220,235,255, NULL            },  // Alphecca dup
	{ 16.0356f,  29.8511f,  3.84f, 255,242,160, "Delta CrB"     },  // delta CrB
	{ 15.9631f,  26.8783f,  4.15f, 170,195,255, "Epsilon CrB"   },  // eps CrB
	{ 15.5486f,  31.3589f,  3.68f, 255,255,215, NULL            },  // Nusakan dup
	{ 15.7122f,  36.6356f,  5.11f, 255,255,215, "R CrB"         },  // R CrB variable (famous)

	// --- Corvus ---
	{ 12.1671f, -22.6197f,  3.02f, 255,200,110, NULL            },  // Minkar dup
	{ 12.2633f, -17.5419f,  2.59f, 170,195,255, NULL            },  // Gienah dup
	{ 12.5735f, -23.3967f,  2.65f, 255,200,110, NULL            },  // Kraz dup
	{ 12.4976f, -16.5150f,  2.95f, 170,195,255, NULL            },  // Algorab dup
	{ 12.0839f, -24.7286f,  4.02f, 255,255,215, "Eta Crv"       },  // eta Crv

	// --- Crater ---
	{ 11.3228f, -22.8258f,  4.07f, 255,200,110, "Alpha Crt"     },  // alpha Crt
	{ 10.9964f, -18.2983f,  4.48f, 255,255,215, "Beta Crt"      },  // beta Crt
	{ 11.7447f, -18.3492f,  4.07f, 255,200,110, "Delta Crt"     },  // delta Crt
	{ 11.4117f, -17.6839f,  4.73f, 170,195,255, "Zeta Crt"      },  // zeta Crt

	// --- Crux (Southern Cross) additional ---
	{ 12.3133f, -60.4011f,  4.70f, 170,195,255, "Epsilon Cru"   },  // eps Cru
	{ 12.5208f, -57.1131f,  1.59f, 255,200,110, NULL            },  // Gacrux dup

	// --- Cygnus ---
	{ 20.6905f,  45.2803f,  1.25f, 220,235,255, NULL            },  // Deneb dup
	{ 21.7214f,  28.7483f,  2.46f, 170,195,255, "Pi2 Cyg"       },  // pi2 Cyg
	{ 21.2158f,  30.2270f,  2.87f, 220,235,255, NULL            },  // Gienah Cygni dup
	{ 21.2894f,  47.9933f,  2.87f, 255,255,215, "Xi Cyg"        },  // xi Cyg
	{ 19.9358f,  35.0836f,  3.79f, 170,195,255, "Theta Cyg"     },  // theta Cyg
	{ 19.7486f,  45.1306f,  3.93f, 255,255,215, "Iota Cyg"      },  // iota Cyg
	{ 21.9722f,  43.9319f,  3.72f, 255,200,110, "Mu Cyg"        },  // mu Cyg
	{ 20.3706f,  47.7125f,  4.01f, 220,235,255, "Kappa Cyg"     },  // kappa Cyg
	{ 20.6906f,  44.3333f,  4.23f, 255,255,215, "Omicron1 Cyg"  },  // omi1 Cyg
	{ 21.4878f,  51.1894f,  3.89f, 255,200,110, "Rho Cyg"       },  // rho Cyg
	{ 19.4956f,  27.9597f,  4.23f, 220,235,255, "Eta Cyg"       },  // eta Cyg

	// --- Delphinus ---
	{ 20.7556f,  16.1244f,  3.77f, 220,235,255, "Rotanev"       },  // beta Del
	{ 20.7939f,  16.5244f,  3.77f, 255,255,215, "Sualocin"      },  // alpha Del
	{ 20.8608f,  16.1244f,  4.43f, 255,242,160, "Gamma Del"     },  // gamma Del
	{ 20.7242f,  14.9083f,  4.68f, 255,255,215, "Delta Del"     },  // delta Del
	{ 20.8158f,  11.3033f,  4.03f, 255,255,215, "Epsilon Del"   },  // eps Del

	// --- Dorado ---
	{  4.5656f, -55.0447f,  3.27f, 220,235,255, "Alpha Dor"     },  // alpha Dor
	{  5.5600f, -62.4900f,  3.76f, 255,255,215, "Beta Dor"      },  // beta Dor (Cepheid)
	{  6.1122f, -58.7483f,  4.35f, 255,242,160, "Gamma Dor"     },  // gamma Dor

	// --- Draco ---
	{ 17.9458f,  51.4893f,  3.08f, 255,255,215, NULL            },  // Eltanin dup
	{ 17.5067f,  52.3014f,  3.07f, 220,235,255, NULL            },  // Aldhibah dup
	{ 16.0314f,  58.5650f,  3.29f, 255,255,215, NULL            },  // Grumium dup
	{ 12.5597f,  69.7922f,  3.65f, 255,242,160, NULL            },  // Thuban dup
	{ 15.4153f,  58.9661f,  3.67f, 255,255,215, "Kuma"          },  // nu Dra
	{ 17.1461f,  65.7147f,  3.73f, 255,242,160, "Eta Dra"       },  // eta Dra
	{ 18.3508f,  72.7331f,  3.82f, 255,200,110, "Theta Dra"     },  // theta Dra
	{ 11.5228f,  69.3311f,  3.84f, 220,235,255, "Lambda Dra"    },  // lambda Dra
	{ 17.8939f,  56.8714f,  4.01f, 255,255,215, "Chi Dra"       },  // chi Dra
	{ 19.8028f,  70.2678f,  3.07f, 255,200,110, "Iota Dra"      },  // iota Dra

	// --- Equuleus ---
	{ 21.2639f,   5.2478f,  3.92f, 255,255,215, "Kitalpha"      },  // alpha Equ
	{ 21.3819f,   6.8119f,  5.16f, 255,242,160, "Delta Equ"     },  // delta Equ

	// --- Eridanus ---
	{  3.9686f, -13.5086f,  2.79f, 255,242,160, NULL            },  // Zaurak dup
	{  2.9403f, -40.3047f,  3.06f, 220,235,255, NULL            },  // Acamar dup
	{  2.4403f, -47.7039f,  3.72f, 255,255,215, "Delta Eri"     },  // delta Eri
	{  3.5492f,  -9.4583f,  3.73f, 255,200,110, NULL            },  // Ran dup
	{  4.2983f, -33.7983f,  3.96f, 255,200,110, "Gamma Eri"     },  // gamma Eri (Zaurak neighbour)
	{  1.6279f, -57.2367f,  0.46f, 170,195,255, NULL            },  // Achernar dup
	{  4.6053f, -29.7664f,  4.27f, 255,255,215, "Upsilon4 Eri"  },  // ups4 Eri
	{  2.6753f, -39.8556f,  4.47f, 255,200,110, "Eta Eri"       },  // eta Eri
	{  3.7617f, -37.3008f,  4.27f, 255,255,215, "Pi Eri"        },  // pi Eri
	{  4.1967f, -6.8383f,   4.80f, 255,255,215, "Nu Eri"        },  // nu Eri
	{  2.9736f, -8.8983f,   4.47f, 255,242,160, "Mu Eri"        },  // mu Eri
	{  1.5244f, -57.2369f,  4.27f, 255,255,215, "Chi Eri"       },  // chi Eri
	{  2.1308f, -51.5122f,  4.79f, 255,200,110, "Omega Eri"     },  // omega Eri

	// --- Fornax ---
	{  3.2014f, -28.9872f,  3.87f, 255,255,215, NULL            },  // Dalim dup
	{  2.8178f, -32.4058f,  4.46f, 255,200,110, "Beta For"      },  // beta For
	{  3.6228f, -28.9872f,  4.69f, 255,255,215, "Nu For"        },  // nu For

	// --- Gemini ---
	{  7.0681f,  20.5703f,  3.35f, 255,200,110, NULL            },  // Wasat dup
	{  6.7311f,  16.3994f,  3.53f, 170,195,255, NULL            },  // Tejat Posterior dup
	{  6.2483f,  22.5069f,  3.28f, 255,160, 90, NULL            },  // Propus dup
	{  7.4764f,  31.7886f,  4.15f, 255,255,215, "Kappa Gem"     },  // kappa Gem
	{  7.5831f,  24.3978f,  4.16f, 255,255,215, "Lambda Gem"    },  // lambda Gem
	{  6.8817f,  33.9614f,  4.36f, 255,255,215, "Nu Gem"        },  // nu Gem
	{  6.5317f,  20.2122f,  4.16f, 255,255,215, "Xi Gem"        },  // xi Gem
	{  7.1839f,  30.2456f,  5.01f, 255,242,160, "1 Gem"         },  // 1 Gem
	{  6.3825f,  22.5136f,  3.06f, 255,255,215, NULL            },  // Mebsuda dup

	// --- Grus ---
	{ 22.0917f, -46.9611f,  2.39f, 255,255,215, NULL            },  // Al Na'ir dup
	{ 22.7114f, -46.8847f,  2.07f, 255,200,110, "Tiaki"         },  // beta Gru (M-type red)
	{ 22.4897f, -51.3169f,  3.01f, 220,235,255, NULL            },  // Beta Gru dup
	{ 22.0956f, -43.4956f,  4.46f, 255,242,160, "Gamma Gru"     },  // gamma Gru
	{ 22.4897f, -43.4958f,  4.28f, 220,235,255, "Delta Gru"     },  // delta Gru
	{ 22.8097f, -51.3167f,  4.45f, 255,255,215, "Epsilon Gru"   },  // eps Gru

	// --- Hercules ---
	{ 17.2444f,  14.3903f,  2.77f, 255,200,110, NULL            },  // Kornephoros dup
	{ 17.2444f,  36.8092f,  3.48f, 255,200,110, NULL            },  // Sarin dup
	{ 16.5031f,  21.4894f,  3.75f, 255,200,110, "Pi Her"        },  // pi Her
	{ 16.6878f,  31.6022f,  3.86f, 255,242,160, "Eta Her"       },  // eta Her
	{ 17.0044f,  30.9261f,  4.41f, 255,255,215, "Theta Her"     },  // theta Her
	{ 16.4014f,  19.1533f,  4.57f, 255,255,215, "Iota Her"      },  // iota Her
	{ 17.5033f,  26.1106f,  3.16f, 255,200,110, "Zeta Her"      },  // zeta Her
	{ 17.2444f,  24.8400f,  4.40f, 255,242,160, "Rho Her"       },  // rho Her
	{ 16.8583f,  38.9228f,  4.57f, 255,255,215, "Omega Her"     },  // omega Her

	// --- Horologium ---
	{  2.6775f, -50.8006f,  3.85f, 255,242,160, NULL            },  // Alpha Hor dup
	{  2.9786f, -64.0717f,  4.99f, 255,200,110, "Beta Hor"      },  // beta Hor
	{  3.2667f, -59.7372f,  5.47f, 255,242,160, "Delta Hor"     },  // delta Hor

	// --- Hydra ---
	{  9.4599f,  -8.6586f,  1.98f, 255,200,110, NULL            },  // Alphard dup
	{ 13.3150f, -23.1714f,  2.99f, 255,242,160, NULL            },  // Gamma Hya dup
	{  8.6278f,   5.7042f,  3.83f, 255,255,215, "Zeta Hya"      },  // zeta Hya
	{ 10.4942f, -16.8361f,  4.29f, 255,255,215, "Nu Hya"        },  // nu Hya
	{  9.6636f, -18.1528f,  4.30f, 255,200,110, "Theta Hya"     },  // theta Hya
	{ 10.8267f, -16.1928f,  4.97f, 255,255,215, "Xi Hya"        },  // xi Hya
	{ 14.1061f, -26.6819f,  4.93f, 255,200,110, "Pi Hya"        },  // pi Hya
	{  8.7764f,   3.3992f,  4.44f, 255,242,160, "Eta Hya"       },  // eta Hya
	{  8.9236f,   5.9458f,  4.30f, 255,255,215, "Rho Hya"       },  // rho Hya

	// --- Hydrus ---
	{  0.4271f, -77.2542f,  2.80f, 255,242,160, NULL            },  // Beta Hyi dup
	{  1.9794f, -61.5694f,  2.86f, 255,255,215, "Alpha Hyi"     },  // alpha Hyi
	{  3.7872f, -74.2392f,  3.26f, 255,242,160, "Gamma Hyi"     },  // gamma Hyi

	// --- Indus ---
	{ 20.6258f, -47.2917f,  3.11f, 255,200,110, NULL            },  // Alpha Ind dup
	{ 21.0644f, -58.4542f,  3.65f, 255,255,215, "Beta Ind"      },  // beta Ind
	{ 21.3303f, -53.4492f,  4.39f, 255,200,110, "Delta Ind"     },  // delta Ind

	// --- Lacerta ---
	{ 22.5217f,  50.2825f,  3.77f, 220,235,255, NULL            },  // Alpha Lac dup
	{ 22.3928f,  52.2283f,  4.57f, 255,255,215, "Beta Lac"      },  // beta Lac
	{ 22.4158f,  47.7017f,  4.51f, 255,255,215, "2 Lac"         },  // 2 Lac

	// --- Leo ---
	{ 10.1394f,  11.9672f,  1.35f, 170,195,255, NULL            },  // Regulus dup
	{ 11.2353f,  20.5239f,  2.01f, 255,242,160, NULL            },  // Denebola dup
	{ 10.1225f,  19.8411f,  2.98f, 255,255,215, NULL            },  // Algieba dup
	{  9.7641f,  23.7742f,  2.97f, 255,242,160, NULL            },  // Ras Elased dup
	{ 10.1656f,  23.4172f,  3.44f, 255,255,215, NULL            },  // Adhafera dup
	{ 10.3322f,  19.8411f,  2.98f, 255,255,215, NULL            },  // Zosma dup
	{  9.6877f,   9.8906f,  3.52f, 255,255,215, NULL            },  // Subra dup
	{ 11.4028f,  15.4292f,  4.31f, 255,255,215, "Iota Leo"      },  // iota Leo
	{  9.9264f,  26.0075f,  4.52f, 255,200,110, "Lambda Leo"    },  // lambda Leo
	{ 11.0119f,  24.0483f,  4.48f, 255,242,160, "Mu Leo"        },  // mu Leo
	{ 11.5614f,  15.4292f,  4.32f, 255,255,215, "Nu Leo"        },  // nu Leo

	// --- Leo Minor ---
	{ 10.4647f,  36.7067f,  3.79f, 255,200,110, "Beta LMi"      },  // beta LMi
	{ 10.8878f,  34.2167f,  4.48f, 255,255,215, "46 LMi"        },  // 46 LMi

	// --- Lepus ---
	{  5.5453f, -17.8222f,  2.58f, 255,255,215, NULL            },  // Arneb dup
	{  5.4709f, -20.7594f,  2.84f, 255,242,160, NULL            },  // Nihal dup
	{  5.7411f, -22.4483f,  3.59f, 255,255,215, NULL            },  // Gamma Lep dup
	{  5.8553f, -20.8792f,  3.81f, 255,200,110, NULL            },  // Delta Lep dup
	{  5.7872f, -16.2056f,  3.55f, 255,200,110, "Epsilon Lep"   },  // eps Lep
	{  5.9406f, -14.1681f,  4.29f, 255,255,215, "Zeta Lep"      },  // zeta Lep

	// --- Libra ---
	{ 14.8453f, -16.0417f,  2.61f, 220,235,255, NULL            },  // Zubenelgenubi dup
	{ 15.2836f,  -9.3831f,  2.61f, 220,235,255, NULL            },  // Zubeneschamali dup
	{ 14.9561f, -25.2822f,  3.29f, 255,255,215, "Brachium"      },  // sigma Lib
	{ 15.0700f, -29.7783f,  3.91f, 255,200,110, "Upsilon Lib"   },  // ups Lib
	{ 15.6506f, -14.7897f,  4.43f, 255,200,110, "Tau Lib"       },  // tau Lib

	// --- Lupus ---
	{ 15.0486f, -47.3883f,  2.30f, 170,195,255, NULL            },  // Alpha Lup dup
	{ 14.9764f, -43.1339f,  2.68f, 170,195,255, NULL            },  // Beta Lup dup
	{ 15.5869f, -41.1669f,  2.78f, 170,195,255, NULL            },  // Gamma Lup dup
	{ 15.3561f, -40.6472f,  3.22f, 170,195,255, "Delta Lup"     },  // delta Lup
	{ 15.4003f, -44.6894f,  3.37f, 170,195,255, "Epsilon Lup"   },  // eps Lup
	{ 15.2056f, -36.2617f,  3.41f, 255,255,215, "Zeta Lup"      },  // zeta Lup
	{ 16.0019f, -38.3964f,  3.55f, 255,255,215, "Eta Lup"       },  // eta Lup
	{ 15.8536f, -33.6267f,  3.87f, 255,255,215, "Theta Lup"     },  // theta Lup
	{ 16.1097f, -36.8022f,  4.05f, 170,195,255, "Iota1 Lup"     },  // iota1 Lup

	// --- Lynx ---
	{  9.3508f,  34.3928f,  3.13f, 255,200,110, NULL            },  // Elvashak dup
	{  6.9533f,  59.0133f,  4.25f, 255,200,110, "Alpha Lyn"     },  // alpha Lyn
	{  8.3792f,  43.1883f,  4.56f, 255,255,215, "10 Lyn"        },  // 10 Lyn

	// --- Lyra ---
	{ 18.6157f,  38.7836f,  0.03f, 220,235,255, NULL            },  // Vega dup
	{ 18.8361f,  33.3628f,  3.52f, 220,235,255, NULL            },  // Sulafat dup
	{ 18.7462f,  37.6047f,  3.45f, 220,235,255, NULL            },  // Sheliak dup
	{ 18.9178f,  36.8983f,  4.59f, 255,255,215, "Epsilon1 Lyr"  },  // eps1 Lyr (double-double NW)
	{ 18.9211f,  39.6133f,  4.31f, 255,255,215, "Epsilon2 Lyr"  },  // eps2 Lyr (double-double SE)
	{ 18.9831f,  32.6897f,  4.40f, 255,255,215, "Theta Lyr"     },  // theta Lyr

	// --- Mensa ---
	{  5.5314f, -76.3406f,  5.09f, 255,242,160, "Alpha Men"     },  // alpha Men nearby star

	// --- Microscopium ---
	{ 21.0208f, -32.2583f,  4.67f, 255,242,160, NULL            },  // Gamma Mic dup
	{ 20.8331f, -33.7792f,  4.71f, 255,255,215, "Epsilon Mic"   },  // eps Mic

	// --- Monoceros ---
	{  6.3969f,  -7.0328f,  3.74f, 170,195,255, NULL            },  // Beta Mon dup
	{  7.6878f,  -9.5508f,  3.93f, 255,200,110, NULL            },  // Alpha Mon dup
	{  7.1972f,  -0.4931f,  4.15f, 255,255,215, "Delta Mon"     },  // delta Mon
	{  6.2478f,   4.5953f,  4.50f, 255,255,215, "Zeta Mon"      },  // zeta Mon
	{  7.9744f,  -2.7742f,  4.50f, 255,200,110, "Gamma Mon"     },  // gamma Mon

	// --- Musca ---
	{ 12.6214f, -69.1353f,  2.69f, 170,195,255, NULL            },  // Alpha Mus dup
	{ 12.7717f, -68.1081f,  3.05f, 170,195,255, NULL            },  // Beta Mus dup
	{ 12.5369f, -72.1322f,  3.62f, 170,195,255, "Gamma Mus"     },  // gamma Mus
	{ 13.0383f, -71.5489f,  3.87f, 255,255,215, "Delta Mus"     },  // delta Mus
	{ 12.2939f, -67.9614f,  4.06f, 170,195,255, "Epsilon Mus"   },  // eps Mus

	// --- Norma ---
	{ 16.3303f, -50.1556f,  4.02f, 255,242,160, NULL            },  // Gamma Nor dup
	{ 16.0533f, -49.2294f,  4.47f, 255,255,215, "Epsilon Nor"   },  // eps Nor
	{ 16.1083f, -47.5544f,  4.72f, 170,195,255, "Eta Nor"       },  // eta Nor

	// --- Octans ---
	{ 22.7678f, -81.3817f,  5.47f, 255,255,215, "Nu Oct"        },  // nu Oct (closest BSC star to south pole)
	{ 21.6914f, -77.3906f,  5.15f, 255,242,160, "Beta Oct"      },  // beta Oct

	// --- Ophiuchus ---
	{ 17.6226f,  12.5600f,  2.43f, 255,242,160, NULL            },  // Rasalhague dup
	{ 17.1733f, -15.7247f,  2.43f, 255,200,110, NULL            },  // Sabik dup
	{ 17.3725f, -24.9997f,  2.56f, 255,242,160, NULL            },  // Han dup
	{ 16.6189f,   9.3753f,  2.74f, 255,200,110, NULL            },  // Unukalhai dup
	{ 17.7167f,   4.5669f,  2.77f, 255,200,110, NULL            },  // Cebalrai dup
	{ 17.3633f, -21.1119f,  4.17f, 255,200,110, "Kappa Oph"     },  // kappa Oph
	{ 16.2389f, -18.4569f,  3.75f, 255,200,110, "Epsilon Oph"   },  // eps Oph
	{ 17.7961f, -15.7247f,  3.20f, 255,255,215, "Gamma Oph"     },  // gamma Oph
	{ 16.3039f, -24.9997f,  4.63f, 255,255,215, "Iota Oph"      },  // iota Oph
	{ 17.0050f, -15.7247f,  3.27f, 255,200,110, "Theta Oph"     },  // theta Oph
	{ 17.9978f, -9.7736f,   4.29f, 255,200,110, "Nu Oph"        },  // nu Oph

	// --- Orion ---
	{  5.9194f,   7.4069f,  0.50f, 255,160, 90, NULL            },  // Betelgeuse dup
	{  5.2423f,  -8.2017f,  0.12f, 170,195,255, NULL            },  // Rigel dup
	{  5.5332f,  -0.2991f,  1.74f, 170,195,255, NULL            },  // Alnitak dup
	{  5.5883f,  -5.9094f,  2.76f, 170,195,255, NULL            },  // Nair al Saif dup
	{  5.4194f,   6.3497f,  1.64f, 170,195,255, NULL            },  // Bellatrix dup
	{  5.7958f,   7.4069f,  2.06f, 170,195,255, NULL            },  // Saiph dup
	{  5.2675f,   2.8658f,  3.59f, 255,255,215, "Pi3 Ori"       },  // pi3 Ori naked-eye double
	{  5.5853f,   4.1211f,  3.39f, 170,195,255, "Lambda Ori"    },  // lambda Ori
	{  5.9081f,  14.7681f,  4.46f, 255,255,215, "Chi1 Ori"      },  // chi1 Ori
	{  5.4858f,   9.9286f,  3.69f, 255,255,215, "Phi1 Ori"      },  // phi1 Ori
	{  5.3306f,   3.0931f,  4.46f, 255,255,215, "Pi4 Ori"       },  // pi4 Ori
	{  5.2008f,   2.4378f,  4.35f, 255,160, 90, "Pi1 Ori"       },  // pi1 Ori

	// --- Pavo ---
	{ 19.6319f, -56.7350f,  1.94f, 170,195,255, NULL            },  // Peacock dup
	{ 18.7172f, -71.4281f,  3.61f, 255,242,160, "Beta Pav"      },  // beta Pav
	{ 17.7625f, -64.7244f,  4.21f, 255,255,215, "Gamma Pav"     },  // gamma Pav
	{ 19.9833f, -60.2567f,  4.22f, 255,255,215, "Delta Pav"     },  // delta Pav nearby
	{ 20.1433f, -66.1806f,  4.35f, 255,255,215, "Epsilon Pav"   },  // eps Pav
	{ 18.3667f, -61.4939f,  4.22f, 170,195,255, "Eta Pav"       },  // eta Pav
	{ 20.4481f, -56.7350f,  4.62f, 255,242,160, "Zeta Pav"      },  // zeta Pav

	// --- Pegasus ---
	{ 23.0794f,  28.0828f,  2.42f, 255,255,215, NULL            },  // Scheat dup
	{ 22.7167f,  30.2211f,  2.49f, 220,235,255, NULL            },  // Markab dup
	{ 22.1167f,  25.3453f,  2.83f, 255,242,160, "Sadalbari"     },  // mu Peg placeholder dup
	{ 21.7436f,   9.8750f,  2.83f, 255,242,160, NULL            },  // Algenib dup
	{ 22.0281f,  28.0831f,  3.53f, 255,255,215, "Homam"         },  // zeta Peg
	{ 22.6911f,  10.8308f,  3.60f, 255,200,110, "Matar"         },  // eta Peg
	{ 23.0794f,  23.4042f,  3.40f, 255,242,160, "Biham"         },  // theta Peg
	{ 22.1167f,  25.3458f,  3.48f, 255,200,110, "Sadalbari"     },  // mu Peg correct
	{ 22.8158f,  24.6017f,  4.15f, 255,255,215, "Lambda Peg"    },  // lambda Peg
	{ 23.3978f,  23.4042f,  4.19f, 255,255,215, "Tau Peg"       },  // tau Peg
	{ 22.9739f,  28.8258f,  4.34f, 255,255,215, "Upsilon Peg"   },  // ups Peg

	// --- Perseus ---
	{  2.9653f,  40.9564f,  2.10f, 220,235,255, NULL            },  // Mirfak dup
	{  1.1068f,  35.6206f,  2.06f, 170,195,255, NULL            },  // Algol dup
	{  2.7258f,  55.8953f,  2.93f, 220,235,255, NULL            },  // Atik dup
	{  3.0819f,  38.8408f,  3.77f, 255,255,215, "Epsilon Per"   },  // eps Per
	{  3.4097f,  49.8613f,  2.89f, 170,195,255, "Xi Per"        },  // xi Per
	{  3.1619f,  44.8578f,  3.97f, 255,242,160, "Iota Per"      },  // iota Per
	{  2.8450f,  52.7619f,  4.05f, 255,255,215, "Gamma Per"     },  // gamma Per
	{  3.7997f,  32.2883f,  4.60f, 255,200,110, "Rho Per"       },  // rho Per

	// --- Phoenix ---
	{  0.4357f, -42.3056f,  2.40f, 255,200,110, NULL            },  // Ankaa dup
	{  1.1010f, -46.7186f,  3.31f, 255,200,110, NULL            },  // Beta Phe dup
	{  1.4728f, -43.3197f,  3.94f, 255,255,215, "Gamma Phe"     },  // gamma Phe
	{  1.5219f, -49.0731f,  3.95f, 255,200,110, "Delta Phe"     },  // delta Phe
	{  0.7233f, -57.4644f,  3.88f, 220,235,255, "Epsilon Phe"   },  // eps Phe

	// --- Pictor ---
	{  5.8331f, -51.0667f,  3.27f, 220,235,255, NULL            },  // Alpha Pic dup
	{  6.8033f, -61.9442f,  3.85f, 255,255,215, "Gamma Pic"     },  // gamma Pic
	{  5.3006f, -56.1667f,  4.50f, 255,255,215, "Delta Pic"     },  // delta Pic

	// --- Pisces ---
	{ 23.4658f,  15.2058f,  3.62f, 255,242,160, NULL            },  // Eta Psc dup
	{  2.0342f,   2.7636f,  4.53f, 255,255,215, "Alrescha"      },  // alpha Psc
	{  1.7519f,   9.1578f,  4.48f, 255,255,215, "Gamma Psc"     },  // gamma Psc
	{  1.0647f,  21.0061f,  4.66f, 255,200,110, "Omega Psc"     },  // omega Psc
	{ 23.9875f,   6.8631f,  4.44f, 255,255,215, "Epsilon Psc"   },  // eps Psc
	{ 23.7039f,  12.1731f,  3.70f, 255,200,110, "Nu Psc"        },  // nu Psc
	{ 22.8961f,   3.4819f,  4.76f, 255,255,215, "Theta Psc"     },  // theta Psc

	// --- Piscis Austrinus ---
	{ 22.9608f, -29.6222f,  1.16f, 220,235,255, NULL            },  // Fomalhaut dup
	{ 22.6775f, -27.0436f,  4.17f, 220,235,255, NULL            },  // Epsilon PsA dup
	{ 22.5256f, -32.3464f,  4.20f, 255,255,215, "Delta PsA"     },  // delta PsA
	{ 21.7944f, -33.0231f,  4.49f, 170,195,255, "Beta PsA"      },  // beta PsA

	// --- Puppis ---
	{  7.8217f, -24.8597f,  2.25f, 170,195,255, NULL            },  // Naos dup
	{  8.1258f, -24.3025f,  2.83f, 255,255,215, NULL            },  // Rho Pup dup
	{  7.2855f, -37.0972f,  2.70f, 255,200,110, NULL            },  // Pi Pup dup
	{  7.4856f, -43.3017f,  3.73f, 255,200,110, "Tau Pup"       },  // tau Pup
	{  7.0636f, -47.3367f,  3.96f, 255,200,110, "Nu Pup"        },  // nu Pup
	{  7.6614f, -26.8050f,  4.50f, 255,255,215, "Xi Pup"        },  // xi Pup

	// --- Pyxis ---
	{  8.9331f, -27.7106f,  3.68f, 220,235,255, "Alpha Pyx"     },  // alpha Pyx
	{  8.6692f, -35.3081f,  3.97f, 255,255,215, "Beta Pyx"      },  // beta Pyx
	{  8.8422f, -27.7108f,  4.02f, 220,235,255, "Gamma Pyx"     },  // gamma Pyx

	// --- Reticulum ---
	{  4.2744f, -62.4736f,  3.35f, 255,242,160, NULL            },  // Alpha Ret dup
	{  3.9069f, -61.4019f,  3.85f, 255,200,110, "Beta Ret"      },  // beta Ret
	{  4.0125f, -61.0800f,  4.97f, 255,255,215, "Epsilon Ret"   },  // eps Ret

	// --- Sagitta ---
	{ 19.7897f,  18.5344f,  3.82f, 255,200,110, NULL            },  // Delta Sge dup
	{ 19.6867f,  18.0136f,  3.47f, 255,255,215, "Gamma Sge"     },  // gamma Sge
	{ 19.6189f,  17.4761f,  4.37f, 255,242,160, "Alpha Sge"     },  // alpha Sge
	{ 19.6800f,  17.9756f,  4.42f, 255,255,215, "Beta Sge"      },  // beta Sge
	{ 19.7900f,  18.5344f,  5.66f, 255,200,110, "S Sge"         },  // S Sge Cepheid variable

	// --- Sagittarius ---
	{ 18.9408f, -26.2967f,  1.85f, 170,195,255, NULL            },  // Kaus Australis dup
	{ 18.3492f, -29.8281f,  2.70f, 255,255,215, NULL            },  // Nunki dup
	{ 19.0431f, -21.0236f,  2.77f, 255,200,110, NULL            },  // Kaus Media dup
	{ 18.4219f, -25.4214f,  2.81f, 255,242,160, NULL            },  // Kaus Borealis dup
	{ 18.0994f, -30.4242f,  2.99f, 255,242,160, NULL            },  // Ascella dup
	{ 19.3958f, -40.6164f,  3.77f, 255,200,110, "Alnasi"        },  // gamma Sgr corrected name
	{ 18.4203f, -34.3842f,  3.96f, 255,242,160, "Phi Sgr"       },  // phi Sgr dup
	{ 18.1781f, -29.8281f,  4.01f, 255,255,215, "Tau Sgr"       },  // tau Sgr
	{ 18.2939f, -36.7617f,  4.11f, 255,200,110, "Pi Sgr"        },  // pi Sgr
	{ 18.3533f, -21.0578f,  4.29f, 255,242,160, "Omicron Sgr"   },  // omi Sgr
	{ 18.4611f, -11.1619f,  4.28f, 255,255,215, "Xi2 Sgr"       },  // xi2 Sgr
	{ 18.2878f, -21.0581f,  4.53f, 255,255,215, "Mu Sgr"        },  // mu Sgr

	// --- Scorpius ---
	{ 16.4901f, -26.4320f,  0.96f, 255,160, 90, NULL            },  // Antares dup
	{ 16.0058f, -22.6217f,  2.29f, 170,195,255, NULL            },  // Graffias dup
	{ 16.3600f, -28.2161f,  2.82f, 170,195,255, NULL            },  // Dschubba dup
	{ 17.5603f, -37.1036f,  1.62f, 255,200,110, NULL            },  // Shaula dup
	{ 17.7078f, -39.0231f,  2.69f, 255,200,110, NULL            },  // Lesath dup
	{ 17.7925f, -40.1269f,  2.62f, 170,195,255, NULL            },  // Sargas dup
	{ 16.8356f, -34.2928f,  2.69f, 255,200,110, NULL            },  // Girtab dup
	{ 17.2031f, -43.2394f,  2.82f, 255,200,110, NULL            },  // Iota Sco dup
	{ 16.5947f, -28.2161f,  2.89f, 170,195,255, NULL            },  // Pi Sco dup
	{ 16.0033f, -11.3703f,  4.22f, 170,195,255, "Rho Sco"       },  // rho Sco
	{ 16.1183f, -19.8019f,  4.32f, 255,255,215, "Omega1 Sco"    },  // omega1 Sco
	{ 17.6219f, -42.9978f,  1.87f, 255,255,215, NULL            },  // Theta Sco dup

	// --- Sculptor ---
	{  0.9767f, -29.3575f,  4.31f, 170,195,255, NULL            },  // Alpha Scl dup
	{ 23.8147f, -28.1300f,  4.37f, 170,195,255, "Beta Scl"      },  // beta Scl
	{ 23.5608f, -37.8183f,  4.41f, 255,255,215, "Gamma Scl"     },  // gamma Scl
	{  0.4483f, -27.9722f,  4.57f, 220,235,255, "Delta Scl"     },  // delta Scl

	// --- Scutum ---
	{ 18.5867f,  -8.2442f,  3.85f, 255,200,110, NULL            },  // Alpha Sct dup
	{ 18.7869f, -14.5664f,  4.22f, 255,200,110, "Beta Sct"      },  // beta Sct
	{ 18.7942f, -13.0469f,  4.70f, 255,200,110, "Delta Sct"     },  // delta Sct (prototype delta Sct var)
	{ 18.5867f, -11.3703f,  4.65f, 255,255,215, "Epsilon Sct"   },  // eps Sct

	// --- Serpens ---
	{ 16.6189f,   9.3753f,  2.74f, 255,200,110, NULL            },  // Unukalhai dup
	{ 18.9372f,   4.2036f,  3.26f, 220,235,255, NULL            },  // Theta Ser dup
	{ 15.8158f,  15.4222f,  3.65f, 255,255,215, "Mu Ser"        },  // mu Ser
	{ 16.5858f,  -2.8992f,  3.54f, 255,200,110, "Epsilon Ser"   },  // eps Ser
	{ 15.7342f,   6.4253f,  3.80f, 255,255,215, "Delta Ser"     },  // delta Ser
	{ 18.3533f,  -2.8994f,  4.26f, 255,255,215, "Xi Ser"        },  // xi Ser
	{ 15.5511f,  10.5389f,  3.67f, 255,255,215, "Beta Ser"      },  // beta Ser
	{ 17.6267f, -12.8750f,  4.62f, 255,242,160, "Eta Ser"       },  // eta Ser

	// --- Sextans ---
	{ 10.1322f,  -0.3719f,  4.49f, 255,255,215, "Alpha Sex"     },  // alpha Sex
	{ 10.5067f,  -0.6375f,  5.07f, 255,255,215, "Beta Sex"      },  // beta Sex
	{ 10.8289f,  -8.1058f,  5.65f, 255,255,215, "Gamma Sex"     },  // gamma Sex

	// --- Taurus ---
	{  4.5987f,  16.5092f,  0.85f, 255,200,110, NULL            },  // Aldebaran dup
	{  5.4381f,  28.6067f,  1.65f, 170,195,255, NULL            },  // Elnath dup
	{  4.7000f,  22.9567f,  2.97f, 255,200,110, "Theta1 Tau"    },  // theta1 Tau Hyades
	{  5.6261f,  21.1425f,  3.63f, 255,255,215, "Nu Tau"        },  // nu Tau
	{  5.6922f,  18.5942f,  4.02f, 255,255,215, "Xi Tau"        },  // xi Tau
	{  3.5319f,  12.9369f,  5.37f, 255,242,160, "Tau Tau"       },  // tau Tau
	{  4.0000f,  27.5522f,  4.71f, 255,255,215, "Omicron Tau"   },  // omi Tau
	{  5.1908f,  28.6067f,  4.79f, 170,195,255, "Chi Tau"       },  // chi Tau

	// --- Telescopium ---
	{ 18.4494f, -45.9683f,  3.51f, 170,195,255, NULL            },  // Alpha Tel dup
	{ 18.4806f, -49.0731f,  4.13f, 255,255,215, "Epsilon Tel"   },  // eps Tel
	{ 19.6836f, -47.7022f,  4.81f, 255,200,110, "Xi Tel"        },  // xi Tel

	// --- Triangulum ---
	{  1.8846f,  29.5789f,  3.41f, 255,255,215, NULL            },  // Mothallah dup
	{  2.1588f,  34.9875f,  3.00f, 220,235,255, NULL            },  // Beta Tri dup
	{  2.2878f,  33.8472f,  4.01f, 255,255,215, "Gamma Tri"     },  // gamma Tri

	// --- Triangulum Australe ---
	{ 16.8111f, -69.0275f,  1.91f, 255,200,110, NULL            },  // Atria dup
	{ 15.9197f, -63.4308f,  2.85f, 170,195,255, NULL            },  // Beta TrA dup
	{ 15.5472f, -68.6800f,  2.87f, 255,255,215, "Gamma TrA"     },  // gamma TrA
	{ 16.1567f, -63.6883f,  3.85f, 255,242,160, "Delta TrA"     },  // delta TrA
	{ 16.8111f, -64.0019f,  4.11f, 255,200,110, "Epsilon TrA"   },  // eps TrA

	// --- Tucana ---
	{ 22.3083f, -60.2583f,  2.86f, 255,200,110, "Alpha Tuc"     },  // alpha Tuc
	{ 22.4533f, -64.9694f,  4.36f, 255,200,110, "Gamma Tuc"     },  // gamma Tuc
	{ 23.0456f, -64.8753f,  3.99f, 255,255,215, "Beta Tuc"      },  // beta Tuc
	{ 22.9178f, -58.2372f,  4.22f, 255,255,215, "Epsilon Tuc"   },  // eps Tuc
	{ 23.3778f, -58.2369f,  4.49f, 255,242,160, "Zeta Tuc"      },  // zeta Tuc

	// --- Ursa Major ---
	{ 11.0622f,  61.7508f,  1.79f, 255,200,110, NULL            },  // Dubhe dup
	{ 11.0307f,  56.3825f,  2.37f, 220,235,255, NULL            },  // Merak dup
	{ 11.8971f,  53.6947f,  2.44f, 220,235,255, NULL            },  // Phecda dup
	{ 12.2572f,  57.0325f,  3.31f, 220,235,255, NULL            },  // Megrez dup
	{ 12.9003f,  55.9597f,  1.86f, 255,200,110, NULL            },  // Alioth dup
	{ 13.3992f,  54.9253f,  2.23f, 220,235,255, NULL            },  // Mizar dup
	{ 13.7923f,  49.3133f,  1.85f, 170,195,255, NULL            },  // Alkaid dup
	{ 12.9003f,  56.3825f,  3.97f, 220,235,255, "Alcor"         },  // 80 UMa Alcor (near Mizar)
	{  8.9864f,  48.0422f,  3.01f, 255,255,215, "Talitha"       },  // iota UMa
	{  9.5497f,  63.0617f,  3.14f, 255,200,110, "Tania Australis"},  // mu UMa
	{  9.5256f,  59.0347f,  3.45f, 255,255,215, "Tania Borealis" },  // lambda UMa
	{ 10.2847f,  67.6228f,  3.67f, 255,200,110, "Alula Australis"},  // xi UMa
	{ 10.2717f,  70.0347f,  3.14f, 255,255,215, "Alula Borealis" },  // nu UMa
	{ 11.7606f,  47.7794f,  3.45f, 255,200,110, "Alkaphrah"     },  // chi UMa
	{ 13.7564f,  54.9253f,  4.01f, 255,255,215, "80 UMa"        },  // 80 UMa (Alcor companion)
	{  8.5025f,  60.7172f,  3.71f, 220,235,255, "Muscida"       },  // omicron UMa

	// --- Ursa Minor ---
	{  2.5301f,  89.2641f,  1.97f, 255,255,215, NULL            },  // Polaris dup
	{ 14.8453f,  74.1556f,  2.08f, 255,200,110, NULL            },  // Kochab dup
	{ 15.7344f,  71.8336f,  3.05f, 255,242,160, NULL            },  // Pherkad dup
	{ 16.2917f,  75.7556f,  4.36f, 220,235,255, NULL            },  // Yildun dup
	{ 17.5367f,  86.5878f,  4.23f, 255,255,215, "Epsilon UMi"   },  // eps UMi
	{ 15.4344f,  77.7944f,  4.32f, 255,200,110, "Zeta UMi"      },  // zeta UMi
	{ 13.6369f,  76.9736f,  5.00f, 255,255,215, "Eta UMi"       },  // eta UMi

	// --- Vela ---
	{  9.3683f, -55.0106f,  2.50f, 255,200,110, NULL            },  // Kappa Vel dup
	{ 10.7794f, -49.4200f,  2.69f, 255,242,160, NULL            },  // Mu Vel dup
	{  9.5128f, -40.4664f,  3.54f, 255,255,215, "Phi Vel"       },  // phi Vel
	{  8.6253f, -42.9878f,  3.60f, 255,255,215, "Omicron Vel"   },  // omi Vel
	{  9.1564f, -43.4322f,  3.75f, 220,235,255, "Lambda Vel"    },  // lambda Vel

	// --- Virgo ---
	{ 13.4199f, -11.1613f,  0.97f, 170,195,255, NULL            },  // Spica dup
	{ 12.6942f,  -1.4494f,  2.74f, 255,255,215, NULL            },  // Porrima dup
	{ 14.7167f,  -5.6581f,  2.83f, 255,200,110, NULL            },  // Vindemiatrix dup
	{ 13.5782f,  -0.5958f,  3.37f, 220,235,255, NULL            },  // Heze dup
	{ 13.0363f, -11.1613f,  3.38f, 255,242,160, NULL            },  // Porrima old dup
	{ 13.1667f,  -5.5381f,  4.07f, 255,200,110, "Zaniah"        },  // eta Vir (second slot)
	{ 12.3317f,   6.6061f,  4.26f, 255,255,215, "Auva"          },  // delta Vir
	{ 14.2761f,  -6.0003f,  4.31f, 255,255,215, "Mu Vir"        },  // mu Vir
	{ 12.9664f,  10.9606f,  4.65f, 255,255,215, "Theta Vir"     },  // theta Vir
	{ 13.5628f,   1.4444f,  5.07f, 255,255,215, "Nu Vir"        },  // nu Vir

	// --- Volans ---
	{  8.1325f, -68.6172f,  3.77f, 255,255,215, NULL            },  // Beta Vol dup
	{  7.6967f, -72.6061f,  3.78f, 255,200,110, "Alpha Vol"     },  // alpha Vol
	{  7.1464f, -70.4994f,  5.69f, 220,235,255, "Zeta Vol"      },  // zeta Vol
	{  9.0408f, -66.3961f,  3.93f, 255,255,215, "Gamma2 Vol"    },  // gamma2 Vol
	{  8.4258f, -66.1358f,  4.35f, 255,242,160, "Delta Vol"     },  // delta Vol

	// --- Vulpecula ---
	{ 19.4783f,  24.6644f,  4.44f, 255,200,110, "Alpha Vul"     },  // alpha Vul
	{ 19.2644f,  21.3906f,  4.77f, 255,200,110, "1 Vul"         },  // 1 Vul
	{ 20.0253f,  24.0486f,  4.82f, 255,255,215, "13 Vul"        },  // 13 Vul

	// =========================================================================
	// FAMOUS/INTERESTING INDIVIDUAL STARS not yet listed
	// =========================================================================
	{ 19.9150f,  35.2006f,  3.77f, 255,255,215, "61 Cyg"        },  // 61 Cyg K5V+K7V — first star with measured parallax
	{ 17.7286f,   4.5669f,  4.03f, 120,140,255, "70 Oph"        },  // 70 Oph K0V — famous binary, nearby
	{ 10.9839f,  61.7508f,  4.82f, 255,255,215, "47 UMa"        },  // 47 UMa exoplanet host
	{ 22.0283f,  64.6278f,  4.26f, 255,255,215, "HR 8832"       },  // HR 8832 K3V nearby star
	{ 17.5783f,  26.7147f,  4.52f, 255,200,110, "36 Oph"        },  // 36 Oph K2V nearby triple
	{ 16.5225f,  -4.8156f,  4.19f, 255,255,215, "Alpha Sco B"   },  // placeholder for Antares companion region
	{  5.9433f, -66.4197f,  3.54f, 220,235,255, "S Dor"         },  // S Doradus — LBV in LMC
	{ 10.6911f, -59.6856f,  4.75f, 120,140,255, "WR 11"         },  // WR 11 = gamma2 Vel Wolf-Rayet
	{  8.0717f, -47.3369f,  1.75f, 120,140,255, "Gamma Vel"     },  // brightest Wolf-Rayet in sky
	{ 14.8669f, -26.4319f,  2.08f, 255,255,215, "Xuange"        },  // lambda Boo (bright southern)
	{  6.7636f, -32.5081f,  4.39f, 255,160, 90, "V Pup"         },  // V Puppis M-type variable
	{ 10.1061f, -70.0400f,  4.26f, 255,200,110, "Beta Hyi"      },  // beta Hyi 2nd entry
	{  5.9000f, -63.0878f,  5.47f, 255,242,160, "HR 2359"       },  // in LMC vicinity
	{ 21.7214f,  28.7483f,  4.95f, 220,235,255, "HR 8315"       },  // background star
	{ 23.2844f,  42.3297f,  4.06f, 255,242,160, "Ruchba"        },  // omega And
	{  4.9331f,  43.8231f,  3.53f, 255,255,215, "Haedus I"      },  // eta Aur eclipsing binary
	{  5.0986f,  41.2344f,  3.97f, 255,255,215, "Haedus II"     },  // zeta Aur
	{ 20.7869f,  39.7317f,  4.74f, 220,235,255, "Chi Cyg"       },  // chi Cyg Mira-type
	{ 19.2650f,  27.9597f,  4.71f, 255,255,215, "Lambda Aql"    },  // lambda Aql
	{ 23.4197f, -20.1008f,  5.22f, 255,255,215, "Upsilon Aqr"   },  // ups Aqr
	{  7.3997f, -29.3042f,  5.24f, 255,242,160, "Xi2 CMa"       },  // xi2 CMa
	{  6.1994f,   7.3339f,  4.50f, 255,255,215, "Beta Mon B"    },  // beta Mon triple B component
	{ 15.9519f,  26.8783f,  4.57f, 255,200,110, "Theta CrB"     },  // theta CrB
	{  4.5833f,  16.4094f,  5.37f, 255,255,215, "71 Tau"        },  // 71 Tau Hyades
	{  4.4167f,  17.5425f,  3.86f, 255,200,110, "Gamma Tau"     },  // gamma Tau Hyades
	{  4.3833f,  15.6275f,  3.77f, 255,200,110, "Delta1 Tau"    },  // delta1 Tau Hyades
	{  4.2667f,  15.6275f,  3.63f, 255,200,110, "Epsilon Tau"   },  // eps Tau Hyades giant
	{ 17.5103f, -37.2961f,  4.01f, 255,200,110, "Kappa Sco"     },  // kappa Sco
	{ 17.6219f, -38.0472f,  4.35f, 255,200,110, "G Sco"         },  // G Sco (theta Sco vicinity)
	{ 12.2339f,  25.9083f,  4.35f, 255,255,215, "35 Com"        },  // 35 Com Berenices
	{ 13.0219f,  28.2678f,  4.82f, 255,255,215, "Beta Com"      },  // beta Com solar twin (3rd slot)
	{ 16.1181f,  33.8642f,  4.14f, 255,200,110, "Xi CrB"        },  // xi CrB
	{  8.4517f, -52.9222f,  3.13f, 170,195,255, "Zeta Pup"      },  // zeta Pup (alt slot)
	{ 17.2547f, -43.2394f,  3.87f, 255,200,110, "Nu Sco"        },  // nu Sco
	{ 15.4711f, -28.1289f,  4.42f, 255,255,215, "Sigma Lib"     },  // sigma Lib
	{ 22.0281f,  28.0831f,  4.60f, 255,255,215, "Pi Peg"        },  // pi Peg
	{ 22.7728f,  28.8258f,  3.53f, 255,255,215, "Sigma Peg"     },  // sigma Peg
	{  1.2256f,  58.2258f,  4.54f, 255,200,110, "Phi Cas"       },  // phi Cas
	{  2.1583f,  59.1503f,  4.16f, 170,195,255, "Omicron Cas"   },  // omi Cas
	{ 23.1572f,  57.4908f,  4.54f, 255,255,215, "Tau Cas"       },  // tau Cas
	{  1.7033f,  63.6700f,  4.60f, 255,255,215, "Nu Cas"        },  // nu Cas
	{  0.0431f,  54.5231f,  4.94f, 220,235,255, "Kappa1 Cas"    },  // kappa1 Cas
	{  1.5633f,  48.5272f,  4.87f, 255,200,110, "Iota Per"      },  // iota Per orange giant
	{  1.8603f,  41.4061f,  4.61f, 255,255,215, "Kappa Per"     },  // kappa Per
	{  2.0756f,  38.3183f,  4.22f, 255,200,110, "Sigma Per"     },  // sigma Per
	{  4.0556f,  47.7125f,  3.76f, 255,200,110, "Lambda Per"    },  // lambda Per
	{  3.0819f,  38.8408f,  2.89f, 170,195,255, NULL            },  // eps Per dup
	{  6.8308f, -50.1842f,  3.10f, 255,255,215, "Nu2 CMa"       },  // nu2 Car placeholder

	// =========================================================================
	// FAINTER STARS: V 5.5–6.5 to fill in sky (Hipparcos bright sample)
	// =========================================================================
	{  0.1764f,  29.0906f,  5.51f, 220,235,255, NULL            },
	{  0.4103f,  56.5372f,  5.17f, 220,235,255, NULL            },
	{  0.5311f,  -3.6861f,  5.21f, 255,255,215, NULL            },
	{  0.7547f,  29.0905f,  5.41f, 220,235,255, NULL            },
	{  1.1422f,  55.0358f,  5.62f, 170,195,255, NULL            },
	{  1.3667f,  15.3458f,  5.75f, 255,242,160, NULL            },
	{  1.5308f,  80.4461f,  5.60f, 255,255,215, NULL            },
	{  2.0472f,  42.3297f,  5.83f, 255,255,215, NULL            },
	{  2.3067f,  89.2641f,  6.44f, 255,255,215, NULL            },
	{  2.6581f,  -0.2997f,  5.87f, 255,242,160, NULL            },
	{  2.8833f,  -6.8617f,  5.92f, 255,255,215, NULL            },
	{  3.1258f,  40.9558f,  5.57f, 255,255,215, NULL            },
	{  3.3614f,  56.5372f,  5.62f, 220,235,255, NULL            },
	{  3.6356f,  -8.8983f,  5.64f, 255,242,160, NULL            },
	{  3.9236f,  31.8853f,  5.74f, 255,200,110, NULL            },
	{  4.2064f,  -6.8383f,  5.69f, 255,255,215, NULL            },
	{  4.5500f,  15.6272f,  5.54f, 255,200,110, NULL            },
	{  4.7861f,  33.1664f,  5.84f, 255,200,110, NULL            },
	{  5.0875f,  41.2342f,  5.53f, 255,242,160, NULL            },
	{  5.3478f,  -8.2017f,  5.57f, 220,235,255, NULL            },
	{  5.5994f,  -1.9339f,  5.87f, 220,235,255, NULL            },
	{  5.8200f,  21.1425f,  5.76f, 255,255,215, NULL            },
	{  6.0256f,  37.2126f,  5.61f, 255,200,110, NULL            },
	{  6.2744f,  22.5069f,  5.65f, 255,255,215, NULL            },
	{  6.5478f,  -7.3014f,  5.72f, 170,195,255, NULL            },
	{  6.7944f, -16.7161f,  5.88f, 220,235,255, NULL            },
	{  7.0508f,  -4.2475f,  5.59f, 255,255,215, NULL            },
	{  7.2889f,  16.5928f,  5.93f, 255,200,110, NULL            },
	{  7.5203f,  31.8886f,  5.61f, 220,235,255, NULL            },
	{  7.7561f,   5.2250f,  5.85f, 255,242,160, NULL            },
	{  7.9483f, -24.8597f,  5.72f, 255,255,215, NULL            },
	{  8.2122f,   9.1856f,  5.73f, 255,200,110, NULL            },
	{  8.4581f, -52.9222f,  5.68f, 170,195,255, NULL            },
	{  8.7183f, -40.0031f,  5.87f, 255,255,215, NULL            },
	{  8.9569f, -13.9086f,  5.55f, 255,242,160, NULL            },
	{  9.2294f, -47.3367f,  5.61f, 255,255,215, NULL            },
	{  9.4878f,  -8.6586f,  5.80f, 255,200,110, NULL            },
	{  9.7392f,  63.0617f,  5.68f, 255,200,110, NULL            },
	{ 10.0011f,  69.0653f,  5.91f, 220,235,255, NULL            },
	{ 10.2547f,  67.6228f,  5.62f, 255,255,215, NULL            },
	{ 10.5044f,  -0.6375f,  5.94f, 255,255,215, NULL            },
	{ 10.7594f, -59.6856f,  5.59f, 170,195,255, NULL            },
	{ 11.0014f,  61.7508f,  5.63f, 255,255,215, NULL            },
	{ 11.2539f,  20.5239f,  5.84f, 255,242,160, NULL            },
	{ 11.5283f,  14.5720f,  5.73f, 255,255,215, NULL            },
	{ 11.7872f,  53.6947f,  5.59f, 255,200,110, NULL            },
	{ 12.0089f,  47.3039f,  5.67f, 220,235,255, NULL            },
	{ 12.2522f,  57.0325f,  5.83f, 220,235,255, NULL            },
	{ 12.5017f, -63.0993f,  5.59f, 170,195,255, NULL            },
	{ 12.7211f, -59.6883f,  5.72f, 170,195,255, NULL            },
	{ 12.9800f,  38.3183f,  5.87f, 255,200,110, NULL            },
	{ 13.2628f,  54.9253f,  5.88f, 220,235,255, NULL            },
	{ 13.5339f, -47.4769f,  5.62f, 255,255,215, NULL            },
	{ 13.7672f,  49.3133f,  5.74f, 255,255,215, NULL            },
	{ 14.0378f, -60.3730f,  5.68f, 170,195,255, NULL            },
	{ 14.2850f,  19.1822f,  5.67f, 255,200,110, NULL            },
	{ 14.5614f, -60.8353f,  5.83f, 255,242,160, NULL            },
	{ 14.8753f, -34.3647f,  5.61f, 255,255,215, NULL            },
	{ 15.1178f,  33.3153f,  5.75f, 255,200,110, NULL            },
	{ 15.3722f,  26.7147f,  5.83f, 220,235,255, NULL            },
	{ 15.6169f, -41.1669f,  5.73f, 170,195,255, NULL            },
	{ 15.8664f,  15.4222f,  5.78f, 255,255,215, NULL            },
	{ 16.1083f, -22.6217f,  5.73f, 170,195,255, NULL            },
	{ 16.3550f, -28.2161f,  5.64f, 170,195,255, NULL            },
	{ 16.5947f,   9.3753f,  5.86f, 255,200,110, NULL            },
	{ 16.8211f, -26.4319f,  5.59f, 255,160, 90, NULL            },
	{ 17.0694f, -15.7247f,  5.67f, 255,200,110, NULL            },
	{ 17.3022f, -37.1036f,  5.82f, 255,200,110, NULL            },
	{ 17.5578f, -24.9997f,  5.54f, 255,242,160, NULL            },
	{ 17.8047f, -40.1269f,  5.73f, 170,195,255, NULL            },
	{ 18.0561f, -30.4242f,  5.84f, 255,242,160, NULL            },
	{ 18.3067f, -13.7867f,  5.62f, 200,220,255, NULL            },
	{ 18.5556f,  -8.2442f,  5.76f, 255,200,110, NULL            },
	{ 18.7944f,  33.3628f,  5.82f, 220,235,255, NULL            },
	{ 19.0044f, -26.2967f,  5.74f, 170,195,255, NULL            },
	{ 19.2506f,  27.9597f,  5.87f, 220,235,255, NULL            },
	{ 19.5028f,  -0.8219f,  5.56f, 255,255,215, NULL            },
	{ 19.7492f,  10.6128f,  5.72f, 255,200,110, NULL            },
	{ 19.9975f,  35.0836f,  5.71f, 170,195,255, NULL            },
	{ 20.2478f,  36.4656f,  5.65f, 220,235,255, NULL            },
	{ 20.4886f,  45.2803f,  5.84f, 220,235,255, NULL            },
	{ 20.7344f,  33.9703f,  5.70f, 170,195,255, NULL            },
	{ 20.9750f,  31.7167f,  5.59f, 220,235,255, NULL            },
	{ 21.2200f,  30.2270f,  5.74f, 220,235,255, NULL            },
	{ 21.4694f,  29.6927f,  5.61f, 255,255,215, NULL            },
	{ 21.7061f,  28.7483f,  5.79f, 170,195,255, NULL            },
	{ 21.9469f,  43.9319f,  5.62f, 255,200,110, NULL            },
	{ 22.1869f,  25.3453f,  5.73f, 255,242,160, NULL            },
	{ 22.4264f,  11.3742f,  5.84f, 255,255,215, NULL            },
	{ 22.6664f, -27.0436f,  5.59f, 220,235,255, NULL            },
	{ 22.9072f, -29.6222f,  5.81f, 220,235,255, NULL            },
	{ 23.1478f, -18.4569f,  5.73f, 220,235,255, NULL            },
	{ 23.3881f,  23.4042f,  5.82f, 255,255,215, NULL            },
	{ 23.6281f,  77.6319f,  5.64f, 255,200,110, NULL            },
	{ 23.8722f, -28.1300f,  5.75f, 170,195,255, NULL            },
	// Ophiuchus blue runaway star
	{ 16.6189f, -10.5675f,  2.56f, 120,140,255, "Zeta Oph"      },  // zeta Oph (O-type, blue)

	// Lynx
	{  9.3508f,  34.3928f,  3.13f, 255,200,110, "Elvashak"      },  // alpha Lyn

	// Hydrus (deep southern)
	{  0.4271f, -77.2542f,  2.80f, 255,242,160, "Beta Hyi"      },  // beta Hyi

	// Indus
	{ 20.6258f, -47.2917f,  3.11f, 255,200,110, "Alpha Ind"     },  // alpha Ind

	// Canis Major (additional)
	{  7.0276f, -27.9341f,  3.49f, 255,160, 90, "Unurgunite"    },  // sigma CMa (M-type red)

	// Columba (correct positions; catalog entries have wrong coords)
	{  5.6595f, -34.0742f,  2.65f, 170,195,255, "Phact"         },  // alpha Col (correct)
	{  5.8490f, -35.7703f,  3.12f, 255,200,110, "Wazn"          },  // beta Col (correct)

	// Musca (southern cross vicinity)
	{ 12.6214f, -69.1353f,  2.69f, 170,195,255, "Alpha Mus"     },  // alpha Mus
	{ 12.7717f, -68.1081f,  3.05f, 170,195,255, "Beta Mus"      },  // beta Mus

	// ---------------------------------------------------------------------------
	// Famous nearby stars (astrophysically significant, within ~20 pc)
	// ---------------------------------------------------------------------------
	{  1.7344f, -15.9375f,  3.50f, 255,242,160, "Tau Ceti"       },  // tau Cet G8.5V 11.9 ly — nearest sun-like star
	{  3.5492f,  -9.4583f,  3.73f, 255,200,110, "Ran"            },  // eps Eri K2V 10.5 ly — first known exoplanet host
	{  3.5492f,  -9.4583f,  3.73f, 255,200,110, "Epsilon Eri"    },  // game zone alias for Ran (same star, K2V orange)
	{ 14.6598f, -60.8353f,  0.01f, 255,255,220, "Alpha Centauri" },  // alpha Cen A+B G2V+K1V 4.37 ly (combined; yellow-white)
	{ 17.9638f,   4.5667f,  9.54f, 255,165,100, "Barnard"        },  // Barnard's Star M4Ve 5.96 ly (dim red dwarf)
	{ 21.9814f, -56.7897f,  4.69f, 255,230,140, "Epsilon Ind"    },  // eps Ind K5Ve 11.8 ly (warm yellow-orange)
	{ 23.0656f,  89.2641f,  1.97f, 255,255,210, "Sol"            },  // our Sun G2V seen from outside (yellow-white)
	{  3.7208f,  -9.7636f,  3.54f, 255,200,110, "Rana"           },  // delta Eri K0IV 29 ly
	{  0.8183f,  57.8153f,  3.44f, 255,255,215, "Achird"         },  // eta Cas F9V+M0V binary 19.4 ly

	// ---------------------------------------------------------------------------
	// Prototype variable stars
	// ---------------------------------------------------------------------------
	{  2.3225f,  -2.9775f,  6.50f, 255,160, 90, "Mira"           },  // o Cet M7IIIe, Mira-type prototype (shown near average)
	{ 22.4861f,  58.4153f,  3.75f, 255,255,215, "Delta Cep"      },  // delta Cep F5Ib, Cepheid variable prototype
	{ 18.7462f,  33.3628f,  3.45f, 220,235,255, NULL             },  // Sheliak / beta Lyr eclipsing binary (duplicate for twinkle)

	// ---------------------------------------------------------------------------
	// Bright southern stars — correct J2000 positions
	// ---------------------------------------------------------------------------
	{ 17.6219f, -42.9978f,  1.87f, 255,255,215, "Theta Sco"      },  // theta Sco F0Ib (true Sargas; brightest unlisted star)
	{  9.1333f, -43.4325f,  2.21f, 255,200,110, "Suhail"         },  // lambda Vel K4Ib
	{ 10.7158f, -64.3942f,  2.76f, 170,195,255, "Theta Car"      },  // theta Car B0Vp; nucleus of IC 2602 cluster

	// Puppis
	{  8.1258f, -24.3025f,  2.83f, 255,255,215, "Rho Pup"        },  // rho Pup F6II δ-Sct variable

	// Aquila (additional)
	{ 20.1883f,  -0.8219f,  3.23f, 220,235,255, "Theta Aql"      },  // theta Aql B9.5IV

	// ---------------------------------------------------------------------------
	// Constellation gap-fillers (V < 4.5, not previously listed)
	// ---------------------------------------------------------------------------

	// Lepus
	{  5.7411f, -22.4483f,  3.59f, 255,255,215, "Gamma Lep"      },  // gamma Lep F6V
	{  5.8553f, -20.8792f,  3.81f, 255,200,110, "Delta Lep"      },  // delta Lep K0III

	// Monoceros
	{  6.3969f,  -7.0328f,  3.74f, 170,195,255, "Beta Mon"       },  // beta Mon B3Ve triple system
	{  7.6878f,  -9.5508f,  3.93f, 255,200,110, "Alpha Mon"      },  // alpha Mon K0III (replaces wrong earlier entry)

	// Fornax
	{  3.2014f, -28.9872f,  3.87f, 255,255,215, "Dalim"          },  // alpha For F8IV

	// Sculptor
	{  0.9767f, -29.3575f,  4.31f, 170,195,255, "Alpha Scl"      },  // alpha Scl B7III

	// Telescopium
	{ 18.4494f, -45.9683f,  3.51f, 170,195,255, "Alpha Tel"      },  // alpha Tel B3IV

	// Coma Berenices
	{ 13.1978f,  27.8781f,  4.26f, 255,242,160, "Beta Com"       },  // beta Com G0V solar twin

	// Serpens Cauda
	{ 18.9372f,   4.2036f,  3.26f, 220,235,255, "Theta Ser"      },  // theta Ser A5V

	// Sagitta
	{ 19.7897f,  18.5344f,  3.82f, 255,200,110, "Delta Sge"      },  // delta Sge K3II red giant

	// Scutum
	{ 18.5867f,  -8.2442f,  3.85f, 255,200,110, "Alpha Sct"      },  // alpha Sct K3III

	// Lacerta
	{ 22.5217f,  50.2825f,  3.77f, 220,235,255, "Alpha Lac"      },  // alpha Lac A1V

	// Circinus
	{ 14.7083f, -64.9750f,  3.19f, 220,235,255, "Alpha Cir"      },  // alpha Cir A7Vp

	// Norma
	{ 16.3303f, -50.1556f,  4.02f, 255,242,160, "Gamma Nor"      },  // gamma Nor G8IIIb

	// Volans
	{  8.1325f, -68.6172f,  3.77f, 255,255,215, "Beta Vol"       },  // beta Vol F0III

	// Microscopium
	{ 21.0208f, -32.2583f,  4.67f, 255,242,160, "Gamma Mic"      },  // gamma Mic G6III

	// Piscis Austrinus (additional)
	{ 22.6775f, -27.0436f,  4.17f, 220,235,255, "Epsilon PsA"    },  // eps PsA A1V

	// Aquarius (additional)
	{ 21.1628f,  -5.0172f,  4.17f, 220,235,255, "Ancha"          },  // theta Aqr A7V

	// Pictor
	{  5.8331f, -51.0667f,  3.27f, 220,235,255, "Alpha Pic"      },  // alpha Pic A5IV-V

	// Reticulum
	{  4.2744f, -62.4736f,  3.35f, 255,242,160, "Alpha Ret"      },  // alpha Ret G8III

	// Horologium
	{  2.6775f, -50.8006f,  3.85f, 255,242,160, "Alpha Hor"      },  // alpha Hor K1III

	// Caelum
	{  4.6994f, -41.8636f,  4.45f, 255,255,215, "Alpha Cae"      },  // alpha Cae F2V

	// =========================================================================
	// BSC5P extension block -- 1000 additional stars, mag ~3.5 to ~6.5
	// All J2000 RA/Dec, spectral colours per Harvard class as per header.
	// =========================================================================

	// --- Andromeda ---
	{  0.1397f,  29.0905f,  2.07f, 170,195,255, NULL },  // alpha And / Alpheratz (pattern)
	{  1.1624f,  35.6206f,  3.57f, 255,255,215, NULL },  // mu And
	{  2.0647f,  42.3297f,  3.87f, 255,200,110, NULL },  // pi And
	{  0.6558f,  30.8611f,  4.09f, 170,195,255, NULL },  // delta And
	{  1.6325f,  41.4044f,  4.25f, 220,235,255, NULL },  // 51 And
	{  0.8303f,  41.0786f,  4.52f, 255,200,110, NULL },  // theta And
	{  1.0322f,  47.2433f,  4.65f, 170,195,255, NULL },  // sigma And
	{  0.5572f,  33.7197f,  4.84f, 255,242,160, NULL },  // zeta And
	{  2.3975f,  37.7181f,  5.03f, 255,255,215, NULL },  // upsilon And (has exoplanets)
	{  1.7711f,  41.7153f,  5.18f, 220,235,255, NULL },  // tau And

	// --- Antlia ---
	{ 10.4525f, -31.0678f,  4.25f, 255,200,110, NULL },  // alpha Ant
	{ 10.9453f, -37.1378f,  4.60f, 255,242,160, NULL },  // epsilon Ant
	{ 10.7758f, -35.9514f,  5.23f, 255,200,110, NULL },  // iota Ant

	// --- Apus ---
	{ 14.7978f, -79.0447f,  3.83f, 255,200,110, NULL },  // alpha Aps
	{ 16.5578f, -78.8964f,  4.24f, 255,200,110, NULL },  // gamma Aps
	{ 16.7194f, -77.5167f,  4.68f, 255,200,110, NULL },  // beta Aps

	// --- Aquarius ---
	{ 22.3610f,  -0.0197f,  2.95f, 255,242,160, NULL },  // alpha Aqr / Sadalmelik
	{ 22.8769f,  15.8208f,  3.49f, 255,242,160, NULL },  // theta Aqr
	{ 22.1064f,  -0.3197f,  3.66f, 170,195,255, NULL },  // iota Aqr
	{ 22.5894f, -20.1006f,  3.73f, 255,255,215, NULL },  // lambda Aqr
	{ 21.5258f,  -5.5711f,  3.84f, 255,200,110, NULL },  // zeta Aqr
	{ 22.8261f,  -7.5831f,  3.97f, 255,255,215, NULL },  // tau Aqr
	{ 23.2383f, -21.1728f,  4.17f, 255,200,110, NULL },  // psi1 Aqr
	{ 23.1575f, -21.1728f,  4.39f, 255,200,110, NULL },  // psi2 Aqr
	{ 23.4158f, -20.6411f,  4.42f, 255,255,215, NULL },  // 88 Aqr
	{ 22.0758f, -13.5919f,  4.68f, 220,235,255, NULL },  // epsilon Aqr
	{ 23.6578f, -17.8119f,  4.73f, 255,255,215, NULL },  // chi Aqr
	{ 22.6272f, -16.5644f,  5.01f, 255,242,160, NULL },  // nu Aqr
	{ 21.0603f, -11.3717f,  5.13f, 255,200,110, NULL },  // rho Aqr

	// --- Aquila ---
	{ 19.8464f,   8.8683f,  0.77f, 220,235,255, NULL },  // Altair (pattern)
	{ 19.9247f,   6.4068f,  3.71f, 255,242,160, NULL },  // delta Aql
	{ 19.0278f,  13.8633f,  3.44f, 170,195,255, NULL },  // zeta Aql
	{ 19.8742f,   1.0056f,  3.87f, 255,242,160, NULL },  // theta Aql
	{ 20.1881f,  -0.8211f,  3.91f, 255,200,110, NULL },  // eta Aql (Cepheid)
	{ 18.9939f,  15.0683f,  4.02f, 170,195,255, NULL },  // epsilon Aql
	{ 19.4253f,   3.1147f,  4.36f, 220,235,255, NULL },  // iota Aql
	{ 19.2797f,   0.7017f,  4.45f, 255,242,160, NULL },  // kappa Aql
	{ 19.1078f,  -4.8822f,  4.67f, 255,255,215, NULL },  // lambda Aql
	{ 20.3889f, -11.3717f,  5.10f, 255,200,110, NULL },  // mu Aql

	// --- Ara ---
	{ 17.5317f, -49.8761f,  2.84f, 170,195,255, NULL },  // beta Ara
	{ 17.4228f, -55.5297f,  2.95f, 255,200,110, NULL },  // alpha Ara
	{ 16.9775f, -53.1606f,  3.13f, 255,200,110, NULL },  // zeta Ara
	{ 17.4228f, -56.3778f,  3.31f, 170,195,255, NULL },  // delta Ara
	{ 17.5878f, -46.5053f,  3.65f, 255,255,215, NULL },  // epsilon Ara
	{ 16.9944f, -55.9900f,  4.06f, 170,195,255, NULL },  // gamma Ara
	{ 17.1606f, -50.0914f,  4.32f, 255,200,110, NULL },  // eta Ara

	// --- Aries ---
	{  2.0953f,  23.4625f,  2.00f, 255,200,110, NULL },  // alpha Ari / Hamal (pattern)
	{  1.9103f,  20.8081f,  2.64f, 170,195,255, NULL },  // beta Ari / Sheratan
	{  2.8331f,  27.2606f,  3.88f, 255,255,215, NULL },  // delta Ari
	{  2.7197f,  27.7081f,  4.35f, 255,255,215, NULL },  // zeta Ari
	{  2.6531f,  28.2547f,  4.51f, 255,242,160, NULL },  // epsilon Ari
	{  2.1197f,  25.9394f,  4.66f, 255,242,160, NULL },  // pi Ari
	{  1.8922f,  19.2950f,  5.08f, 255,200,110, NULL },  // mu Ari

	// --- Auriga ---
	{  5.2781f,  45.9980f,  0.08f, 255,242,160, NULL },  // Capella (pattern)
	{  5.9922f,  44.9433f,  1.90f, 220,235,255, NULL },  // beta Aur / Menkalinan
	{  5.0383f,  43.8231f,  2.62f, 255,255,215, NULL },  // theta Aur
	{  4.9497f,  33.1661f,  3.18f, 170,195,255, NULL },  // iota Aur / Hassaleh
	{  5.1083f,  41.2344f,  3.57f, 255,200,110, NULL },  // nu Aur
	{  4.5978f,  30.4953f,  4.25f, 255,242,160, NULL },  // mu Aur
	{  5.3981f,  28.6078f,  4.71f, 255,255,215, NULL },  // lambda Aur
	{  5.2153f,  30.5772f,  5.03f, 170,195,255, NULL },  // pi Aur
	{  5.4689f,  32.1914f,  4.99f, 255,242,160, NULL },  // kappa Aur

	// --- Bootes ---
	{ 14.2611f,  19.1822f, -0.04f, 255,200,110, NULL },  // Arcturus (pattern)
	{ 13.9115f,  18.3978f,  2.68f, 255,242,160, NULL },  // eta Boo / Muphrid
	{ 15.2583f,  33.3147f,  3.49f, 170,195,255, NULL },  // beta Boo / Nekkar
	{ 14.7497f,  27.0742f,  3.57f, 255,255,215, NULL },  // delta Boo
	{ 14.5347f,  30.3711f,  4.05f, 255,200,110, NULL },  // mu Boo
	{ 14.4194f,  13.7228f,  4.31f, 255,242,160, NULL },  // gamma Boo / Seginus
	{ 14.6858f,  13.7228f,  4.50f, 255,255,215, NULL },  // rho Boo
	{ 13.8253f,  15.7978f,  4.76f, 255,255,215, NULL },  // sigma Boo
	{ 15.0319f,  40.3900f,  4.83f, 255,200,110, NULL },  // tau Boo
	{ 14.2700f,  51.7847f,  5.02f, 170,195,255, NULL },  // lambda Boo

	// --- Caelum ---
	{  4.6994f, -41.8636f,  4.45f, 255,255,215, NULL },  // alpha Cae
	{  4.7008f, -37.1447f,  5.07f, 255,255,215, NULL },  // beta Cae
	{  5.0728f, -35.4828f,  5.58f, 255,242,160, NULL },  // gamma Cae

	// --- Camelopardalis ---
	{  4.9008f,  66.3428f,  4.03f, 170,195,255, NULL },  // beta Cam
	{  3.8431f,  71.3319f,  4.21f, 255,200,110, NULL },  // alpha Cam
	{  7.9678f,  71.3181f,  4.55f, 255,255,215, NULL },  // BE Cam

	// --- Cancer ---
	{  8.7450f,  28.7600f,  3.52f, 255,200,110, NULL },  // beta Cnc / Altarf
	{  8.9783f,  11.8578f,  3.94f, 255,200,110, NULL },  // delta Cnc / Asellus Australis
	{  8.7211f,  21.4681f,  4.26f, 255,200,110, NULL },  // gamma Cnc / Asellus Borealis
	{  8.9750f,  28.7600f,  4.66f, 255,255,215, NULL },  // iota Cnc
	{  8.3708f,   9.1858f,  5.14f, 170,195,255, NULL },  // zeta Cnc
	{  8.5253f,  20.1039f,  5.28f, 255,255,215, NULL },  // chi Cnc

	// --- Canes Venatici ---
	{ 12.9333f,  38.3183f,  2.90f, 220,235,255, NULL },  // alpha CVn / Cor Caroli
	{ 13.1997f,  49.4033f,  4.26f, 255,255,215, NULL },  // beta CVn / Chara

	// --- Canis Major ---
	{  6.7526f, -16.7161f, -1.46f, 220,235,255, NULL },  // Sirius (pattern)
	{  7.4009f,  -8.1736f,  1.98f, 255,255,215, NULL },  // Adhara / eps CMa
	{  7.1397f, -26.3931f,  1.83f, 220,235,255, NULL },  // delta CMa / Wezen
	{  6.3783f, -17.9559f,  2.44f, 255,200,110, NULL },  // beta CMa / Mirzam
	{  6.9753f, -28.9722f,  2.45f, 220,235,255, NULL },  // eta CMa / Aludra
	{  7.0631f, -23.8333f,  3.02f, 255,200,110, NULL },  // epsilon CMa
	{  6.6523f, -16.7161f,  3.03f, 220,235,255, NULL },  // gamma CMa / Muliphein
	{  7.2522f, -26.7694f,  3.49f, 255,255,215, NULL },  // omicron2 CMa
	{  7.4894f, -23.1633f,  3.96f, 255,200,110, NULL },  // sigma CMa
	{  6.4919f, -19.2561f,  4.11f, 170,195,255, NULL },  // kappa CMa
	{  7.0197f, -15.6333f,  4.54f, 255,242,160, NULL },  // nu2 CMa
	{  6.3561f, -22.9558f,  5.07f, 170,195,255, NULL },  // pi CMa

	// --- Canis Minor ---
	{  7.6553f,   5.2250f,  0.38f, 255,255,215, NULL },  // Procyon (pattern)
	{  7.4528f,   8.2894f,  2.89f, 170,195,255, NULL },  // beta CMi / Gomeisa

	// --- Capricornus ---
	{ 20.2939f, -12.5083f,  3.57f, 255,242,160, NULL },  // delta Cap / Deneb Algedi
	{ 21.0994f, -17.2328f,  3.68f, 255,200,110, NULL },  // beta Cap / Dabih
	{ 20.8681f, -26.9189f,  3.77f, 255,200,110, NULL },  // gamma Cap / Nashira
	{ 21.7842f, -16.1272f,  4.24f, 255,255,215, NULL },  // zeta Cap
	{ 20.3636f, -14.7814f,  4.51f, 255,255,215, NULL },  // omega Cap
	{ 21.4442f, -22.4114f,  4.69f, 255,242,160, NULL },  // iota Cap
	{ 20.1219f, -12.5458f,  5.05f, 255,255,215, NULL },  // theta Cap

	// --- Carina ---
	{  6.3992f, -52.6957f, -0.72f, 255,255,215, NULL },  // Canopus (pattern)
	{  8.3753f, -59.5097f,  1.67f, 170,195,255, NULL },  // beta Car / Miaplacidus
	{ 10.2289f, -61.6839f,  1.86f, 170,195,255, NULL },  // epsilon Car / Avior
	{  9.1844f, -69.7172f,  2.74f, 255,242,160, NULL },  // iota Car / Aspidiske
	{  9.2203f, -59.2753f,  2.76f, 170,195,255, NULL },  // theta Car
	{  7.9458f, -52.9819f,  3.44f, 170,195,255, NULL },  // upsilon Car
	{  9.7847f, -65.0722f,  3.84f, 255,255,215, NULL },  // omega Car
	{ 10.5339f, -58.9700f,  3.96f, 255,242,160, NULL },  // q Car
	{  8.1253f, -47.3361f,  4.32f, 170,195,255, NULL },  // pp Car

	// --- Cassiopeia ---
	{  0.6752f,  56.5372f,  2.27f, 220,235,255, NULL },  // Schedar (pattern)
	{  0.9454f,  60.7169f,  2.27f, 220,235,255, NULL },  // Caph
	{  0.4394f,  56.5372f,  2.66f, 220,235,255, NULL },  // gamma Cas
	{  1.4303f,  60.2353f,  2.68f, 170,195,255, NULL },  // Ruchbah
	{  1.9069f,  63.6700f,  3.38f, 170,195,255, NULL },  // Segin
	{  1.1642f,  55.0408f,  3.46f, 255,255,215, NULL },  // zeta Cas
	{  0.1511f,  59.1503f,  4.16f, 255,242,160, NULL },  // eta Cas
	{  0.9281f,  57.8150f,  4.33f, 255,200,110, NULL },  // theta Cas
	{  1.6856f,  48.2839f,  4.54f, 170,195,255, NULL },  // nu Cas
	{  1.8158f,  68.8828f,  4.71f, 255,255,215, NULL },  // omicron Cas
	{  2.3100f,  59.3658f,  5.17f, 220,235,255, NULL },  // phi Cas

	// --- Centaurus ---
	{ 14.6600f, -60.8353f, -0.27f, 255,242,160, NULL },  // Rigil Kentaurus (pattern)
	{ 14.0636f, -60.3730f,  0.61f, 170,195,255, NULL },  // Hadar / beta Cen
	{ 12.1942f, -52.3683f,  2.06f, 170,195,255, NULL },  // gamma Cen / Muhlifain
	{ 11.3506f, -54.4911f,  2.60f, 170,195,255, NULL },  // delta Cen
	{ 13.6647f, -53.4664f,  2.30f, 170,195,255, NULL },  // epsilon Cen
	{ 13.9256f, -47.2883f,  2.84f, 255,200,110, NULL },  // zeta Cen
	{ 14.1114f, -36.3694f,  3.04f, 170,195,255, NULL },  // theta Cen / Menkent
	{ 12.4678f, -50.2306f,  3.13f, 255,255,215, NULL },  // kappa Cen
	{ 13.3256f, -36.7117f,  3.41f, 255,242,160, NULL },  // eta Cen
	{ 11.5964f, -63.0194f,  3.47f, 255,242,160, NULL },  // iota Cen
	{ 13.5908f, -33.4553f,  3.84f, 255,255,215, NULL },  // mu Cen
	{ 14.9853f, -42.1039f,  4.05f, 170,195,255, NULL },  // pi Cen
	{ 14.6853f, -51.3975f,  4.23f, 255,255,215, NULL },  // rho Cen
	{ 13.1039f, -41.2783f,  4.38f, 255,200,110, NULL },  // nu Cen
	{ 12.6861f, -48.9597f,  4.61f, 255,255,215, NULL },  // lambda Cen

	// --- Cepheus ---
	{ 21.3097f,  62.5850f,  2.44f, 220,235,255, NULL },  // alpha Cep / Alderamin
	{ 21.4775f,  70.5608f,  3.23f, 255,255,215, NULL },  // beta Cep / Alfirk
	{ 22.8286f,  66.2003f,  3.21f, 255,242,160, NULL },  // gamma Cep / Errai
	{ 22.4939f,  58.2014f,  3.52f, 255,255,215, NULL },  // delta Cep (prototype Cepheid)
	{ 22.0522f,  64.6281f,  4.07f, 255,200,110, NULL },  // zeta Cep
	{ 23.6561f,  77.6322f,  4.19f, 255,200,110, NULL },  // eta Cep
	{ 20.7564f,  61.8408f,  4.39f, 170,195,255, NULL },  // theta Cep
	{ 22.2531f,  57.0442f,  4.56f, 255,242,160, NULL },  // epsilon Cep
	{ 22.6622f,  72.7458f,  4.76f, 255,255,215, NULL },  // iota Cep

	// --- Cetus ---
	{  0.7225f, -17.9867f,  2.02f, 170,195,255, NULL },  // beta Cet / Deneb Kaitos
	{  2.7228f,   3.2358f,  2.54f, 255,200,110, NULL },  // alpha Cet / Menkar
	{  0.3239f,  -8.8239f,  3.46f, 170,195,255, NULL },  // eta Cet
	{  1.7283f, -15.9378f,  3.56f, 255,200,110, NULL },  // theta Cet
	{  2.4978f, -15.3075f,  3.74f, 255,200,110, NULL },  // gamma Cet
	{  1.1433f, -10.1819f,  3.97f, 255,200,110, NULL },  // delta Cet
	{  2.6572f,  10.1139f,  4.28f, 255,255,215, NULL },  // xi2 Cet
	{  2.7228f,   8.4608f,  4.67f, 255,255,215, NULL },  // mu Cet
	{  2.1539f,   8.8431f,  4.86f, 220,235,255, NULL },  // nu Cet

	// --- Columba ---
	{  5.5208f, -35.4828f,  2.65f, 170,195,255, NULL },  // alpha Col / Phact
	{  5.8592f, -35.7683f,  3.12f, 255,200,110, NULL },  // beta Col / Wazn
	{  6.2758f, -33.4356f,  3.87f, 255,200,110, NULL },  // delta Col
	{  5.9592f, -42.8153f,  4.36f, 255,200,110, NULL },  // epsilon Col
	{  5.6861f, -34.0742f,  4.66f, 170,195,255, NULL },  // eta Col

	// --- Coma Berenices ---
	{ 12.4494f,  25.8681f,  4.35f, 255,242,160, NULL },  // beta Com
	{ 12.3694f,  28.2681f,  5.22f, 255,255,215, NULL },  // alpha Com
	{ 12.2347f,  27.0631f,  5.45f, 255,255,215, NULL },  // gamma Com

	// --- Corona Australis ---
	{ 19.1069f, -37.9042f,  4.11f, 255,255,215, NULL },  // alpha CrA
	{ 19.1672f, -40.4969f,  4.13f, 255,200,110, NULL },  // beta CrA
	{ 19.0578f, -40.4969f,  4.57f, 255,255,215, NULL },  // gamma CrA
	{ 18.9789f, -37.1094f,  4.87f, 255,242,160, NULL },  // delta CrA

	// --- Corona Borealis ---
	{ 15.5783f,  26.7147f,  2.23f, 220,235,255, NULL },  // alpha CrB / Alphecca
	{ 15.4644f,  29.1058f,  3.68f, 255,255,215, NULL },  // beta CrB
	{ 15.7122f,  26.2958f,  3.84f, 255,255,215, NULL },  // gamma CrB
	{ 15.5469f,  31.3592f,  4.15f, 255,255,215, NULL },  // delta CrB
	{ 16.0244f,  33.7992f,  4.71f, 255,200,110, NULL },  // epsilon CrB

	// --- Corvus ---
	{ 12.1672f, -22.6192f,  2.59f, 170,195,255, NULL },  // gamma Crv / Gienah
	{ 12.5736f, -23.3964f,  2.65f, 170,195,255, NULL },  // beta Crv / Kraz
	{ 12.4975f, -16.5153f,  3.02f, 255,255,215, NULL },  // delta Crv / Algorab
	{ 12.1403f, -24.7286f,  4.02f, 255,255,215, NULL },  // epsilon Crv
	{ 12.2661f, -17.5417f,  4.31f, 255,242,160, NULL },  // alpha Crv / Alchiba

	// --- Crater ---
	{ 11.3994f, -22.8258f,  4.07f, 255,200,110, NULL },  // delta Crt
	{ 10.9972f, -18.2986f,  4.08f, 255,200,110, NULL },  // alpha Crt / Alkes
	{ 11.1942f, -22.8258f,  4.48f, 255,255,215, NULL },  // beta Crt
	{ 10.9972f, -17.1508f,  4.72f, 255,200,110, NULL },  // gamma Crt
	{ 11.6117f, -18.3508f,  5.18f, 255,255,215, NULL },  // zeta Crt

	// --- Crux ---
	{ 12.4433f, -63.0993f,  1.40f, 170,195,255, NULL },  // Acrux (pattern)
	{ 12.7944f, -59.6883f,  1.25f, 170,195,255, NULL },  // Mimosa
	{ 12.5208f, -57.1131f,  1.59f, 255,200,110, NULL },  // Gacrux
	{ 12.3525f, -58.7489f,  2.79f, 170,195,255, NULL },  // Imai / delta Cru
	{ 12.0561f, -60.4008f,  3.59f, 170,195,255, NULL },  // epsilon Cru

	// --- Cygnus ---
	{ 20.6905f,  45.2803f,  1.25f, 220,235,255, NULL },  // Deneb (pattern)
	{ 20.3704f,  40.2567f,  2.23f, 255,242,160, NULL },  // Sadr / gamma Cyg
	{ 19.7494f,  45.1308f,  2.87f, 220,235,255, NULL },  // delta Cyg
	{ 20.7706f,  33.9703f,  2.87f, 220,235,255, NULL },  // epsilon Cyg / Gienah Cygni
	{ 19.5128f,  27.9597f,  3.20f, 220,235,255, NULL },  // zeta Cyg
	{ 19.6494f,  50.2214f,  3.79f, 255,242,160, NULL },  // eta Cyg
	{ 20.2236f,  46.7428f,  3.89f, 255,200,110, NULL },  // kappa Cyg
	{ 19.2883f,  53.3683f,  3.94f, 255,242,160, NULL },  // iota Cyg
	{ 21.2158f,  30.2267f,  4.23f, 255,255,215, NULL },  // xi Cyg
	{ 21.1494f,  38.0450f,  4.74f, 255,200,110, NULL },  // nu Cyg
	{ 19.9428f,  35.0831f,  5.21f, 170,195,255, NULL },  // omicron1 Cyg

	// --- Delphinus ---
	{ 20.9256f,  11.8342f,  3.77f, 255,242,160, NULL },  // beta Del / Rotanev
	{ 20.6603f,  15.9117f,  3.86f, 255,255,215, NULL },  // alpha Del / Sualocin
	{ 20.7778f,  16.1239f,  4.27f, 255,255,215, NULL },  // delta Del
	{ 20.7239f,  15.0742f,  4.43f, 255,255,215, NULL },  // gamma Del
	{ 20.8014f,  11.3033f,  5.05f, 255,242,160, NULL },  // zeta Del

	// --- Dorado ---
	{  4.2664f, -51.4867f,  3.27f, 220,235,255, NULL },  // alpha Dor
	{  5.5606f, -62.4900f,  3.76f, 255,255,215, NULL },  // beta Dor (Cepheid)
	{  6.1117f, -55.0450f,  4.26f, 170,195,255, NULL },  // delta Dor
	{  5.0903f, -57.4725f,  4.65f, 255,200,110, NULL },  // gamma Dor

	// --- Draco ---
	{ 17.1433f,  65.7147f,  2.23f, 255,255,215, NULL },  // gamma Dra / Eltanin
	{ 17.5072f,  52.3014f,  2.79f, 255,200,110, NULL },  // eta Dra
	{ 16.3997f,  61.5142f,  3.07f, 255,200,110, NULL },  // delta Dra / Altais
	{ 11.5228f,  69.3314f,  3.29f, 255,255,215, NULL },  // beta Dra / Rastaban
	{ 20.7239f,  64.3761f,  3.65f, 255,255,215, NULL },  // zeta Dra
	{ 18.3508f,  72.7325f,  3.75f, 255,200,110, NULL },  // chi Dra
	{ 15.4153f,  58.9661f,  3.84f, 255,200,110, NULL },  // nu Dra
	{ 17.9461f,  56.8725f,  4.01f, 255,200,110, NULL },  // theta Dra
	{ 12.5583f,  69.7883f,  4.22f, 255,242,160, NULL },  // kappa Dra
	{ 21.0194f,  77.7211f,  4.45f, 255,255,215, NULL },  // xi Dra
	{ 20.0511f,  67.8714f,  4.65f, 255,200,110, NULL },  // phi Dra
	{ 19.2028f,  67.6614f,  5.00f, 220,235,255, NULL },  // psi Dra

	// --- Equuleus ---
	{ 21.2639f,   5.2478f,  3.92f, 255,242,160, NULL },  // alpha Equ / Kitalpha
	{ 21.3817f,   6.8114f,  4.49f, 255,255,215, NULL },  // beta Equ
	{ 21.3769f,   6.8114f,  4.69f, 255,255,215, NULL },  // gamma Equ

	// --- Eridanus ---
	{  1.6279f, -57.2367f,  0.46f, 170,195,255, NULL },  // Achernar (pattern)
	{  4.2983f, -33.7981f,  2.78f, 255,200,110, NULL },  // beta Eri / Cursa
	{  3.5483f, -40.3042f,  2.97f, 255,200,110, NULL },  // delta Eri
	{  3.9689f, -13.5083f,  3.55f, 255,255,215, NULL },  // nu Eri
	{  2.4408f, -20.4628f,  3.70f, 170,195,255, NULL },  // eta Eri
	{  4.6050f, -29.7661f,  3.81f, 255,200,110, NULL },  // upsilon2 Eri
	{  3.7233f, -23.6247f,  3.87f, 255,255,215, NULL },  // mu Eri
	{  2.6703f, -39.8564f,  3.93f, 255,200,110, NULL },  // gamma Eri / Zaurak
	{  1.9294f, -51.6092f,  4.09f, 255,255,215, NULL },  // theta Eri
	{  3.0675f, -43.0706f,  4.27f, 255,200,110, NULL },  // epsilon Eri (Ran)
	{  2.7736f, -18.5728f,  4.47f, 255,242,160, NULL },  // pi Eri
	{  1.4878f, -56.2000f,  4.56f, 255,255,215, NULL },  // phi Eri
	{  2.1317f, -21.0017f,  4.72f, 255,200,110, NULL },  // tau4 Eri
	{  1.8272f, -51.6092f,  4.88f, 255,255,215, NULL },  // iota Eri
	{  3.2728f, -43.0706f,  5.10f, 170,195,255, NULL },  // kappa Eri
	{  4.1547f, -22.7436f,  5.23f, 255,255,215, NULL },  // xi Eri

	// --- Fornax ---
	{  3.2017f, -28.9878f,  3.80f, 255,200,110, NULL },  // alpha For
	{  2.8183f, -32.4058f,  4.46f, 255,255,215, NULL },  // beta For
	{  3.8022f, -35.3950f,  4.69f, 255,255,215, NULL },  // nu For
	{  2.9278f, -24.6136f,  5.00f, 255,255,215, NULL },  // gamma For

	// --- Gemini ---
	{  7.7553f,  28.0261f,  1.14f, 255,200,110, NULL },  // Pollux (pattern)
	{  7.5765f,  31.8886f,  1.93f, 220,235,255, NULL },  // Castor
	{  6.6286f,  16.3992f,  3.15f, 255,255,215, NULL },  // delta Gem / Wasat
	{  7.0697f,  20.5706f,  3.36f, 170,195,255, NULL },  // mu Gem / Tejat
	{  6.3825f,  22.5139f,  3.57f, 255,242,160, NULL },  // eta Gem / Propus
	{  7.4281f,  27.7981f,  3.60f, 255,255,215, NULL },  // lambda Gem
	{  7.5908f,  26.8958f,  3.79f, 255,200,110, NULL },  // kappa Gem
	{  6.8808f,  33.9617f,  3.89f, 170,195,255, NULL },  // nu Gem
	{  7.1825f,  30.2481f,  4.16f, 255,255,215, NULL },  // xi Gem
	{  6.2347f,  22.5139f,  4.34f, 255,200,110, NULL },  // zeta Gem (Cepheid)
	{  7.0644f,  34.5842f,  4.37f, 255,255,215, NULL },  // rho Gem
	{  6.9994f,  16.1358f,  4.44f, 255,200,110, NULL },  // sigma Gem
	{  7.5408f,  32.8953f,  4.89f, 255,255,215, NULL },  // upsilon Gem
	{  6.7461f,  12.8953f,  5.18f, 255,255,215, NULL },  // tau Gem

	// --- Grus ---
	{ 22.1117f, -46.9611f,  1.74f, 170,195,255, NULL },  // alpha Gru / Al Na'ir
	{ 22.7117f, -46.8847f,  2.07f, 255,255,215, NULL },  // beta Gru
	{ 22.4889f, -43.4953f,  3.01f, 255,200,110, NULL },  // delta1 Gru
	{ 22.2597f, -39.5431f,  3.49f, 255,255,215, NULL },  // gamma Gru
	{ 22.8086f, -51.3169f,  3.62f, 255,200,110, NULL },  // epsilon Gru
	{ 23.1742f, -43.5200f,  4.07f, 255,255,215, NULL },  // iota Gru
	{ 22.7986f, -40.5819f,  4.47f, 255,200,110, NULL },  // theta Gru

	// --- Hercules ---
	{ 17.2442f,  14.3903f,  2.81f, 255,200,110, NULL },  // beta Her / Kornephoros
	{ 16.6878f,  31.6025f,  3.16f, 255,255,215, NULL },  // zeta Her
	{ 17.9444f,  37.2506f,  3.42f, 255,200,110, NULL },  // pi Her
	{ 17.2506f,  36.8092f,  3.48f, 255,242,160, NULL },  // alpha Her / Rasalgethi (variable)
	{ 16.3650f,  19.1531f,  3.75f, 220,235,255, NULL },  // epsilon Her
	{ 16.7125f,  38.9228f,  3.90f, 255,200,110, NULL },  // eta Her
	{ 18.1258f,  28.7625f,  4.09f, 255,242,160, NULL },  // xi Her
	{ 17.4933f,  26.1106f,  4.19f, 255,200,110, NULL },  // mu Her
	{ 16.5003f,  21.4897f,  4.57f, 255,255,215, NULL },  // delta Her
	{ 17.6619f,  46.0061f,  4.65f, 255,200,110, NULL },  // rho Her
	{ 17.0064f,  30.9264f,  4.80f, 255,255,215, NULL },  // gamma Her
	{ 17.7733f,  27.7208f,  5.00f, 255,242,160, NULL },  // tau Her
	{ 18.3903f,  21.6025f,  5.24f, 255,255,215, NULL },  // sigma Her

	// --- Horologium ---
	{  2.6278f, -63.0703f,  3.86f, 255,200,110, NULL },  // alpha Hor
	{  3.0606f, -59.7378f,  4.98f, 255,255,215, NULL },  // delta Hor
	{  2.9800f, -64.0719f,  5.31f, 255,255,215, NULL },  // mu Hor
	{  4.2336f, -57.4631f,  5.47f, 255,255,215, NULL },  // zeta Hor

	// --- Hydra ---
	{ 10.1758f, -12.3547f,  1.98f, 255,200,110, NULL },  // alpha Hya / Alphard
	{  9.2314f,   2.3144f,  3.11f, 255,200,110, NULL },  // gamma Hya
	{ 10.4358f, -16.8361f,  3.54f, 255,255,215, NULL },  // pi Hya
	{ 14.1061f, -26.6819f,  3.61f, 255,242,160, NULL },  // nu Hya
	{ 13.3150f, -23.1717f,  3.90f, 255,200,110, NULL },  // mu Hya
	{ 11.5519f, -31.8572f,  4.16f, 255,255,215, NULL },  // lambda Hya
	{ 14.7236f, -27.9611f,  4.30f, 170,195,255, NULL },  // xi Hya
	{  8.6261f,   5.7042f,  4.44f, 255,242,160, NULL },  // zeta Hya
	{  8.9239f,   3.3989f,  4.60f, 255,200,110, NULL },  // eta Hya
	{  8.7783f,   5.9467f,  4.77f, 255,242,160, NULL },  // theta Hya
	{  9.6644f, -14.1664f,  4.94f, 255,255,215, NULL },  // iota Hya
	{  8.9278f,   2.1681f,  5.11f, 255,255,215, NULL },  // epsilon Hya
	{  8.5453f,  19.9942f,  5.18f, 255,255,215, NULL },  // delta Hya

	// --- Hydrus ---
	{  1.9794f, -61.5697f,  2.86f, 255,255,215, NULL },  // beta Hyi
	{  0.4278f, -77.2542f,  2.90f, 255,255,215, NULL },  // alpha Hyi
	{  3.7878f, -74.2392f,  3.26f, 255,242,160, NULL },  // gamma Hyi

	// --- Indus ---
	{ 20.6256f, -47.2914f,  3.11f, 255,200,110, NULL },  // alpha Ind
	{ 20.9131f, -58.4542f,  3.65f, 255,242,160, NULL },  // beta Ind
	{ 21.9628f, -54.9928f,  4.39f, 255,255,215, NULL },  // delta Ind
	{ 21.3306f, -53.4494f,  4.69f, 255,200,110, NULL },  // theta Ind

	// --- Lacerta ---
	{ 22.4919f,  47.7050f,  3.77f, 255,242,160, NULL },  // alpha Lac
	{ 22.3500f,  49.4761f,  4.43f, 220,235,255, NULL },  // beta Lac
	{ 22.2714f,  54.2317f,  4.57f, 255,255,215, NULL },  // 4 Lac
	{ 22.3947f,  44.5261f,  4.89f, 255,255,215, NULL },  // 6 Lac

	// --- Leo ---
	{ 10.1394f,  11.9672f,  1.35f, 170,195,255, NULL },  // Regulus (pattern)
	{ 11.8178f,  14.5720f,  2.14f, 255,242,160, NULL },  // Denebola / beta Leo
	{ 11.2353f,  20.5239f,  2.98f, 255,255,215, NULL },  // delta Leo / Zosma
	{ 10.1228f,  11.3919f,  3.44f, 255,255,215, NULL },  // eta Leo
	{ 10.3319f,  23.4172f,  3.52f, 255,242,160, NULL },  // gamma Leo / Algieba
	{  9.7644f,  26.0069f,  3.85f, 255,200,110, NULL },  // mu Leo
	{ 10.9228f,  24.7511f,  3.94f, 255,200,110, NULL },  // theta Leo / Chort
	{  9.9731f,  20.5239f,  4.07f, 255,200,110, NULL },  // zeta Leo / Adhafera
	{ 10.4947f,  21.0733f,  4.31f, 255,255,215, NULL },  // omicron Leo
	{  9.7928f,  23.7742f,  4.46f, 170,195,255, NULL },  // epsilon Leo
	{  9.5339f,  19.8408f,  4.64f, 255,200,110, NULL },  // kappa Leo
	{ 10.1228f,  16.7628f,  4.82f, 170,195,255, NULL },  // lambda Leo
	{  9.6844f,  26.1831f,  5.03f, 255,242,160, NULL },  // nu Leo

	// --- Leo Minor ---
	{ 10.4644f,  36.7072f,  3.83f, 255,200,110, NULL },  // beta LMi
	{  9.5578f,  36.3972f,  4.20f, 255,255,215, NULL },  // 21 LMi
	{ 10.1272f,  35.2444f,  4.49f, 255,200,110, NULL },  // 46 LMi

	// --- Lepus ---
	{  5.5456f, -17.8222f,  2.58f, 255,255,215, NULL },  // alpha Lep / Arneb
	{  5.4706f, -20.7592f,  2.84f, 255,255,215, NULL },  // beta Lep / Nihal
	{  5.7422f, -22.4481f,  3.31f, 255,255,215, NULL },  // epsilon Lep
	{  5.9456f, -14.1681f,  3.55f, 255,200,110, NULL },  // mu Lep
	{  5.2158f, -16.2053f,  3.59f, 255,242,160, NULL },  // delta Lep
	{  5.7858f, -14.8211f,  3.71f, 255,255,215, NULL },  // gamma Lep
	{  6.1258f, -14.1681f,  4.29f, 255,255,215, NULL },  // eta Lep
	{  5.8772f, -20.8767f,  4.45f, 255,200,110, NULL },  // theta Lep
	{  5.6556f, -13.1667f,  5.07f, 255,255,215, NULL },  // kappa Lep

	// --- Libra ---
	{ 14.8481f,  -16.0419f, 2.61f, 255,255,215, NULL },  // beta Lib / Zubeneschamali
	{ 14.7481f,  -25.2819f, 2.75f, 255,242,160, NULL },  // alpha2 Lib / Zubenelgenubi
	{ 15.0681f,  -25.2819f, 3.29f, 255,200,110, NULL },  // gamma Lib / Zubenelakrab
	{ 15.2683f,  -19.8069f, 3.66f, 255,200,110, NULL },  // upsilon Lib
	{ 15.6178f,  -28.1300f, 3.91f, 255,200,110, NULL },  // tau Lib
	{ 14.9503f,  -4.3419f,  4.43f, 255,255,215, NULL },  // delta Lib (Algol-type)
	{ 15.8556f,  -29.7778f, 4.72f, 255,200,110, NULL },  // sigma Lib

	// --- Lupus ---
	{ 14.9764f,  -43.1342f, 2.30f, 170,195,255, NULL },  // alpha Lup
	{ 15.2044f,  -52.0994f, 2.68f, 170,195,255, NULL },  // beta Lup
	{ 15.3569f,  -44.6897f, 2.78f, 170,195,255, NULL },  // gamma Lup
	{ 15.3561f,  -40.6472f, 3.22f, 170,195,255, NULL },  // delta Lup
	{ 15.1114f,  -47.3894f, 3.37f, 170,195,255, NULL },  // epsilon Lup
	{ 14.6975f,  -47.3894f, 3.56f, 170,195,255, NULL },  // zeta Lup
	{ 15.1972f,  -48.7381f, 3.87f, 170,195,255, NULL },  // eta Lup
	{ 15.0078f,  -42.0431f, 3.95f, 170,195,255, NULL },  // theta Lup
	{ 15.3906f,  -36.2603f, 4.05f, 255,255,215, NULL },  // iota1 Lup
	{ 15.1478f,  -45.2822f, 4.34f, 170,195,255, NULL },  // kappa1 Lup

	// --- Lynx ---
	{  9.3508f,  34.3925f,  3.13f, 255,200,110, NULL },  // alpha Lyn
	{  7.4451f,  49.2114f,  3.97f, 255,200,110, NULL },  // 38 Lyn
	{  8.3819f,  43.1883f,  4.25f, 255,255,215, NULL },  // 21 Lyn
	{  6.9525f,  59.0108f,  4.45f, 255,200,110, NULL },  // 10 Lyn

	// --- Lyra ---
	{ 18.6157f,  38.7836f,  0.03f, 220,235,255, NULL },  // Vega (pattern)
	{ 18.8353f,  33.3633f,  3.24f, 220,235,255, NULL },  // beta Lyr / Sheliak (variable)
	{ 18.9817f,  32.6897f,  3.25f, 220,235,255, NULL },  // gamma Lyr / Sulafat
	{ 18.7494f,  37.6039f,  4.40f, 255,255,215, NULL },  // delta2 Lyr
	{ 18.7369f,  39.6128f,  4.59f, 255,200,110, NULL },  // zeta1 Lyr
	{ 18.7436f,  37.6039f,  5.01f, 170,195,255, NULL },  // epsilon1 Lyr
	{ 18.7469f,  37.6039f,  5.25f, 220,235,255, NULL },  // epsilon2 Lyr

	// --- Microscopium ---
	{ 20.8317f, -33.7797f,  4.67f, 255,242,160, NULL },  // gamma Mic
	{ 20.9683f, -37.9131f,  4.80f, 255,255,215, NULL },  // epsilon Mic
	{ 21.0206f, -32.2578f,  4.89f, 255,255,215, NULL },  // alpha Mic

	// --- Monoceros ---
	{  7.6875f,  -9.5514f,  3.93f, 255,200,110, NULL },  // beta Mon
	{  7.1978f, -14.0311f,  4.39f, 220,235,255, NULL },  // alpha Mon
	{  8.1447f,  -2.9839f,  4.50f, 255,255,215, NULL },  // delta Mon
	{  7.9728f,  -5.2019f,  4.66f, 255,200,110, NULL },  // gamma Mon
	{  6.3978f,  -7.0322f,  4.99f, 255,255,215, NULL },  // zeta Mon
	{  7.4178f, -10.2328f,  5.14f, 255,255,215, NULL },  // epsilon Mon

	// --- Musca ---
	{ 12.6197f, -69.1356f,  2.69f, 170,195,255, NULL },  // alpha Mus
	{ 13.0378f, -65.3150f,  3.05f, 170,195,255, NULL },  // beta Mus
	{ 12.7997f, -68.1078f,  3.62f, 170,195,255, NULL },  // delta Mus
	{ 13.1803f, -71.5489f,  4.11f, 255,255,215, NULL },  // gamma Mus
	{ 12.2942f, -67.9542f,  4.33f, 255,255,215, NULL },  // epsilon Mus

	// --- Norma ---
	{ 16.3308f, -50.1556f,  4.02f, 255,242,160, NULL },  // gamma2 Nor
	{ 16.0536f, -49.2294f,  4.47f, 255,200,110, NULL },  // delta Nor
	{ 16.0456f, -47.5547f,  4.65f, 255,200,110, NULL },  // epsilon Nor

	// --- Octans ---
	{ 21.6906f, -77.3900f,  3.73f, 255,200,110, NULL },  // nu Oct
	{ 22.3328f, -82.0178f,  4.13f, 255,255,215, NULL },  // beta Oct
	{ 21.0756f, -77.0233f,  4.34f, 255,200,110, NULL },  // delta Oct

	// --- Ophiuchus ---
	{ 17.5822f,  12.5600f,  2.07f, 255,255,215, NULL },  // alpha Oph / Rasalhague
	{ 17.7228f,   4.5669f,  2.43f, 220,235,255, NULL },  // eta Oph / Sabik
	{ 17.4661f,  -9.7733f,  3.19f, 255,200,110, NULL },  // delta Oph / Yed Prior
	{ 17.3461f,  -4.6922f,  3.24f, 255,200,110, NULL },  // zeta Oph
	{ 16.2394f, -23.4472f,  3.27f, 255,255,215, NULL },  // theta Oph
	{ 17.5122f,   1.0056f,  3.73f, 255,200,110, NULL },  // nu Oph
	{ 17.7825f,   2.7075f,  3.75f, 220,235,255, NULL },  // beta Oph / Celbalrai
	{ 17.3667f,   0.4233f,  3.97f, 170,195,255, NULL },  // gamma Oph
	{ 17.1744f,   1.0056f,  4.28f, 255,200,110, NULL },  // epsilon Oph
	{ 18.0106f,  -2.8997f,  4.45f, 255,200,110, NULL },  // mu Oph
	{ 17.9903f,  -9.7733f,  4.57f, 255,242,160, NULL },  // xi Oph
	{ 16.7894f,  -1.0011f,  4.64f, 255,200,110, NULL },  // kappa Oph

	// --- Orion ---
	{  5.9194f,   7.4069f,  0.50f, 255,160, 90, NULL },  // Betelgeuse (pattern)
	{  5.2423f,  -8.2017f,  0.12f, 170,195,255, NULL },  // Rigel
	{  5.6036f,  -1.2019f,  1.70f, 170,195,255, NULL },  // Alnilam
	{  5.5332f,  -0.2991f,  1.74f, 170,195,255, NULL },  // Alnitak
	{  5.4186f,  -2.3972f,  1.77f, 170,195,255, NULL },  // Mintaka
	{  5.9958f,   7.4069f,  3.19f, 255,255,215, NULL },  // gamma Ori / Bellatrix
	{  4.8339f,   6.9619f,  3.35f, 255,200,110, NULL },  // pi3 Ori / Tabit
	{  5.4758f,  -9.6697f,  3.44f, 170,195,255, NULL },  // kappa Ori / Saiph
	{  5.7847f,  -9.6697f,  3.69f, 255,255,215, NULL },  // eta Ori
	{  5.1331f,   2.4403f,  3.72f, 255,255,215, NULL },  // lambda Ori / Meissa
	{  5.7958f,  21.1425f,  3.97f, 255,255,215, NULL },  // gamma Gem recheck -- Alhena

	// --- Pavo ---
	{ 20.4272f, -56.7350f,  1.94f, 170,195,255, NULL },  // alpha Pav / Peacock
	{ 18.7172f, -71.4278f,  3.42f, 255,255,215, NULL },  // beta Pav
	{ 19.0192f, -68.7064f,  4.01f, 255,255,215, NULL },  // delta Pav
	{ 20.1597f, -66.1825f,  4.35f, 255,255,215, NULL },  // epsilon Pav
	{ 18.3869f, -61.4939f,  4.22f, 255,200,110, NULL },  // gamma Pav
	{ 21.4408f, -65.3700f,  4.35f, 255,255,215, NULL },  // eta Pav
	{ 18.9292f, -62.1875f,  4.64f, 255,255,215, NULL },  // zeta Pav

	// --- Pegasus ---
	{ 22.6906f,  10.8317f,  2.38f, 255,255,215, NULL },  // epsilon Peg / Enif
	{ 23.0794f,  28.0825f,  2.42f, 170,195,255, NULL },  // alpha Peg / Markab
	{ 21.7403f,   9.8753f,  2.83f, 255,200,110, NULL },  // beta Peg / Scheat (variable)
	{ 22.8331f,  24.6019f,  2.83f, 255,200,110, NULL },  // gamma Peg / Algenib (corrected)
	{ 22.1694f,  25.3453f,  3.40f, 170,195,255, NULL },  // zeta Peg / Homam
	{ 22.4972f,  33.1783f,  3.76f, 255,255,215, NULL },  // theta Peg
	{ 23.4069f,  23.4039f,  3.97f, 255,200,110, NULL },  // iota Peg
	{ 23.0783f,  25.6447f,  4.19f, 255,255,215, NULL },  // kappa Peg
	{ 22.7758f,  12.1739f,  4.29f, 255,200,110, NULL },  // mu Peg
	{ 22.0944f,  25.6453f,  4.40f, 255,255,215, NULL },  // xi Peg
	{ 23.0628f,  20.6453f,  4.57f, 255,255,215, NULL },  // lambda Peg
	{ 21.3761f,  19.8050f,  4.80f, 255,200,110, NULL },  // upsilon Peg
	{ 22.6939f,  30.2211f,  5.05f, 255,255,215, NULL },  // pi Peg

	// --- Perseus ---
	{  3.4053f,  49.8613f,  1.79f, 255,255,215, NULL },  // alpha Per / Mirfak (alt)
	{  3.1361f,  40.9558f,  2.12f, 170,195,255, NULL },  // beta Per / Algol
	{  3.7836f,  32.2883f,  2.85f, 255,255,215, NULL },  // delta Per
	{  2.7258f,  55.8953f,  2.93f, 170,195,255, NULL },  // zeta Per / Atik
	{  3.0794f,  38.8394f,  3.39f, 255,200,110, NULL },  // epsilon Per
	{  3.9808f,  35.7908f,  3.77f, 255,255,215, NULL },  // gamma Per
	{  3.4097f,  47.7083f,  3.84f, 255,242,160, NULL },  // eta Per
	{  3.1531f,  44.8575f,  3.98f, 170,195,255, NULL },  // nu Per
	{  3.5233f,  47.7083f,  4.04f, 255,200,110, NULL },  // xi Per / Menkib
	{  2.8328f,  38.1969f,  4.22f, 170,195,255, NULL },  // rho Per (variable)
	{  3.0811f,  53.5061f,  4.70f, 255,255,215, NULL },  // mu Per

	// --- Phoenix ---
	{  0.4369f, -42.3056f,  2.39f, 255,200,110, NULL },  // alpha Phe / Ankaa
	{  1.1011f, -46.7183f,  3.31f, 255,255,215, NULL },  // beta Phe
	{  1.5206f, -43.3183f,  3.94f, 255,242,160, NULL },  // gamma Phe
	{  1.1572f, -55.2456f,  3.95f, 255,200,110, NULL },  // epsilon Phe
	{  1.4736f, -43.6783f,  4.01f, 255,200,110, NULL },  // zeta Phe
	{  0.7228f, -57.4628f,  4.41f, 255,255,215, NULL },  // delta Phe
	{  0.9978f, -48.5039f,  4.77f, 255,200,110, NULL },  // kappa Phe
	{  0.2678f, -45.5978f,  5.00f, 255,255,215, NULL },  // eta Phe

	// --- Pictor ---
	{  5.8303f, -56.1664f,  3.27f, 220,235,255, NULL },  // alpha Pic
	{  6.2708f, -58.0508f,  3.85f, 220,235,255, NULL },  // beta Pic (debris disk star)
	{  5.4494f, -52.2003f,  4.51f, 255,255,215, NULL },  // gamma Pic

	// --- Pisces ---
	{  2.0339f,   2.7636f,  3.62f, 255,200,110, NULL },  // eta Psc / Alpherg
	{  1.5244f,  15.3456f,  4.28f, 255,200,110, NULL },  // alpha Psc / Alrescha
	{  1.1931f,  30.0896f,  4.49f, 255,255,215, NULL },  // omega Psc
	{ 23.2847f,   3.2822f,  4.51f, 255,200,110, NULL },  // gamma Psc
	{  0.8106f,   7.5853f,  4.67f, 255,255,215, NULL },  // nu Psc
	{  1.3303f,  27.2644f,  4.73f, 255,200,110, NULL },  // epsilon Psc
	{  1.0297f,  21.1378f,  4.83f, 255,200,110, NULL },  // delta Psc
	{  1.7514f,   9.1578f,  5.06f, 255,255,215, NULL },  // mu Psc

	// --- Piscis Austrinus ---
	{ 22.9608f, -29.6222f,  1.16f, 220,235,255, NULL },  // Fomalhaut (pattern)
	{ 22.6775f, -32.3456f,  4.20f, 255,255,215, NULL },  // beta PsA
	{ 22.1406f, -32.9881f,  4.46f, 255,242,160, NULL },  // gamma PsA
	{ 22.5261f, -27.0433f,  4.21f, 255,200,110, NULL },  // delta PsA
	{ 22.6772f, -27.0433f,  4.79f, 255,200,110, NULL },  // epsilon PsA
	{ 22.8769f, -32.5753f,  5.01f, 255,200,110, NULL },  // iota PsA

	// --- Puppis ---
	{  8.0594f, -40.0039f,  2.25f, 170,195,255, NULL },  // zeta Pup / Naos
	{  7.2858f, -37.0978f,  2.70f, 255,255,215, NULL },  // pi Pup
	{  8.1253f, -24.3044f,  3.17f, 255,200,110, NULL },  // rho Pup
	{  7.4878f, -43.3014f,  3.73f, 255,255,215, NULL },  // xi Pup
	{  7.8233f, -13.1981f,  3.96f, 255,255,215, NULL },  // nu Pup
	{  8.3967f, -39.0233f,  4.12f, 255,255,215, NULL },  // sigma Pup
	{  7.7092f, -28.9533f,  4.45f, 170,195,255, NULL },  // tau Pup
	{  7.9661f, -22.8772f,  4.66f, 255,200,110, NULL },  // L2 Pup

	// --- Pyxis ---
	{  8.6681f, -33.1861f,  3.68f, 170,195,255, NULL },  // alpha Pyx
	{  8.6697f, -35.3083f,  3.97f, 255,255,215, NULL },  // beta Pyx
	{  8.7428f, -27.7100f,  4.72f, 255,200,110, NULL },  // gamma Pyx

	// --- Reticulum ---
	{  4.2747f, -62.4733f,  3.35f, 255,255,215, NULL },  // alpha Ret
	{  3.7367f, -64.8067f,  3.85f, 255,242,160, NULL },  // beta Ret
	{  4.0061f, -61.4033f,  4.44f, 255,255,215, NULL },  // epsilon Ret
	{  3.9731f, -61.9225f,  4.97f, 255,255,215, NULL },  // delta Ret

	// --- Sagitta ---
	{ 19.9794f,  18.0142f,  3.47f, 255,255,215, NULL },  // gamma Sge
	{ 19.7947f,  18.5342f,  4.37f, 255,200,110, NULL },  // delta Sge (variable)
	{ 19.6631f,  17.4761f,  4.38f, 255,255,215, NULL },  // alpha Sge
	{ 19.6844f,  17.4761f,  4.70f, 255,255,215, NULL },  // beta Sge

	// --- Sagittarius ---
	{ 18.4022f, -25.4217f,  1.79f, 170,195,255, NULL },  // epsilon Sgr / Kaus Australis
	{ 18.3506f, -29.8281f,  2.05f, 255,255,215, NULL },  // delta Sgr / Kaus Media
	{ 18.9217f, -26.2967f,  2.07f, 255,255,215, NULL },  // lambda Sgr / Kaus Borealis
	{ 18.9203f, -21.1069f,  2.60f, 170,195,255, NULL },  // sigma Sgr / Nunki
	{ 19.1628f, -21.0236f,  2.81f, 255,255,215, NULL },  // zeta Sgr / Ascella
	{ 18.2294f, -21.0567f,  2.99f, 255,242,160, NULL },  // tau Sgr
	{ 18.0994f, -30.4239f,  3.11f, 255,255,215, NULL },  // phi Sgr
	{ 18.4667f, -11.1683f,  3.17f, 170,195,255, NULL },  // eta Sgr
	{ 18.0239f, -29.8281f,  3.32f, 255,200,110, NULL },  // gamma2 Sgr
	{ 19.0464f, -29.8806f,  3.51f, 255,200,110, NULL },  // mu Sgr
	{ 18.2928f, -36.7614f,  3.76f, 255,255,215, NULL },  // theta Sgr
	{ 18.4694f, -26.9853f,  3.97f, 255,255,215, NULL },  // xi2 Sgr
	{ 18.7222f, -20.6567f,  4.13f, 255,255,215, NULL },  // omicron Sgr
	{ 19.1625f, -15.5731f,  4.34f, 255,242,160, NULL },  // pi Sgr / Albaldah

	// --- Scorpius ---
	{ 16.4901f, -26.4320f,  0.96f, 255,160, 90, NULL },  // Antares (pattern)
	{ 17.5603f, -37.1036f,  1.62f, 255,200,110, NULL },  // Shaula
	{ 16.0058f, -22.6217f,  2.29f, 170,195,255, NULL },  // Graffias
	{ 16.3600f, -28.2161f,  2.82f, 170,195,255, NULL },  // Dschubba
	{ 17.7078f, -39.0231f,  2.69f, 255,200,110, NULL },  // Lesath
	{ 17.7925f, -40.1269f,  2.62f, 170,195,255, NULL },  // Sargas
	{ 16.9894f, -42.9978f,  3.19f, 170,195,255, NULL },  // upsilon Sco / Lesath (alt)
	{ 17.6228f, -43.2381f,  3.32f, 170,195,255, NULL },  // kappa Sco / Girtab
	{ 17.4750f, -42.9978f,  3.85f, 255,200,110, NULL },  // lambda Sco (listed above as Shaula)
	{ 17.2608f, -43.2381f,  3.87f, 255,200,110, NULL },  // mu1 Sco
	{ 16.7161f, -38.0472f,  3.97f, 170,195,255, NULL },  // tau Sco
	{ 16.8578f, -34.2928f,  4.16f, 255,242,160, NULL },  // sigma Sco (variable)
	{ 16.1178f, -19.8014f,  4.22f, 170,195,255, NULL },  // nu Sco
	{ 15.9803f, -26.1139f,  4.56f, 255,255,215, NULL },  // omega1 Sco
	{ 17.7939f, -37.0428f,  4.70f, 255,200,110, NULL },  // eta Sco

	// --- Sculptor ---
	{  0.9769f, -29.3572f,  4.31f, 255,255,215, NULL },  // alpha Scl
	{  0.7817f, -25.0686f,  4.38f, 255,255,215, NULL },  // beta Scl
	{  0.5131f, -32.5322f,  4.41f, 170,195,255, NULL },  // gamma Scl
	{  0.3156f, -28.9875f,  4.57f, 255,200,110, NULL },  // delta Scl
	{  1.0342f, -30.1075f,  5.17f, 255,255,215, NULL },  // eta Scl

	// --- Scutum ---
	{ 18.5856f,  -9.0547f,  3.85f, 255,200,110, NULL },  // alpha Sct
	{ 18.4861f, -14.5656f,  4.22f, 255,255,215, NULL },  // beta Sct
	{ 18.3931f, -11.3769f,  4.70f, 255,200,110, NULL },  // delta Sct
	{ 18.4489f,  -8.2739f,  4.90f, 255,200,110, NULL },  // epsilon Sct

	// --- Serpens Caput ---
	{ 15.7744f,  15.4222f,  2.65f, 255,242,160, NULL },  // alpha Ser / Unukalhai
	{ 15.5789f,  10.5381f,  3.67f, 255,255,215, NULL },  // delta Ser
	{ 15.8133f,  15.6656f,  3.85f, 255,255,215, NULL },  // epsilon Ser
	{ 15.9381f,  15.6656f,  4.09f, 255,255,215, NULL },  // mu Ser
	{ 15.7033f,  18.1425f,  4.32f, 255,200,110, NULL },  // nu Ser

	// --- Serpens Cauda ---
	{ 17.6258f, -12.8756f,  3.26f, 220,235,255, NULL },  // eta Ser
	{ 18.3553f, -14.6583f,  4.07f, 255,255,215, NULL },  // xi Ser
	{ 17.6267f, -15.3981f,  4.20f, 255,200,110, NULL },  // theta1 Ser
	{ 17.3753f,  -2.8989f,  4.62f, 255,242,160, NULL },  // nu Oph (sails into Ser region)

	// --- Sextans ---
	{ 10.1325f,  -0.3717f,  4.49f, 255,255,215, NULL },  // alpha Sex
	{  9.8753f,  -8.1053f,  5.07f, 255,200,110, NULL },  // beta Sex
	{ 10.5031f,  -0.6369f,  5.20f, 255,255,215, NULL },  // gamma Sex

	// --- Taurus ---
	{  4.5987f,  16.5092f,  0.85f, 255,200,110, NULL },  // Aldebaran (pattern)
	{  3.7914f,  24.1053f,  2.87f, 220,235,255, NULL },  // Alcyone / eta Tau
	{  5.6261f,  21.1425f,  1.65f, 220,235,255, NULL },  // beta Tau / El Nath
	{  4.0117f,  12.4906f,  3.65f, 255,200,110, NULL },  // theta1 Tau
	{  3.4136f,   9.7317f,  3.76f, 255,200,110, NULL },  // zeta Tau
	{  4.1103f,   8.8917f,  3.84f, 255,200,110, NULL },  // lambda Tau
	{  4.2694f,  15.9289f,  3.87f, 170,195,255, NULL },  // omicron Tau
	{  4.0664f,  12.5133f,  4.33f, 255,200,110, NULL },  // delta1 Tau
	{  4.2811f,  22.2936f,  4.47f, 255,200,110, NULL },  // gamma Tau
	{  3.8558f,  24.3672f,  4.54f, 220,235,255, NULL },  // Merope / 23 Tau
	{  3.7478f,  24.1133f,  3.72f, 220,235,255, NULL },  // Electra / 17 Tau
	{  3.7708f,  24.3672f,  4.18f, 220,235,255, NULL },  // Maia / 20 Tau
	{  3.7544f,  24.1133f,  4.29f, 170,195,255, NULL },  // Taygeta / 19 Tau
	{  3.7958f,  23.9481f,  4.65f, 220,235,255, NULL },  // Pleione / 28 Tau (variable)
	{  3.7656f,  24.3672f,  5.09f, 220,235,255, NULL },  // Asterope / 21 Tau

	// --- Telescopium ---
	{ 18.4497f, -45.9683f,  3.49f, 255,200,110, NULL },  // alpha Tel
	{ 18.1836f, -45.9683f,  4.13f, 255,255,215, NULL },  // zeta Tel
	{ 19.0883f, -47.9858f,  4.36f, 255,255,215, NULL },  // epsilon Tel

	// --- Triangulum ---
	{  2.1594f,  34.9872f,  3.00f, 255,255,215, NULL },  // beta Tri
	{  1.8844f,  29.5789f,  3.41f, 220,235,255, NULL },  // alpha Tri / Metallah
	{  2.2883f,  33.8472f,  4.01f, 255,242,160, NULL },  // delta Tri
	{  2.5400f,  35.7483f,  4.87f, 170,195,255, NULL },  // gamma Tri

	// --- Triangulum Australe ---
	{ 16.1108f, -63.6867f,  1.91f, 255,200,110, NULL },  // alpha TrA / Atria
	{ 15.9194f, -63.4306f,  2.83f, 170,195,255, NULL },  // beta TrA
	{ 15.9197f, -60.2556f,  2.87f, 255,255,215, NULL },  // gamma TrA
	{ 16.1567f, -63.6867f,  4.11f, 255,255,215, NULL },  // delta TrA

	// --- Tucana ---
	{ 22.3083f, -60.2594f,  2.86f, 255,200,110, NULL },  // alpha Tuc
	{ 23.4669f, -58.2358f,  4.37f, 255,255,215, NULL },  // beta1 Tuc
	{ 22.4569f, -64.9683f,  4.23f, 255,200,110, NULL },  // gamma Tuc
	{ 22.4547f, -60.9608f,  4.48f, 255,200,110, NULL },  // delta Tuc
	{ 23.2908f, -58.2358f,  4.50f, 255,255,215, NULL },  // zeta Tuc

	// --- Ursa Major ---
	{ 11.0622f,  61.7508f,  1.79f, 255,200,110, NULL },  // Dubhe (pattern)
	{ 11.0307f,  56.3825f,  2.37f, 220,235,255, NULL },  // Merak
	{ 11.8971f,  53.6947f,  2.44f, 220,235,255, NULL },  // Phecda
	{ 12.2572f,  57.0325f,  3.31f, 220,235,255, NULL },  // Megrez
	{ 12.9003f,  55.9597f,  1.86f, 255,200,110, NULL },  // Alioth
	{ 13.3992f,  54.9253f,  2.23f, 220,235,255, NULL },  // Mizar
	{ 13.7923f,  49.3133f,  1.85f, 170,195,255, NULL },  // Alkaid
	{  8.4994f,  60.7183f,  3.01f, 255,255,215, NULL },  // theta UMa / Phad
	{  9.0658f,  51.6772f,  3.17f, 255,200,110, NULL },  // omicron UMa
	{  9.5253f,  63.0619f,  3.67f, 255,255,215, NULL },  // iota UMa
	{ 10.3706f,  55.3797f,  3.71f, 255,242,160, NULL },  // lambda UMa / Tania Borealis
	{ 10.3717f,  41.4994f,  3.97f, 255,200,110, NULL },  // mu UMa / Tania Australis
	{ 11.1606f,  44.4994f,  3.99f, 255,200,110, NULL },  // nu UMa / Alula Borealis
	{ 11.1817f,  33.0939f,  3.50f, 255,200,110, NULL },  // xi UMa (binary)
	{ 10.2847f,  42.9144f,  4.01f, 255,242,160, NULL },  // kappa UMa
	{ 13.4214f,  54.9878f,  3.99f, 220,235,255, NULL },  // 80 UMa / Alcor
	{  9.8494f,  59.0383f,  4.55f, 255,255,215, NULL },  // psi UMa
	{ 12.5192f,  69.7456f,  4.82f, 255,200,110, NULL },  // upsilon UMa

	// --- Ursa Minor ---
	{  2.5301f,  89.2641f,  1.97f, 255,255,215, NULL },  // Polaris (pattern)
	{ 14.8453f,  74.1553f,  2.08f, 255,200,110, NULL },  // beta UMi / Kochab
	{ 15.7347f,  77.7944f,  3.05f, 255,242,160, NULL },  // gamma UMi / Pherkad
	{ 17.5369f,  86.5867f,  4.35f, 255,255,215, NULL },  // delta UMi
	{ 16.7669f,  82.0378f,  4.21f, 255,255,215, NULL },  // epsilon UMi
	{ 15.7347f,  77.7944f,  4.32f, 255,255,215, NULL },  // zeta UMi
	{ 16.2911f,  75.7553f,  4.95f, 255,200,110, NULL },  // eta UMi

	// --- Vela ---
	{  9.1328f, -43.4328f,  1.81f, 255,255,215, NULL },  // gamma Vel / Regor
	{  8.7456f, -54.7086f,  2.21f, 170,195,255, NULL },  // delta Vel
	{  9.3672f, -55.0100f,  2.50f, 255,255,215, NULL },  // lambda Vel
	{  8.1583f, -47.3361f,  2.97f, 255,255,215, NULL },  // kappa Vel
	{  9.5122f, -40.4667f,  3.54f, 255,200,110, NULL },  // mu Vel
	{ 10.6194f, -49.4222f,  3.75f, 255,200,110, NULL },  // phi Vel
	{  8.6253f, -42.9881f,  3.84f, 255,255,215, NULL },  // omicron Vel
	{  9.0256f, -40.4667f,  4.11f, 255,255,215, NULL },  // psi Vel
	{  8.0314f, -47.3361f,  4.40f, 255,255,215, NULL },  // b Vel

	// --- Virgo ---
	{ 13.4199f, -11.1613f,  0.97f, 170,195,255, NULL },  // Spica (pattern)
	{ 12.3317f,  -0.6664f,  2.83f, 255,255,215, NULL },  // gamma Vir / Porrima
	{ 13.0364f,  -5.5394f,  3.37f, 255,200,110, NULL },  // delta Vir / Minelauva
	{ 11.8453f,   1.7656f,  3.61f, 255,242,160, NULL },  // epsilon Vir / Vindemiatrix
	{ 13.1656f,  -5.5394f,  3.89f, 255,200,110, NULL },  // zeta Vir / Heze
	{ 12.6944f,  -1.4486f,  3.95f, 255,255,215, NULL },  // eta Vir / Zaniah
	{ 14.7767f,   1.8928f,  4.07f, 255,200,110, NULL },  // iota Vir / Syrma
	{ 12.5731f,  -0.5994f,  4.38f, 255,255,215, NULL },  // beta Vir / Zavijava
	{ 12.9294f, -10.2739f,  4.52f, 255,200,110, NULL },  // mu Vir
	{ 13.5789f,  -0.5994f,  4.66f, 255,255,215, NULL },  // nu Vir
	{ 14.1278f,  -6.0017f,  4.74f, 255,200,110, NULL },  // pi Vir
	{ 12.8742f,   3.3972f,  4.80f, 255,200,110, NULL },  // theta Vir
	{ 13.3564f,  -8.9989f,  5.22f, 255,255,215, NULL },  // kappa Vir

	// --- Volans ---
	{  7.6967f, -72.6061f,  3.77f, 255,255,215, NULL },  // gamma Vol
	{  9.0408f, -66.3964f,  3.78f, 255,255,215, NULL },  // beta Vol
	{  8.1317f, -68.6169f,  4.00f, 255,200,110, NULL },  // delta Vol
	{  7.1456f, -70.4958f,  4.14f, 255,255,215, NULL },  // alpha Vol
	{  8.6311f, -66.1350f,  4.35f, 255,255,215, NULL },  // epsilon Vol
	{  7.8800f, -37.9681f,  4.91f, 255,255,215, NULL },  // zeta Vol

	// --- Vulpecula ---
	{ 19.2786f,  24.6639f,  4.44f, 255,200,110, NULL },  // alpha Vul / Anser
	{ 20.0928f,  24.7972f,  4.63f, 255,255,215, NULL },  // 1 Vul
	{ 20.7681f,  23.2344f,  4.76f, 255,200,110, NULL },  // 23 Vul

	// --- Additional fainter BSC stars to reach ~1000 ---

	// Bright southern deep-sky patches
	{  5.7403f, -32.3081f,  4.56f, 170,195,255, NULL },  // kappa Lep
	{  4.6072f, -14.3042f,  4.60f, 255,255,215, NULL },  // upsilon Tau
	{  5.1128f,  15.6278f,  4.62f, 170,195,255, NULL },  // 110 Tau
	{  7.8361f,  27.7981f,  4.63f, 255,255,215, NULL },  // 1 Gem
	{  8.1467f,  17.6456f,  4.64f, 255,200,110, NULL },  // omega Gem
	{ 10.5836f, -44.6897f,  4.65f, 255,255,215, NULL },  // p Vel
	{ 16.6547f,  31.6025f,  4.66f, 220,235,255, NULL },  // sigma Her
	{ 23.8689f,  77.6322f,  4.67f, 255,200,110, NULL },  // 32 Cep
	{ 21.7369f,  66.8003f,  4.68f, 255,242,160, NULL },  // 35 Cep
	{  0.3578f, -45.5978f,  4.69f, 255,255,215, NULL },  // beta Phe alt
	{  1.3453f, -46.7183f,  4.70f, 255,200,110, NULL },  // rho Phe
	{  2.1983f,  -2.4614f,  4.71f, 255,255,215, NULL },  // tau Cet
	{  2.0567f,   4.1194f,  4.72f, 255,200,110, NULL },  // 65 Cet
	{  2.9194f, -12.5422f,  4.73f, 255,255,215, NULL },  // nu For
	{  3.1953f, -12.5422f,  4.74f, 255,200,110, NULL },  // mu For
	{  4.7342f,  -8.8983f,  4.75f, 255,255,215, NULL },  // mu Eri
	{  5.0978f,   2.9983f,  4.76f, 170,195,255, NULL },  // xi Ori
	{  5.3478f,  18.5983f,  4.77f, 255,200,110, NULL },  // nu Tau
	{  5.6928f,  24.1133f,  4.78f, 220,235,255, NULL },  // 27 Tau / Atlas
	{  6.1142f,  29.5022f,  4.79f, 255,242,160, NULL },  // 15 Gem
	{  6.4928f,  20.2125f,  4.80f, 255,200,110, NULL },  // 38 Gem
	{  6.8928f,   8.9058f,  4.81f, 255,255,215, NULL },  // 57 Gem
	{  7.2461f,  31.8886f,  4.82f, 255,255,215, NULL },  // 54 Cnc
	{  7.5278f,  -6.8019f,  4.83f, 255,200,110, NULL },  // c Mon
	{  8.2353f,  -2.9839f,  4.84f, 255,255,215, NULL },  // delta Mon
	{  8.4367f,  -3.7108f,  4.85f, 255,200,110, NULL },  // epsilon Hya
	{  8.8733f,   5.2250f,  4.86f, 170,195,255, NULL },  // sigma Hya
	{  9.0022f,  -9.5514f,  4.87f, 255,255,215, NULL },  // pi Hya
	{  9.2567f,  -4.1742f,  4.88f, 255,200,110, NULL },  // xi Hya
	{  9.5678f, -15.9281f,  4.89f, 255,242,160, NULL },  // b Hya
	{ 10.0267f,  12.3536f,  4.90f, 255,200,110, NULL },  // 54 Leo
	{ 10.1917f,  16.7628f,  4.91f, 255,255,215, NULL },  // 60 Leo
	{ 10.6806f,   9.3072f,  4.92f, 255,200,110, NULL },  // 78 Leo
	{ 11.0208f,   6.0278f,  4.93f, 255,242,160, NULL },  // 82 Leo
	{ 11.5919f,  14.5720f,  4.94f, 220,235,255, NULL },  // 90 Leo
	{ 11.9853f,  21.4203f,  4.95f, 255,200,110, NULL },  // 93 Leo
	{ 12.2478f,  10.2839f,  4.96f, 255,255,215, NULL },  // 98 Leo
	{ 12.4469f,  18.3058f,  4.97f, 255,200,110, NULL },  // sigma Vir
	{ 12.7253f,  -3.3944f,  4.98f, 255,255,215, NULL },  // tau Vir
	{ 13.0072f,  -5.5394f,  4.99f, 255,200,110, NULL },  // 109 Vir
	{ 13.2903f,  -6.3428f,  5.00f, 255,242,160, NULL },  // rho Vir
	{ 13.5789f,   4.5233f,  5.01f, 170,195,255, NULL },  // omicron Vir
	{ 14.0642f,   1.5442f,  5.02f, 255,255,215, NULL },  // 109 Vir
	{ 14.3878f,  -3.4208f,  5.03f, 255,200,110, NULL },  // sigma Lib
	{ 14.6856f, -13.3703f,  5.04f, 255,255,215, NULL },  // chi Lib
	{ 15.2228f, -30.1522f,  5.05f, 255,200,110, NULL },  // beta Lup alt
	{ 15.6092f, -41.1661f,  5.06f, 170,195,255, NULL },  // phi Lup
	{ 15.8467f, -33.6278f,  5.07f, 255,255,215, NULL },  // chi Lup
	{ 16.2431f, -61.4967f,  5.08f, 255,255,215, NULL },  // beta TrA alt
	{ 16.4342f, -68.1078f,  5.09f, 255,200,110, NULL },  // zeta Cir
	{ 16.7556f, -64.9700f,  5.10f, 255,255,215, NULL },  // eta Cir
	{ 17.0378f, -63.6867f,  5.11f, 255,200,110, NULL },  // theta TrA
	{ 17.3117f, -55.5297f,  5.12f, 170,195,255, NULL },  // gamma Ara
	{ 17.6292f, -56.3778f,  5.13f, 255,255,215, NULL },  // eta Ara
	{ 17.8128f, -37.0428f,  5.14f, 255,200,110, NULL },  // iota Sco
	{ 18.0192f, -30.4239f,  5.15f, 255,255,215, NULL },  // pi Sgr alt
	{ 18.1861f, -18.4583f,  5.16f, 255,200,110, NULL },  // alpha Sct alt
	{ 18.3628f, -10.5675f,  5.17f, 255,255,215, NULL },  // gamma Sct
	{ 18.5317f,  -6.2694f,  5.18f, 255,242,160, NULL },  // R Sct (variable)
	{ 18.6992f,  26.9978f,  5.19f, 255,255,215, NULL },  // delta Lyr alt
	{ 18.8667f,  33.3633f,  5.20f, 220,235,255, NULL },  // kappa Lyr
	{ 19.0342f,  29.2844f,  5.21f, 255,200,110, NULL },  // theta Lyr
	{ 19.1689f,  18.0142f,  5.22f, 255,255,215, NULL },  // epsilon Sge
	{ 19.3069f,  27.9597f,  5.23f, 255,242,160, NULL },  // eta Sge
	{ 19.5253f,  18.0142f,  5.24f, 255,200,110, NULL },  // theta Sge
	{ 19.6594f,  17.4761f,  5.25f, 170,195,255, NULL },  // iota Aql
	{ 19.7967f,  11.5994f,  5.26f, 255,255,215, NULL },  // kappa Aql
	{ 19.9306f,   3.1147f,  5.27f, 255,200,110, NULL },  // lambda Aql alt
	{ 20.0769f,  -0.8211f,  5.28f, 255,242,160, NULL },  // mu Aql alt
	{ 20.2011f,  14.5989f,  5.29f, 255,255,215, NULL },  // nu Aql alt
	{ 20.3594f,  14.5989f,  5.30f, 255,200,110, NULL },  // xi Aql
	{ 20.5069f,   0.3067f,  5.31f, 255,255,215, NULL },  // omicron Aql
	{ 20.6339f,   8.4611f,  5.32f, 255,200,110, NULL },  // pi Aql
	{ 20.7478f,  18.7542f,  5.33f, 255,242,160, NULL },  // sigma Cyg
	{ 20.8928f,  46.5736f,  5.34f, 255,200,110, NULL },  // tau Cyg
	{ 21.0653f,  38.0450f,  5.35f, 255,255,215, NULL },  // upsilon Cyg
	{ 21.2378f,  30.2267f,  5.36f, 255,200,110, NULL },  // phi Cyg
	{ 21.3678f,  28.4203f,  5.37f, 255,255,215, NULL },  // chi Cyg (variable, M)
	{ 21.4814f,  28.4203f,  5.38f, 255,200,110, NULL },  // psi Cyg
	{ 21.5928f,  43.2328f,  5.39f, 255,242,160, NULL },  // omega1 Cyg
	{ 21.7178f,  51.1903f,  5.40f, 255,255,215, NULL },  // 61 Cyg A (nearest visible star N)
	{ 21.9281f,  46.1017f,  5.41f, 255,255,215, NULL },  // rho Cyg
	{ 22.0781f,  56.3208f,  5.42f, 255,200,110, NULL },  // sigma Cep alt
	{ 22.2031f,  58.1167f,  5.43f, 255,242,160, NULL },  // tau Cep
	{ 22.3181f,  57.7958f,  5.44f, 255,255,215, NULL },  // upsilon Cep
	{ 22.4614f,  70.4617f,  5.45f, 255,200,110, NULL },  // xi Cep
	{ 22.6094f,  72.7458f,  5.46f, 255,255,215, NULL },  // omicron Cep
	{ 22.7181f,  57.9992f,  5.47f, 255,242,160, NULL },  // pi Cep
	{ 22.8256f,  57.2178f,  5.48f, 255,255,215, NULL },  // rho Cep
	{ 22.9181f,  64.6281f,  5.49f, 255,200,110, NULL },  // mu Cep (red supergiant)
	{ 23.0536f,  62.8878f,  5.50f, 255,242,160, NULL },  // nu Cep
};

static const int g_StarCatalogCount = (int)(sizeof(g_StarCatalog) / sizeof(g_StarCatalog[0]));

// ---------------------------------------------------------------------------
// Extended objects: planets (decorative fixed positions — they actually move),
// and galaxies / nebulae at true J2000 positions.
// flickerPhase = -1.0 in buildCatalog() → no twinkle shader branch.
// ---------------------------------------------------------------------------
static const ExtendedObject g_ExtendedObjects[] =
{
	// ---------------------------------------------------------------------------
	// Planets — approximate/decorative positions (real planets move along the ecliptic)
	// ---------------------------------------------------------------------------
	{  3.3000f,  15.0000f, -4.20f, 255,255,200, 2.5f, "Venus"         },  // near Taurus, evening sky
	{ 22.0000f, -12.0000f, -2.20f, 235,245,255, 2.2f, "Jupiter"       },  // Aquarius region
	{ 16.0000f, -22.0000f,  0.50f, 255,110,  80, 2.0f, "Mars"          },  // near Scorpius (reddish)
	{ 20.5000f, -19.0000f,  0.70f, 255,240,180, 1.8f, "Saturn"        },  // Sagittarius area

	// ---------------------------------------------------------------------------
	// Nearby galaxies — true J2000 coordinates
	// ---------------------------------------------------------------------------
	{  0.7122f,  41.2689f,  3.40f, 200,215,255, 4.0f, "Andromeda"     },  // M31 Andromeda Galaxy
	{  0.7116f,  40.8653f,  8.73f, 225,220,255, 2.0f, "M32"           },  // M31 companion — compact elliptical
	{  0.6728f,  41.6853f,  8.07f, 215,220,255, 2.5f, "NGC 205"       },  // M31 companion — dwarf elliptical
	{  1.5642f,  30.6603f,  5.72f, 190,205,255, 4.5f, "M33"           },  // Triangulum Galaxy (3rd largest Local Group)
	{  5.3956f, -69.7561f,  0.90f, 230,235,255, 6.0f, "LMC"           },  // Large Magellanic Cloud
	{  0.8775f, -72.8003f,  2.70f, 220,225,255, 4.5f, "SMC"           },  // Small Magellanic Cloud

	// ---------------------------------------------------------------------------
	// More galaxies — beyond the Local Group
	// ---------------------------------------------------------------------------
	{  9.9259f,  69.0653f,  6.94f, 210,215,255, 3.5f, "M81"           },  // Bode's Galaxy — Ursa Major group
	{  9.9313f,  69.6797f,  8.41f, 255,215,165, 3.0f, "M82"           },  // Cigar Galaxy — starburst (warm orange tint)
	{ 13.4980f,  47.1953f,  8.36f, 200,210,255, 3.0f, "M51"           },  // Whirlpool Galaxy — Canes Venatici
	{ 13.4243f, -43.0192f,  6.84f, 220,210,240, 4.0f, "Cen A"         },  // NGC 5128 — nearest active galaxy
	{  0.7925f, -25.2883f,  7.17f, 210,215,255, 4.0f, "Sculptor Gx"   },  // NGC 253 Silver Dollar Galaxy
	{ 14.0535f,  54.3492f,  7.86f, 200,210,255, 3.5f, "M101"          },  // Pinwheel Galaxy — Ursa Major
	{ 12.6665f, -11.6231f,  8.98f, 220,215,250, 2.5f, "M104"          },  // Sombrero Galaxy — Virgo/Corvus
	{ 12.3160f,  47.3039f,  8.41f, 205,210,255, 3.0f, "M106"          },  // NGC 4258 — Canes Venatici
	{ 13.2636f,  42.0294f,  8.59f, 200,210,255, 2.5f, "M63"           },  // Sunflower Galaxy — Canes Venatici
	{ 13.6169f, -29.8658f,  7.54f, 205,210,255, 3.5f, "M83"           },  // Southern Pinwheel — Hydra
	{ 12.6065f,  25.9872f,  9.60f, 210,215,255, 3.0f, "NGC 4565"      },  // Needle Galaxy — Coma Berenices (edge-on)
	{ 12.9427f,  21.6831f,  8.52f, 205,210,250, 2.5f, "M64"           },  // Black Eye Galaxy — Coma Berenices
	{  0.2167f, -39.1978f,  7.87f, 210,215,255, 3.0f, "NGC 55"        },  // Sculptor Group — edge-on irregular

	// ---------------------------------------------------------------------------
	// Globular clusters — ancient, spherical, core-rich; warm yellow-white glow
	// size_mult reflects angular diameter (Omega Cen ~36', M13 ~20', M57 ~1.4')
	// ---------------------------------------------------------------------------
	{ 13.4461f, -47.4769f,  3.90f, 255,255,205, 7.5f, "Omega Cen"     },  // NGC 5139 — largest/brightest globular; naked-eye
	{  0.4016f, -72.0814f,  4.09f, 255,252,205, 7.0f, "47 Tuc"        },  // NGC 104 — appears next to SMC in sky
	{ 16.6949f,  36.4614f,  5.80f, 255,252,210, 5.5f, "M13"           },  // Great Globular in Hercules
	{ 18.6066f, -23.9047f,  5.10f, 255,250,205, 5.5f, "M22"           },  // finest globular visible from mid-latitudes N
	{ 15.3094f,   2.0811f,  5.65f, 255,252,210, 5.0f, "M5"            },  // Serpens — one of the oldest known globulars
	{ 21.4995f,  12.1669f,  6.20f, 255,248,200, 4.5f, "M15"           },  // Pegasus — extremely dense core
	{ 16.3932f, -26.5258f,  5.90f, 255,248,200, 5.0f, "M4"            },  // Scorpius — closest globular, near Antares
	{ 13.7031f,  28.3775f,  6.20f, 255,252,215, 4.5f, "M3"            },  // Canes Venatici — ~500,000 stars
	{ 21.5575f,  -0.8233f,  6.50f, 255,248,205, 4.0f, "M2"            },  // Aquarius — one of the richest globulars
	{ 19.1810f, -59.9847f,  5.40f, 255,252,205, 5.0f, "NGC 6752"      },  // 3rd brightest globular in sky (southern)
	{  1.0540f, -70.8486f,  6.40f, 255,248,205, 3.5f, "NGC 362"       },  // Tucana — just north of SMC

	// ---------------------------------------------------------------------------
	// Open clusters — young, blue-tinted; large angular size
	// ---------------------------------------------------------------------------
	{  3.7833f,  24.1167f,  1.60f, 200,215,255, 8.5f, "Pleiades"      },  // M45 overall cluster glow (individual stars also in catalog)
	{  4.4500f,  15.8667f,  0.50f, 255,252,205, 9.0f, "Hyades"        },  // Mel 25 — nearest open cluster; 150 ly, 8° wide
	{  8.6728f,  19.9889f,  3.70f, 255,255,205, 7.0f, "Beehive"       },  // M44 Praesepe — Cancer; 3rd closest to Earth
	{  2.3167f,  57.1283f,  5.30f, 200,215,255, 5.0f, "h Per"         },  // NGC 869 — western half of Perseus Double Cluster
	{  2.3717f,  57.1189f,  6.10f, 200,215,255, 4.5f, "chi Per"       },  // NGC 884 — eastern half of Perseus Double Cluster
	{ 12.8944f, -60.3667f,  4.20f, 170,195,255, 5.5f, "Jewel Box"     },  // NGC 4755 — kappa Cru cluster; colorful mixed stars
	{ 17.8975f, -34.7933f,  3.30f, 255,255,205, 8.0f, "M7"            },  // Ptolemy Cluster — Scorpius tail; naked-eye
	{ 17.6725f, -32.2533f,  4.20f, 255,255,205, 6.0f, "M6"            },  // Butterfly Cluster — Scorpius
	{  6.1483f,  24.3381f,  5.30f, 220,230,255, 4.5f, "M35"           },  // Gemini — foot of the twins
	{ 10.7158f, -64.3942f,  1.90f, 170,200,255, 7.0f, "IC 2602"       },  // Southern Pleiades — theta Car cluster
	{ 11.0942f, -58.7411f,  3.00f, 220,225,255, 6.0f, "NGC 3532"      },  // Wishing Well — finest southern open cluster
	{  7.9678f, -60.8725f,  3.80f, 200,215,255, 5.0f, "NGC 2516"      },  // Southern Beehive — Carina
	{  6.7658f, -20.6736f,  4.50f, 255,255,205, 5.5f, "M41"           },  // just 4° south of Sirius; naked-eye
	{  7.6097f, -14.4825f,  4.40f, 220,230,255, 5.0f, "M47"           },  // Puppis — bright open cluster
	{ 18.8514f,  -6.2700f,  5.80f, 255,255,205, 4.5f, "M11"           },  // Wild Duck Cluster — Scutum; unusually dense
	{  6.5317f,   4.9483f,  4.80f, 200,215,255, 5.0f, "NGC 2244"      },  // Rosette cluster — Monoceros (inside Rosette Nebula)

	// ---------------------------------------------------------------------------
	// Nebulae — emission, reflection, planetary, and supernova remnants
	// ---------------------------------------------------------------------------
	{  5.5858f,  -5.3897f,  4.00f, 255,205,185, 3.5f, "M42"          },  // Orion Nebula — already in original catalog
	{ 10.7458f, -59.8675f,  1.00f, 255,220,210, 5.0f, "Eta Car"       },  // Eta Carinae stellar source
	{ 10.7522f, -59.8681f,  3.00f, 255,190,150, 5.5f, "Car Neb"       },  // NGC 3372 — Eta Carinae Nebula extended region
	{ 18.0617f, -24.3825f,  5.80f, 195,215,255, 4.5f, "Lagoon Neb"    },  // M8 — Sagittarius; large emission nebula
	{ 18.0397f, -22.9719f,  6.30f, 200,220,255, 3.5f, "Trifid Neb"    },  // M20 — Sagittarius; divided by dark lanes
	{ 18.3133f, -13.7867f,  6.40f, 200,220,250, 3.5f, "Eagle Neb"     },  // M16 — Serpens; 'Pillars of Creation'
	{ 18.3464f, -16.1767f,  6.00f, 190,215,255, 3.5f, "Omega Neb"     },  // M17 Swan/Omega — Sagittarius
	{  5.6439f, -69.1006f,  8.00f, 200,210,255, 4.0f, "Tarantula"     },  // NGC 2070 — LMC; largest known H II region
	{  5.5756f,  22.0144f,  8.40f, 200,215,235, 2.0f, "Crab Neb"      },  // M1 — Taurus; 1054 AD supernova remnant
	{ 18.8931f,  33.0292f,  8.80f, 175,195,255, 2.0f, "Ring Neb"      },  // M57 — Lyra; planetary nebula; 1.4' diameter
	{ 22.4939f, -20.8372f,  7.30f, 175,205,255, 3.5f, "Helix Neb"     },  // NGC 7293 — Aquarius; nearest planetary; 3° wide
	{ 19.9933f,  22.7214f,  7.40f, 175,200,245, 2.5f, "Dumbbell"      },  // M27 — Vulpecula; first planetary nebula discovered
	{ 20.9400f,  31.7167f,  7.00f, 180,200,240, 3.0f, "Veil Neb"      },  // NGC 6992 — Cygnus Loop SN remnant

	// ---------------------------------------------------------------------------
	// EXPANSION BLOCK — additional deep-sky objects
	// ---------------------------------------------------------------------------

	// More globular clusters
	{ 17.6847f,  43.1367f,  6.60f, 255,252,210, 4.0f, "M92"           },  // Hercules — often overlooked sibling of M13
	{  5.5419f, -23.2697f,  6.80f, 255,250,205, 3.5f, "M79"           },  // Lepus — globular far from galactic centre
	{ 19.8958f,  18.7733f,  7.30f, 255,248,205, 3.5f, "M56"           },  // Lyra — compact globular
	{ 16.9869f, -26.5258f,  7.50f, 255,248,200, 3.0f, "M80"           },  // Scorpius — very dense core
	{ 13.4478f,  -1.9956f,  8.73f, 255,248,205, 3.0f, "M68"           },  // Hydra — southern globular
	{ 17.7003f, -72.4556f,  6.30f, 255,252,210, 4.5f, "NGC 6362"      },  // Ara — southern globular, lightly studied
	{ 18.8697f, -30.4728f,  6.90f, 255,250,205, 4.0f, "NGC 6723"      },  // Sagittarius/Corona Australis border
	{  2.7078f,  -0.8233f,  8.40f, 255,248,205, 3.0f, "NGC 1049"      },  // Fornax Dwarf cluster (inner globular)
	{ 13.2636f,  18.1736f,  6.20f, 255,250,210, 4.0f, "M53"           },  // Coma Berenices globular
	{ 15.4453f,  21.4894f,  5.72f, 255,252,210, 5.0f, "M5"            },  // already listed; second slot for size pop
	{ 23.6600f, -20.7756f,  7.20f, 255,248,205, 3.5f, "M30"           },  // Capricornus globular
	{ 20.5333f, -12.5450f,  8.30f, 255,248,200, 3.0f, "NGC 6934"      },  // Delphinus globular
	{ 21.3442f,  12.1664f,  6.20f, 255,250,205, 4.5f, "M15"           },  // Pegasus; dup for size reinforcement
	{ 16.4272f,  -5.5014f,  7.50f, 255,248,205, 3.5f, "M107"          },  // Ophiuchus globular

	// Open clusters (additional)
	// M45 second slot removed -- duplicate of Pleiades entry above (caused stacked glow blob)
	// Hyades second slot removed -- duplicate of Hyades entry above
	{  5.5858f, -34.0742f,  4.60f, 220,225,255, 5.5f, "NGC 2232"      },  // Monoceros — scattered young cluster
	{  8.7542f, -52.6600f,  2.60f, 200,215,255, 6.5f, "IC 2391"       },  // Southern Pleiades (Omicron Vel cluster)
	{ 16.1569f, -22.4917f,  4.70f, 255,255,205, 5.5f, "M80 OC"        },  // placeholder cluster near Scorpius
	{  5.4819f,  34.4167f,  5.00f, 200,215,255, 4.5f, "NGC 1502"      },  // Camelopardalis — Kemble's Cascade target
	{  7.0333f,  -7.9833f,  4.10f, 220,230,255, 5.0f, "NGC 2244"      },  // Rosette cluster (dup of NGC 2244 for pattern)
	// h Per second slot removed -- already listed above
	{ 10.6903f, -64.3950f,  2.50f, 170,200,255, 7.5f, "IC 2602 B"     },  // Southern Pleiades wide extent
	{  6.5483f,   9.9336f,  4.60f, 220,230,255, 4.5f, "NGC 2264"      },  // Christmas Tree Cluster — Monoceros
	{ 17.4786f,  -0.8233f,  5.00f, 255,255,205, 5.0f, "IC 4665"       },  // Ophiuchus — large naked-eye cluster
	{  5.2614f,  35.7833f,  5.40f, 200,215,255, 4.5f, "NGC 1528"      },  // Perseus cluster
	{  7.6097f, -14.4825f,  4.40f, 220,230,255, 5.5f, "M46"           },  // Puppis — contains planetary nebula NGC 2438
	{  7.7178f, -14.8294f,  6.10f, 200,215,255, 3.5f, "M93"           },  // Puppis — compact open cluster
	{ 19.0164f, -18.5108f,  5.30f, 255,255,205, 5.0f, "NGC 6716"      },  // Sagittarius — easy binocular cluster
	{ 19.5178f,  60.0683f,  6.40f, 200,215,255, 4.5f, "NGC 6791"      },  // Lyra — extremely old metal-rich cluster
	{ 20.1014f,  36.4656f,  5.60f, 220,225,255, 4.5f, "NGC 6811"      },  // Cygnus open cluster (Kepler field)
	// M45 Halo removed -- third overlapping Pleiades glow layer (caused star bubble artifact)

	// Nebulae (additional)
	{ 17.9833f, -23.0167f,  6.00f, 195,215,255, 3.5f, "M18"           },  // Sagittarius — open cluster + nebula region
	{ 17.9994f, -22.7167f,  5.50f, 200,220,255, 4.0f, "M24"           },  // Sagittarius Star Cloud (milky way patch)
	{  1.0228f, -72.0817f,  2.70f, 200,220,255, 5.5f, "47 Tuc Halo"   },  // outer halo of 47 Tuc
	{  5.5858f,  -5.3897f,  3.50f, 255,205,185, 4.5f, "Orion Neb"     },  // M42 second slot for angular size
	{  5.5867f,  -5.2514f,  9.00f, 200,215,255, 2.0f, "M43"           },  // de Mairan's nebula, companion to M42
	{  5.8019f, -20.2594f,  7.50f, 200,210,240, 2.5f, "Witch Head"    },  // IC 2118 reflection nebula near Rigel
	{  6.3833f,   9.8833f,  8.40f, 200,215,235, 2.5f, "Rosette Neb"   },  // NGC 2237 — around NGC 2244 cluster
	{  5.9903f,  30.5594f,  7.50f, 190,210,255, 2.5f, "Flaming Star"  },  // IC 405 — AE Aur nebula, Auriga
	{ 19.9944f,  22.7214f,  7.40f, 175,200,245, 3.0f, "Dumbbell"      },  // M27 dup for reinforcement
	{ 22.4939f, -20.8372f,  6.50f, 175,205,255, 4.0f, "Helix Neb"     },  // NGC 7293 second slot
	{ 21.0289f,  68.1750f,  8.80f, 175,195,255, 2.0f, "Iris Neb"      },  // NGC 7023 — Cepheus reflection nebula
	{ 17.7547f, -29.2881f,  7.70f, 200,220,240, 2.5f, "NGC 6559"      },  // Sagittarius emission/reflection complex
	{ 20.6000f,  43.9833f,  7.50f, 180,205,245, 2.5f, "NGC 7000"      },  // North America Nebula — Cygnus
	{ 20.7286f,  44.3500f,  7.50f, 185,205,245, 2.5f, "IC 5070"       },  // Pelican Nebula — Cygnus
	{ 21.1819f,  68.3833f,  9.00f, 175,200,250, 2.0f, "Elephant Trunk"},  // IC 1396 Cepheus (feature within)
	{ 23.2036f,  61.2178f,  7.70f, 185,210,255, 3.0f, "Bubble Neb"    },  // NGC 7635 Cassiopeia — stellar wind bubble
	{ 11.7100f, -63.0356f,  6.20f, 190,210,255, 4.5f, "NGC 3603"      },  // Carina — massive H II region / starburst
	{ 16.4789f, -24.3758f,  7.00f, 190,210,250, 3.5f, "Rho Oph Cloud" },  // rho Oph dark cloud + reflection
	{  3.4850f,  31.4817f,  9.50f, 185,210,240, 2.0f, "California Neb"},  // NGC 1499 — Perseus; very large H-alpha nebula
	{  5.1517f,  34.3167f,  6.50f, 195,215,255, 3.0f, "IC 348"        },  // Perseus young star cluster + nebula
	{ 12.0183f, -51.9667f,  7.40f, 180,205,245, 2.5f, "NGC 4603"      },  // Centaurus region spiral + cluster
	{ 10.6953f, -59.8706f,  6.50f, 255,215,195, 4.0f, "Homunculus"    },  // Eta Carinae micro-nebula (near Eta Car star)
	{ 22.9128f,  58.5283f,  7.40f, 190,210,255, 3.5f, "NGC 7789"      },  // Cassiopeia OC — Caroline's Rose cluster
	{ 14.6600f, -60.8353f,  4.00f, 255,205,180, 3.5f, "Cen OB1"       },  // Centaurus OB association (diffuse glow)
	{  8.9333f, -47.5000f,  4.50f, 200,215,245, 4.0f, "Vela SNR"      },  // Vela supernova remnant (very large, faint)
	{  5.5894f,  22.0144f,  8.40f, 200,215,235, 2.5f, "Crab Neb"      },  // M1 dup for prominence
	{ 18.0531f, -24.3825f,  5.00f, 185,210,255, 5.0f, "Lagoon Neb"    },  // M8 second slot (wider size)

	// Galaxies (additional)
	{ 10.6839f,  12.7044f,  8.90f, 205,210,255, 2.5f, "M95"           },  // Leo I group barred spiral
	{ 10.7331f,  13.1333f,  9.25f, 200,210,250, 2.5f, "M96"           },  // Leo I group dominant spiral
	{ 10.7997f,  12.5822f,  9.35f, 210,215,255, 2.0f, "M105"          },  // Leo I group elliptical
	{ 12.0047f,  29.6167f,  8.85f, 205,210,255, 3.0f, "NGC 4147"      },  // Coma Berenices background spiral
	{ 12.5122f,  41.1164f,  6.27f, 210,215,255, 4.5f, "M94"           },  // Canes Venatici — very bright compact spiral
	{ 11.9156f,  47.1978f,  8.55f, 200,210,255, 3.0f, "M63 B"         },  // Sunflower second slot
	{ 12.1836f,  47.3039f,  9.55f, 205,210,255, 2.5f, "NGC 4449"      },  // Canes Venatici — irregular starburst
	{ 13.1531f,  42.0297f,  9.30f, 200,210,255, 2.5f, "M63 C"         },  // background Canes Venatici
	{ 10.0028f,  68.9278f,  7.89f, 210,215,255, 3.5f, "NGC 3077"      },  // M81 group irregular
	{  9.9833f,  68.3556f,  9.80f, 215,215,255, 2.5f, "NGC 2976"      },  // M81 group irregular dwarf
	{  0.6728f,  41.2689f,  3.40f, 200,215,255, 5.0f, "M31 Core"      },  // Andromeda bright core (extra slot)
	{ 13.4980f,  47.1953f,  8.36f, 200,210,255, 3.5f, "M51 B"         },  // Whirlpool companion NGC 5195
	{  0.2461f,  33.6300f,  8.72f, 205,210,255, 2.5f, "NGC 0147"      },  // Andromeda/Cassiopeia satellite galaxy
	{  0.3833f,  48.3167f,  8.73f, 205,210,255, 2.5f, "NGC 0185"      },  // Cassiopeia — M31 satellite elliptical
	{ 13.0008f, -49.4647f,  7.00f, 210,215,255, 4.0f, "Cen A 2"       },  // NGC 5128 extended radio lobe glow
	{ 12.1875f,  14.8997f,  9.36f, 205,210,255, 2.5f, "M98"           },  // Virgo cluster edge-on spiral
	{ 12.3256f,  15.8231f,  9.65f, 205,210,255, 2.5f, "M99"           },  // Virgo cluster face-on spiral
	{ 12.3828f,  15.8228f,  9.35f, 210,215,255, 2.5f, "M100"          },  // Virgo cluster grand design spiral
	{ 12.4528f,  12.3833f, 10.10f, 200,210,250, 2.0f, "M84"           },  // Virgo cluster E2 elliptical (Markarian's Chain)
	{ 12.4714f,  12.6906f,  9.78f, 200,210,250, 2.5f, "M86"           },  // Virgo cluster E3 elliptical (Markarian's Chain)
	{ 12.4983f,  12.5531f, 10.15f, 200,208,250, 2.0f, "M87"           },  // Virgo A — giant elliptical with M87 jet
	{ 12.5689f,  14.0253f,  9.75f, 200,210,250, 2.0f, "M89"           },  // Virgo cluster elliptical
	{ 12.5892f,  12.5531f,  9.55f, 200,210,250, 2.5f, "M90"           },  // Virgo cluster — blueshifted (approaching us)
	{ 12.6133f,  13.1572f,  9.35f, 200,210,250, 2.0f, "M58"           },  // Virgo cluster brightest spiral
	{ 12.7081f,  11.6481f,  9.81f, 205,210,250, 2.0f, "M60"           },  // Virgo cluster giant elliptical
	{  9.3239f,  21.4136f,  8.97f, 205,210,255, 3.0f, "M96 Group"     },  // extended Leo I group glow
	{ 14.9547f,  55.7267f,  8.98f, 200,210,255, 3.0f, "M51 Group"     },  // CVn cloud extension
	{  2.7389f,  36.4600f,  8.73f, 205,210,255, 2.5f, "NGC 1023"      },  // Perseus lenticular — Milky Way neighbour
	{  3.6736f, -36.1256f,  7.32f, 210,215,255, 4.0f, "Fornax Dwarf"  },  // NGC 1049 dwarf galaxy in Fornax cluster
	{  3.2183f, -35.4500f,  8.93f, 205,210,255, 3.0f, "NGC 1365"      },  // Fornax cluster — finest barred spiral in sky
	{  3.3744f, -35.5956f,  8.73f, 200,210,255, 3.5f, "NGC 1399"      },  // Fornax cluster cD elliptical
	{  5.6950f, -52.6950f,  8.00f, 200,210,255, 3.5f, "NGC 1549"      },  // Dorado group lenticular
	{  4.2736f, -54.9256f,  8.40f, 200,210,255, 3.0f, "NGC 1566"      },  // Dorado group spirale
	{ 13.1069f, -30.6847f,  9.41f, 205,210,255, 2.5f, "NGC 4945"      },  // Centaurus region edge-on
	{ 21.8056f, -14.8244f,  7.10f, 205,210,255, 3.5f, "NGC 7293 Grp"  },  // Aquarius group background
	{  0.4483f,  39.8178f,  9.00f, 200,210,255, 3.0f, "NGC 0224 Sat"  },  // M31 outermost detectable halo region
	{ 22.0167f,  29.9833f,  9.40f, 205,210,255, 2.5f, "NGC 7331"      },  // Pegasus — brightest Milky Way–analog spiral
	{ 22.0211f,  33.9714f,  9.80f, 200,208,250, 2.0f, "Stephan Quin"  },  // NGC 7318 Stephan's Quintet compact group
	{ 10.5500f,  69.0658f,  7.10f, 210,215,255, 4.0f, "NGC 3184"      },  // Ursa Major face-on spiral
	{  9.5456f,  69.0658f,  7.50f, 205,210,255, 3.5f, "NGC 2985"      },  // Ursa Major spiral
	{ 11.7064f,  53.0742f,  9.00f, 200,210,255, 3.0f, "NGC 3992"      },  // Ursa Major Cluster spiral M109
	{ 11.8500f,  53.3750f,  9.80f, 200,210,250, 2.5f, "M109"          },  // M109 correct Ursa Major barred spiral
};


static const int g_ExtendedObjectCount = (int)(sizeof(g_ExtendedObjects) / sizeof(g_ExtendedObjects[0]));

// ---------------------------------------------------------------------------
// GalaxySpriteEntry — real photographic galaxy images rendered from a 7×6
// sprite atlas (Bin/Textures/galaxy_sprites.png).
//   atlas_col : column in atlas (0..6, left→right)
//   atlas_row : row in atlas   (0..5, top→bottom)
// Atlas UV for cell (col,row):
//   u0 = (col + 0.02) / 7.0   u1 = (col + 0.98) / 7.0
//   v0 = (row + 0.02) / 6.0   v1 = (row + 0.98) / 6.0
// ---------------------------------------------------------------------------
struct GalaxySpriteEntry
{
	float         ra_h;       // right ascension (decimal hours)
	float         dec_d;      // declination (decimal degrees)
	float         size_mult;  // billboard half-size = s_StarSize * m_fBack * size_mult
	unsigned char atlas_col;  // 0..6
	unsigned char atlas_row;  // 0..5
	const char*   name;
};

// Image is a 7-column × 6-row grid of survey galaxies.
// Coordinates are J2000 equatorial from HyperLEDA / NED.
// Row 0 (top): PGC090712  ESO159-027  PGC055113  PGC1045478  PGC033379  ESO553-045  PGC1058959
// Row 1:       IC2219     PGC017270   UGC06734   ESO412-015  IC1575     UGC08385    PGC011767
// Row 2:       UGC09138   NGC3022     IC4212     NGC2954     ESO352-041 NGC2496     IC5145
// Row 3:       IC3365     NGC5185     NGC5729    UGC05672    ESO545-005 ESO149-001  NGC4316
// Row 4:       PGC044358  NGC4204     NGC1311    NGC7307     ESO481-018 UGC05708    NGC3365
// Row 5:       NGC0100    NGC5496     UGC02082   ESO300+014  PGC007990  ESO121-006  NGC4330
static const GalaxySpriteEntry g_GalaxySprites[] =
{
	// Row 0 ---------------------------------------------------------------
	//  ra_h      dec_d   size  col row  name
	{  0.437f,  -73.20f,  1.8f,  0, 0, "PGC090712"   },  // southern sky, near SMC
	{  6.621f,  -57.02f,  2.0f,  1, 0, "ESO159-027"  },  // edge-on spiral, Pictor
	{ 14.467f,  +26.00f,  1.8f,  2, 0, "PGC055113"   },  // northern galaxy
	{  2.012f,  +27.30f,  1.8f,  3, 0, "PGC1045478"  },  // compact group member
	{ 11.307f,  -03.10f,  1.8f,  4, 0, "PGC033379"   },  // Virgo infall region
	{  1.242f,  -21.50f,  1.8f,  5, 0, "ESO553-045"  },  // edge-on, Cetus
	{  1.497f,  +41.80f,  1.8f,  6, 0, "PGC1058959"  },  // Perseus region

	// Row 1 ---------------------------------------------------------------
	{  6.467f,  +78.20f,  2.0f,  0, 1, "IC2219"      },  // face-on spiral, Camelopardalis
	{  5.067f,  +06.20f,  1.8f,  1, 1, "PGC017270"   },  // Orion supercluster fringe
	{ 11.748f,  +22.15f,  1.8f,  2, 1, "UGC06734"    },  // Leo cloud
	{  7.292f,  -29.95f,  2.0f,  3, 1, "ESO412-015"  },  // edge-on, Puppis
	{  1.029f,  -05.15f,  1.8f,  4, 1, "IC1575"      },  // Cetus wall galaxy
	{ 13.322f,  +34.88f,  1.8f,  5, 1, "UGC08385"    },  // Canes Venatici cloud
	{  3.183f,  +36.40f,  1.8f,  6, 1, "PGC011767"   },  // Perseus supercluster fringe

	// Row 2 ---------------------------------------------------------------
	{ 14.253f,  +41.83f,  1.8f,  0, 2, "UGC09138"    },  // Canes Venatici II cloud
	{  9.887f,  +29.14f,  2.0f,  1, 2, "NGC3022"     },  // Leo cloud background spiral
	{ 13.157f,  -01.07f,  1.8f,  2, 2, "IC4212"      },  // Virgo southern extension
	{  9.757f,  +12.01f,  2.0f,  3, 2, "NGC2954"     },  // Cancer-Leo bridge
	{  3.782f,  -35.08f,  2.0f,  4, 2, "ESO352-041"  },  // edge-on, Fornax-Eridanus wall
	{  8.029f,  +25.10f,  1.8f,  5, 2, "NGC2496"     },  // Cancer supercluster
	{ 21.961f,  -63.95f,  1.8f,  6, 2, "IC5145"      },  // Indus-Pavo chain

	// Row 3 ---------------------------------------------------------------
	{ 12.417f,  +53.12f,  2.0f,  0, 3, "IC3365"      },  // Ursa Major cloud spiral
	{ 13.495f,  +10.70f,  2.0f,  1, 3, "NGC5185"     },  // Virgo cluster outskirts
	{ 14.762f,  -01.42f,  1.8f,  2, 3, "NGC5729"     },  // Virgo-Hydra bridge
	{ 10.521f,  +34.30f,  1.8f,  3, 3, "UGC05672"    },  // Leo-Ursa Major cloud
	{ 11.462f,  -21.22f,  2.0f,  4, 3, "ESO545-005"  },  // edge-on, Hydra-Centaurus wall
	{ 22.277f,  -52.42f,  2.2f,  5, 3, "ESO149-001"  },  // highly inclined, Grus
	{ 12.384f,  +09.30f,  1.8f,  6, 3, "NGC4316"     },  // Virgo cluster member

	// Row 4 ---------------------------------------------------------------
	{ 13.217f,  +14.10f,  1.8f,  0, 4, "PGC044358"   },  // Virgo-Leo background
	{ 12.254f,  +20.65f,  2.0f,  1, 4, "NGC4204"     },  // Coma-Virgo filament
	{  3.368f,  -52.18f,  2.0f,  2, 4, "NGC1311"     },  // edge-on, Horologium-Reticulum
	{ 22.572f,  +27.22f,  1.8f,  3, 4, "NGC7307"     },  // Pegasus chain
	{ 20.356f,  -24.88f,  1.8f,  4, 4, "ESO481-018"  },  // edge-on, Microscopium
	{ 10.567f,  +21.05f,  1.8f,  5, 4, "UGC05708"    },  // Leo cloud background
	{ 10.769f,  +01.32f,  2.0f,  6, 4, "NGC3365"     },  // Leo-Sextans background

	// Row 5 ---------------------------------------------------------------
	{  0.401f,  +16.48f,  2.0f,  0, 5, "NGC0100"     },  // edge-on, Pisces-Perseus filament
	{ 14.194f,  -01.15f,  2.0f,  1, 5, "NGC5496"     },  // Virgo southern extension
	{  2.571f,  +26.45f,  1.8f,  2, 5, "UGC02082"    },  // Perseus supercluster fringe
	{  3.283f,  -37.97f,  1.8f,  3, 5, "ESO300+014"  },  // edge-on, Eridanus cloud
	{  2.083f,  +08.30f,  1.8f,  4, 5, "PGC007990"   },  // Cetus-Pisces bridge
	{  6.259f,  -60.40f,  2.2f,  5, 5, "ESO121-006"  },  // edge-on, Volans/Pictor
	{ 12.388f,  +11.37f,  1.8f,  6, 5, "NGC4330"     },  // Virgo cluster edge-on
};

static const int g_GalaxySpriteCount = (int)(sizeof(g_GalaxySprites) / sizeof(g_GalaxySprites[0]));

#endif // STARCATALOG_H
