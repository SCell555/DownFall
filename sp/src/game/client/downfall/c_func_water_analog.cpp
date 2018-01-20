#include "cbase.h"
#include "c_func_water_analog.h"
#include "view_shared.h"
#include "materialsystem/imaterialvar.h"
#include "model_types.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "viewrender.h"

#include "tier0/memdbgon.h"

static CUtlVector<CHandle<C_FuncWaterAnalog>> s_waters;

LINK_ENTITY_TO_CLASS( func_water_analog, C_FuncWaterAnalog );

IMPLEMENT_NETWORKCLASS_ALIASED( FuncWaterAnalog, DT_WaterAnalog );
BEGIN_RECV_TABLE( C_FuncWaterAnalog, DT_WaterAnalog )
END_RECV_TABLE()

static class CBrushRenderer : public IBrushRenderer
{
public:
	bool RenderBrushModelSurface( IClientEntity* pBaseEntity, IBrushSurface* pBrushSurface ) OVERRIDE
	{
		const int vertices = pBrushSurface->GetVertexCount();
		BrushVertex_t* pVertices = static_cast<BrushVertex_t*>( malloc( vertices * sizeof(BrushVertex_t) ) );
		pBrushSurface->GetVertexData( pVertices );
		CMatRenderContextPtr pRenderContext( materials );
		CMeshBuilder builder;
		builder.Begin( pRenderContext->GetDynamicMesh( true, 0, 0, sm_OcclusionProxyMaterial ), MATERIAL_QUADS, vertices );
		for ( int i = 0; i < vertices; ++i )
		{
			BrushVertex_t& vertex = pVertices[i];
			builder.Position3fv( vertex.m_Pos.Base() );
			builder.Normal3fv( vertex.m_Normal.Base() );
			builder.TangentS3fv( vertex.m_TangentS.Base() );
			builder.TangentT3fv( vertex.m_TangentT.Base() );
			builder.TexCoord2fv( 0, vertex.m_TexCoord.Base() );
			builder.TexCoord2fv( 1, vertex.m_LightmapCoord.Base() );
			builder.AdvanceVertex();
		}
		
		builder.End( false, true );

		free( pVertices );
		return false;
	}

	static void InitMaterial()
	{
		if ( !sm_OcclusionProxyMaterial.IsValid() )
			sm_OcclusionProxyMaterial.Init( "engine/occlusionproxy", TEXTURE_GROUP_CLIENT_EFFECTS );
	}
private:
	static CMaterialReference sm_OcclusionProxyMaterial;
} brushOcclusionRenderer;
CMaterialReference CBrushRenderer::sm_OcclusionProxyMaterial;

C_FuncWaterAnalog::C_FuncWaterAnalog()
{
}


C_FuncWaterAnalog::~C_FuncWaterAnalog()
{
}

void C_FuncWaterAnalog::Spawn()
{
	CBrushRenderer::InitMaterial();
	BaseClass::Spawn();
	s_waters.AddToTail( this );
}

void C_FuncWaterAnalog::UpdateOnRemove()
{
	const int count = s_waters.Count();
	for ( int i = 0; i < count; ++i )
	{
		if ( s_waters[i] == this )
		{
			s_waters.Remove( i );
			break;
		}
	}
	BaseClass::UpdateOnRemove();
}

int C_FuncWaterAnalog::DrawBrushModel( bool bTranslucent, int nFlags, bool bTwoPass )
{
	if ( ( nFlags & STUDIO_RENDER ) != 0 && ( nFlags & ( STUDIO_SHADOWDEPTHTEXTURE | STUDIO_SHADOWDEPTHTEXTURE ) ) == 0 )
	{
		render->InstallBrushSurfaceRenderer( &brushOcclusionRenderer );

		m_OcclusionSet.BeginQueryDrawing();
		BaseClass::DrawBrushModel( false, STUDIO_RENDER, false );
		m_OcclusionSet.EndQueryDrawing();

		render->InstallBrushSurfaceRenderer( NULL );
	}

	return BaseClass::DrawBrushModel( bTranslucent, nFlags, bTwoPass );
}

void C_FuncWaterAnalog::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		const int count = modelinfo->GetModelMaterialCount( GetModel() );
		Assert( count == 2 );
		IMaterial** materials = ( IMaterial** )malloc( count * sizeof( IMaterial* ) );
		modelinfo->GetModelMaterials( GetModel(), count, materials );
		for ( int i = 0; i < count; ++i )
		{
			/*if ( !V_stristr( materials[i]->GetName(), "beneath" ) )
			{
				KeyValuesAD patch( "patch" );
				patch->LoadFromFile( g_pFullFileSystem, CFmtStr( "materials/%s.vmt", materials[i]->GetName() ), NULL );
				m_aboveWaterVolumeMaterial.Init( CFmtStr( "%s.vmt", patch->GetString( "include" ) + 10 ), (const char*)NULL );
				m_underWaterVolumeMaterial.Init( CFmtStr( "materials/%s.vmt", m_aboveWaterVolumeMaterial->FindVar( "bottommaterial", NULL )->GetStringValue() ), ( const char* )NULL );

				break;
			}*/
			if ( V_stristr( materials[i]->GetName(), "beneath" ) )
				m_underWaterVolumeMaterial.Init( materials[i] );
			else
				m_aboveWaterVolumeMaterial.Init( materials[i] );
		}
		free( materials );
	}
}

