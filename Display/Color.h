#pragma once
/*
	Color.h
	(c)2005 Palestar Inc
	Simple RGBA color class
*/

#ifndef COLOR_H
#define COLOR_H

#include "Standard/Types.h"
#include <d3d11.h>

//---------------------------------------------------------------------------------------------------

class Color
{
public:
	// Data members
	union {
		struct {
			byte r, g, b, a;
		};
		dword rgba;
	};

	// Construction
	Color() : r(255), g(255), b(255), a(255)
	{
	}

	Color(byte _r, byte _g, byte _b, byte _a = 255)
		: r(_r), g(_g), b(_b), a(_a)
	{
	}

	explicit Color(dword _rgba) : rgba(_rgba)
	{
	}

	Color(const Color& copy) : rgba(copy.rgba)
	{
	}

	// Operators
	Color& operator=(const Color& copy)
	{
		rgba = copy.rgba;
		return *this;
	}

	bool operator==(const Color& compare) const
	{
		return rgba == compare.rgba;
	}

	bool operator!=(const Color& compare) const
	{
		return rgba != compare.rgba;
	}

	Color operator*(float scale) const
	{
		return Color(
			(byte)(r * scale),
			(byte)(g * scale),
			(byte)(b * scale),
			a
		);
	}

	Color operator+(const Color& other) const
	{
		return Color(
			(byte)Min<int>(r + other.r, 255),
			(byte)Min<int>(g + other.g, 255),
			(byte)Min<int>(b + other.b, 255),
			(byte)Min<int>(a + other.a, 255)
		);
	}

	// Accessors
	dword RGBA() const { return rgba; }
	dword ARGB() const { return (a << 24) | (r << 16) | (g << 8) | b; }
	dword BGRA() const { return (b << 24) | (g << 16) | (r << 8) | a; }
	dword ABGR() const { return (a << 24) | (b << 16) | (g << 8) | r; }

	// Convert to float array (0.0 - 1.0 range)
	void toFloat4(float* pOut) const
	{
		pOut[0] = r / 255.0f;
		pOut[1] = g / 255.0f;
		pOut[2] = b / 255.0f;
		pOut[3] = a / 255.0f;
	}

	// Static factory methods
	static Color fromRGBA(dword rgba)
	{
		return Color(rgba);
	}

	static Color fromARGB(dword argb)
	{
		return Color(
			(byte)((argb >> 16) & 0xFF),
			(byte)((argb >> 8) & 0xFF),
			(byte)(argb & 0xFF),
			(byte)((argb >> 24) & 0xFF)
		);
	}

	static Color fromBGRA(dword bgra)
	{
		return Color(
			(byte)(bgra & 0xFF),
			(byte)((bgra >> 8) & 0xFF),
			(byte)((bgra >> 16) & 0xFF),
			(byte)((bgra >> 24) & 0xFF)
		);
	}

	static Color fromFloat4(const float* pColor)
	{
		return Color(
			(byte)(pColor[0] * 255.0f),
			(byte)(pColor[1] * 255.0f),
			(byte)(pColor[2] * 255.0f),
			(byte)(pColor[3] * 255.0f)
		);
	}
};

//---------------------------------------------------------------------------------------------------
// Common color constants
//---------------------------------------------------------------------------------------------------

const Color BLACK(0, 0, 0, 255);
const Color WHITE(255, 255, 255, 255);
const Color RED(255, 0, 0, 255);
const Color GREEN(0, 255, 0, 255);
const Color BLUE(0, 0, 255, 255);
const Color YELLOW(255, 255, 0, 255);
const Color CYAN(0, 255, 255, 255);
const Color MAGENTA(255, 0, 255, 255);
const Color TRANSPARENT(0, 0, 0, 0);

const Color GREY(128, 128, 128, 255);
const Color GRAY(128, 128, 128, 255);
const Color DARKGREY(64, 64, 64, 255);
const Color DARKGRAY(64, 64, 64, 255);
const Color LIGHTGREY(192, 192, 192, 255);
const Color LIGHTGRAY(192, 192, 192, 255);

const Color ORANGE(255, 165, 0, 255);
const Color PURPLE(128, 0, 128, 255);
const Color BROWN(165, 42, 42, 255);
const Color PINK(255, 192, 203, 255);

//---------------------------------------------------------------------------------------------------

#endif

//---------------------------------------------------------------------------------------------------
// EOF