#pragma once
#include "../Misc/Vector.h"

class CBaseEntity;

struct BeamInfo_t
{
	int			m_nType;

	// Entities
	CBaseEntity* m_pStartEnt;
	int			m_nStartAttachment;
	CBaseEntity* m_pEndEnt;
	int			m_nEndAttachment;

	// Points
	Vector		m_vecStart;
	Vector		m_vecEnd;

	int			m_nModelIndex;
	const char* m_pszModelName;

	int			m_nHaloIndex;
	const char* m_pszHaloName;
	float		m_flHaloScale;

	float		m_flLife;
	float		m_flWidth;
	float		m_flEndWidth;
	float		m_flFadeLength;
	float		m_flAmplitude;

	float		m_flBrightness;
	float		m_flSpeed;

	int			m_nStartFrame;
	float		m_flFrameRate;

	float		m_flRed;
	float		m_flGreen;
	float		m_flBlue;

	bool		m_bRenderable;

	int			m_nSegments;

	int			m_nFlags;

	// Rings
	Vector		m_vecCenter;
	float		m_flStartRadius;
	float		m_flEndRadius;

	BeamInfo_t()
	{
		m_nType = 0;
		m_nSegments = -1;
		m_pszModelName = NULL;
		m_pszHaloName = NULL;
		m_nModelIndex = -1;
		m_nHaloIndex = -1;
		m_bRenderable = true;
		m_nFlags = 0;
		m_pStartEnt = nullptr;
		m_pEndEnt = nullptr;
	}
};

#define NOISE_DIVISIONS		128
#define MAX_BEAM_ENTS 10
class Beam_t {
public:
	char pad_0x0[0xC];
	Vector			m_Mins;
	Vector			m_Maxs;
	void*			m_queryHandleHalo;
	float			m_haloProxySize;
	Beam_t*			next;
	int				type;
	int				flags;
	int				numAttachment;
	Vector			attachemnt[MAX_BEAM_ENTS];
	Vector			delta;
	float			t;
	float			freq;
	float			die;
	float			width;
	float			endWidth;
	float			fadeLength;
	float			amplitude;
	float			life;
	float			r, g, b;
	float			brightness;
	float			speed;
	float			frameRate;
	float			frame;
	int				segments;
	unsigned int	entity[MAX_BEAM_ENTS];
	int				attachmentIndex[MAX_BEAM_ENTS];
	int				modelIndex;
	int				haloIndex;
	float			haloScale;
	int				frameCount;
	float			rgNoise[NOISE_DIVISIONS + 1];
	void*			trail;
	float			start_radius;
	float			end_radius;
	bool			m_bCalculatedNoise;
	float			m_flHDRColorScale;
};

enum
{
	TE_BEAMPOINTS = 0x00,		// beam effect between two points
	TE_SPRITE = 0x01,	// additive sprite, plays 1 cycle
	TE_BEAMDISK = 0x02,	// disk that expands to max radius over lifetime
	TE_BEAMCYLINDER = 0x03,		// cylinder that expands to max radius over lifetime
	TE_BEAMFOLLOW = 0x04,		// create a line of decaying beam segments until entity stops moving
	TE_BEAMRING = 0x05,		// connect a beam ring to two entities
	TE_BEAMSPLINE = 0x06,
	TE_BEAMRINGPOINT = 0x07,
	TE_BEAMLASER = 0x08,		// Fades according to viewpoint
	TE_BEAMTESLA = 0x09,
};
enum
{
	FBEAM_STARTENTITY = 0x00000001,
	FBEAM_ENDENTITY = 0x00000002,
	FBEAM_FADEIN = 0x00000004,
	FBEAM_FADEOUT = 0x00000008,
	FBEAM_SINENOISE = 0x00000010,
	FBEAM_SOLID = 0x00000020,
	FBEAM_SHADEIN = 0x00000040,
	FBEAM_SHADEOUT = 0x00000080,
	FBEAM_ONLYNOISEONCE = 0x00000100,		// Only calculate our noise once
	FBEAM_NOTILE = 0x00000200,
	FBEAM_USE_HITBOXES = 0x00000400,		// Attachment indices represent hitbox indices instead when this is set.
	FBEAM_STARTVISIBLE = 0x00000800,		// Has this client actually seen this beam's start entity yet?
	FBEAM_ENDVISIBLE = 0x00001000,		// Has this client actually seen this beam's end entity yet?
	FBEAM_ISACTIVE = 0x00002000,
	FBEAM_FOREVER = 0x00004000,
	FBEAM_HALOBEAM = 0x00008000,		// When drawing a beam with a halo, don't ignore the segments and endwidth
	FBEAM_REVERSED = 0x00010000,
	NUM_BEAM_FLAGS = 17	// KEEP THIS UPDATED!
};
//-----------------------------------------------------------------------------
// Purpose: Declare client .dll beam entity interface
//-----------------------------------------------------------------------------

