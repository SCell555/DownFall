#include "cbase.h"
#include "func_water_analog.h"

#include "tier0/memdbgon.h"

//
// func_water_analog is implemented as a linear mover so we can raise/lower the water level.
//
LINK_ENTITY_TO_CLASS( func_water_analog, CFuncWaterAnalog );

BEGIN_DATADESC( CFuncWaterAnalog )
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFuncWaterAnalog, DT_WaterAnalog )
END_SEND_TABLE()

CFuncWaterAnalog::CFuncWaterAnalog()
{
}


CFuncWaterAnalog::~CFuncWaterAnalog()
{
}