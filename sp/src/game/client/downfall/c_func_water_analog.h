#ifndef C_FUNC_WATER_ANALOG_H
#define C_FUNC_WATER_ANALOG_H

#pragma once
#include "c_baseentity.h"
#include "c_pixel_visibility.h"

class C_FuncWaterAnalog : public C_BaseEntity
{
	DECLARE_CLASS( C_FuncWaterAnalog, C_BaseEntity );
public:
	C_FuncWaterAnalog();
	~C_FuncWaterAnalog();

	void Spawn() OVERRIDE;
	void UpdateOnRemove() OVERRIDE;
	int DrawBrushModel(bool bTranslucent, int nFlags, bool bTwoPass) OVERRIDE;
	void PostDataUpdate(DataUpdateType_t updateType) OVERRIDE;

	bool IsSurfaceVisible() { return m_OcclusionSet.QueryNumPixelsRenderedForAllViewsLastFrame() > 0; }
	IMaterial* GetWaterVolumeMaterial( bool bUnder ) const { return bUnder ? m_underWaterVolumeMaterial : m_aboveWaterVolumeMaterial; }

	DECLARE_CLIENTCLASS();

private:
	COcclusionQuerySet m_OcclusionSet;
	CMaterialReference m_aboveWaterVolumeMaterial;
	CMaterialReference m_underWaterVolumeMaterial;
};

struct WaterAnalogInfo_t
{
	WaterAnalogInfo_t();

	bool bActive;
	Vector fogColor;
	float fogStart;
	float fogEnd;
	bool bFogEnabled;
	int iWaterIndex;
};

bool FindWaterAnalogInView( const Vector& position, VisibleFogVolumeInfo_t* fogVolumeInfo, struct WaterRenderInfo_t* waterRenderInfo, WaterAnalogInfo_t* waterAnalogInfo );
bool BoxIntersectWaterAnalogVolume( const Vector& mins, const Vector& maxs, int iWaterIndex );

#endif // C_FUNC_WATER_ANALOG_H