WaterAnalogInfo_t::WaterAnalogInfo_t()
{
	bActive = false;
	fogColor.Init( 0.f );
	fogStart = 0.f;
	fogEnd = 0.f;
	bFogEnabled = false;
	iWaterIndex = -1;
}

ConVar func_water_analog_cheap( "func_water_analog_cheap", "1" );
bool FindWaterAnalogInView( const Vector& position, VisibleFogVolumeInfo_t* fogVolumeInfo, WaterRenderInfo_t* waterRenderInfo, WaterAnalogInfo_t* waterAnalogInfo )
{
	if ( !s_waters.Count() )
		return false;

	// Only search if no water volume is in view
	//if ( fogVolumeInfo->m_nVisibleFogVolume != -1 )
	//	return false;

	const int count = s_waters.Count();
	for ( int i = 0; i < count; ++i )
	{
		C_FuncWaterAnalog* pWater = s_waters[i];
		const CCollisionProperty* pCollision = pWater->CollisionProp();
		const Vector& origin = pWater->GetAbsOrigin();
		const Vector& mins = pCollision->OBBMins() + origin;
		const Vector& maxs = pCollision->OBBMaxs() + origin;

		const bool bInWater = position.WithinAABox( mins, maxs );		// Are we in water
		const bool bSeeWater = !bInWater && pWater->IsSurfaceVisible();	// Can we see water surface?

		if ( bInWater || bSeeWater )
		{
			IMaterial* pWaterMaterial = pWater->GetWaterVolumeMaterial( bInWater );
			if ( pWaterMaterial && ( fogVolumeInfo->m_nVisibleFogVolume == -1 || bInWater ) )
			{
				fogVolumeInfo->m_nVisibleFogVolume = -1;
				fogVolumeInfo->m_pFogVolumeMaterial = pWaterMaterial;
				IMaterialVar* pFogColorVar = fogVolumeInfo->m_pFogVolumeMaterial->FindVar( "$fogcolor", NULL );
				IMaterialVar* pFogEnableVar = fogVolumeInfo->m_pFogVolumeMaterial->FindVar( "$fogenable", NULL );
				IMaterialVar* pFogStartVar = fogVolumeInfo->m_pFogVolumeMaterial->FindVar( "$fogstart", NULL );
				IMaterialVar* pFogEndVar = fogVolumeInfo->m_pFogVolumeMaterial->FindVar( "$fogend", NULL );
				waterAnalogInfo->bActive = true;
				fogVolumeInfo->m_bEyeInFogVolume = bInWater;
				fogVolumeInfo->m_flWaterHeight = maxs.z + 2;
				pFogColorVar->GetVecValueFast( waterAnalogInfo->fogColor.Base(), 3 );
				waterAnalogInfo->fogStart = pFogStartVar->GetFloatValueFast();
				waterAnalogInfo->fogEnd = pFogEndVar->GetFloatValueFast();
				waterAnalogInfo->bFogEnabled = pFogEnableVar->GetIntValueFast();
				waterAnalogInfo->iWaterIndex = i;
				waterRenderInfo->m_bCheapWater = func_water_analog_cheap.GetBool();
				waterRenderInfo->m_bDrawWaterSurface = true;
				waterRenderInfo->m_bOpaqueWater = false;// !fogVolumeInfo->m_pFogVolumeMaterial->IsTranslucent();
				waterRenderInfo->m_bReflect = true;
				waterRenderInfo->m_bReflectEntities = true;
				waterRenderInfo->m_bRefract = true;
				return true;
			}
		}
	}
	return false;
}

bool BoxIntersectWaterAnalogVolume( const Vector& mins, const Vector& maxs, int iWaterIndex )
{
	C_FuncWaterAnalog* pWater = s_waters[iWaterIndex];
	const CCollisionProperty* pCollision = pWater->CollisionProp();
	const Vector& origin = pWater->GetAbsOrigin();
	const Vector& waterMins = pCollision->OBBMins() + origin;
	const Vector& waterMaxs = pCollision->OBBMaxs() + origin;
	
	return QuickBoxIntersectTest( mins, maxs, waterMins, waterMaxs );
}