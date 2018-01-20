#ifndef FUNC_WATER_ANALOG_H
#define FUNC_WATER_ANALOG_H

#pragma once
#include "func_movelinear.h"
class CFuncWaterAnalog : public CFuncMoveLinear
{
	DECLARE_CLASS( CFuncWaterAnalog, CFuncMoveLinear );
public:
	CFuncWaterAnalog();
	~CFuncWaterAnalog();

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};

#endif // FUNC_WATER_ANALOG_H