class ITraceFilter;
class C_Beam;

class IViewRenderBeams {
public:
	// Construction
public:
	IViewRenderBeams(void);
	virtual				~IViewRenderBeams(void) = 0;

	// Implement IViewRenderBeams
public:
	virtual	void		InitBeams(void) = 0;
	virtual	void		ShutdownBeams(void) = 0;
	virtual	void		ClearBeams(void) = 0;

	// Updates the state of the temp ent beams
	virtual void		UpdateTemeBeams() = 0;

	virtual void		DrawBeam(Beam_t* pbeam) = 0;
	virtual void		DrawBeam(C_Beam* pbeam, ITraceFilter* pEntityBeamTraceFilter = NULL) = 0;

	virtual	void		KillDeadBeams(IClientEntity* pDeadEntity) = 0;

	virtual	void		CreateBeamEnts(int startEnt, int endEnt, int modelIndex, int haloIndex, float haloScale,
		float life, float width, float endWidth, float fadeLength, float amplitude,
		float brightness, float speed, int startFrame,
		float framerate, float r, float g, float b, int type = -1) = 0;
	virtual Beam_t* CreateBeamEnts(BeamInfo_t& beamInfo) = 0;

	virtual	void		CreateBeamEntPoint(int	nStartEntity, const Vector* pStart, int nEndEntity, const Vector* pEnd,
		int modelIndex, int haloIndex, float haloScale,
		float life, float width, float endWidth, float fadeLength, float amplitude,
		float brightness, float speed, int startFrame,
		float framerate, float r, float g, float b) = 0;
	virtual Beam_t* CreateBeamEntPoint(BeamInfo_t& beamInfo) = 0;

	virtual	void		CreateBeamPoints(Vector& start, Vector& end, int modelIndex, int haloIndex, float haloScale,
		float life, float width, float endWidth, float fadeLength, float amplitude,
		float brightness, float speed, int startFrame,
		float framerate, float r, float g, float b) = 0;
	virtual	Beam_t* CreateBeamPoints(BeamInfo_t& beamInfo) = 0;

	virtual	void		CreateBeamRing(int startEnt, int endEnt, int modelIndex, int haloIndex, float haloScale,
		float life, float width, float endWidth, float fadeLength, float amplitude,
		float brightness, float speed, int startFrame,
		float framerate, float r, float g, float b, int flags) = 0;
	virtual Beam_t* CreateBeamRing(BeamInfo_t& beamInfo) = 0;

	virtual void		CreateBeamRingPoint(const Vector& center, float start_radius, float end_radius, int modelIndex, int haloIndex, float haloScale,
		float life, float width, float m_nEndWidth, float m_nFadeLength, float amplitude,
		float brightness, float speed, int startFrame,
		float framerate, float r, float g, float b, int flags) = 0;
	virtual Beam_t* CreateBeamRingPoint(BeamInfo_t& beamInfo) = 0;

	virtual	void		CreateBeamCirclePoints(int type, Vector& start, Vector& end,
		int modelIndex, int haloIndex, float haloScale, float life, float width,
		float endWidth, float fadeLength, float amplitude, float brightness, float speed,
		int startFrame, float framerate, float r, float g, float b) = 0;
	virtual Beam_t* CreateBeamCirclePoints(BeamInfo_t& beamInfo) = 0;

	virtual	void		CreateBeamFollow(int startEnt, int modelIndex, int haloIndex, float haloScale,
		float life, float width, float endWidth, float fadeLength, float r, float g, float b,
		float brightness) = 0;
	virtual Beam_t* CreateBeamFollow(BeamInfo_t& beamInfo) = 0;

	virtual void		FreeBeam(Beam_t* pBeam) = 0;
};