#pragma once
#include "../SDK/Definitions/Types.h"
#include <windows.h>
#include <unordered_map>
#include <typeinfo>

#define VA_LIST(...) __VA_ARGS__

#define DEFAULT_BIND -1

// forward declartion of ConfigVar
template <class T>
class ConfigVar;

class CVarBase
{
public:
	size_t m_iType;
	std::string m_sName;
	std::vector<const char*> m_vEntries = {};
	int m_iFlags;

	// getter for ConfigVar
	template <class T>
	inline ConfigVar<T>* As()
	{
		if (typeid(T).hash_code() != m_iType)
			return nullptr;

		return reinterpret_cast<ConfigVar<T>*>(this);
	}
};

namespace G
{
	inline std::vector<CVarBase*> Vars = {};
};

template <class T>
class ConfigVar : public CVarBase
{
public:
	T Default;
	T Value;
	std::unordered_map<int, T> Map;
	ConfigVar(T value, std::string name, int iFlags = 0);
	ConfigVar(T value, std::string name, std::vector<const char*> Entries, int iFlags = 0);

	inline T& operator[](int i)
	{
		return Map[i];
	}
	inline bool contains(int i) const
	{
		return Map.contains(i);
	}
};

inline std::vector<CVarBase*> g_Vars;

template<class T>
inline ConfigVar<T>::ConfigVar(T value, std::string name, int iFlags)
{
	Default = value;
	Value = value;
	Map[DEFAULT_BIND] = value;
	m_iType = typeid(T).hash_code();
	m_sName = name;
	m_iFlags = iFlags;
	g_Vars.push_back(this);
}

template<class T>
inline ConfigVar<T>::ConfigVar( T value, std::string name, std::vector<const char*> Entries, int iFlags )
{
	Default = value;
	Value = value;
	Map[ DEFAULT_BIND ] = value;
	m_iType = typeid( T ).hash_code( );
	m_sName = name;
	g_Vars.push_back( this );
	m_iFlags = iFlags;
	m_vEntries = Entries;
}

#define NAMESPACE_BEGIN(name)\
	namespace name {\
		inline std::string GetNamespace() { return "Vars::" + std::string(#name) + "::"; }\
		inline std::string GetSubname() { return ""; }

#define SUBNAMESPACE_BEGIN(name)\
	namespace name {\
		inline std::string GetSubname() { return std::string(#name) + "::"; }

#define NAMESPACE_END(name)\
	}
#define SUBNAMESPACE_END(name)\
	}

#define CVar(name, value, ...)\
	inline ConfigVar<decltype(value)> name = { value, GetNamespace() + GetSubname() + std::string(#name), __VA_ARGS__ };
#define Enum(name, ...)\
	namespace name##Enum { enum name##Enum { __VA_ARGS__ }; };
#define CVarEnum(name, value, flags, entries, ...)\
	CVar(name, value, entries, flags)\
	Enum(name, __VA_ARGS__)

#define NONE 0
#define VISUAL (1 << 0)
#define NOSAVE (1 << 1)
#define NOBIND (1 << 2)
#define MULTI (1 << 3)
#define DEBUGVAR NONE

namespace Vars
{
	NAMESPACE_BEGIN(Menu)
		CVar(CheatName, std::string("Amalgam"), VISUAL)
		CVar(CheatPrefix, std::string("[Amalgam]"), VISUAL)
		CVar(MenuPrimaryKey, VK_INSERT, NOBIND)
		CVar(MenuSecondaryKey, VK_F3, NOBIND)

		CVar(BindWindowTitle, true)
		CVar(BindsDisplay, DragBox_t(100, 100), NOBIND)
		CVar(MenuShowsBinds, false, NOBIND)

		CVarEnum( Indicators, 0b00000, MULTI, VA_LIST( { "Ticks", "Crit hack", "Spectators", "Ping", "Conditions", "Seed prediction" } ), Ticks = 1 << 0, CritHack = 1 << 1, Spectators = 1 << 2, Ping = 1 << 3, Conditions = 1 << 4, SeedPrediction = 1 << 5 )
		//CVar(SpectatorAvatars, false, VISUAL)

		CVar(TicksDisplay, DragBox_t(), NOBIND)
		CVar(CritsDisplay, DragBox_t(), NOBIND)
		CVar(SpectatorsDisplay, DragBox_t(), NOBIND)
		CVar(PingDisplay, DragBox_t(), NOBIND)
		CVar(ConditionsDisplay, DragBox_t(), NOBIND)
		CVar(SeedPredictionDisplay, DragBox_t(), NOBIND)

		CVar(Scale, 1.f, NOBIND)
		CVar(CheapText, false)

		SUBNAMESPACE_BEGIN(Theme)
			CVar(Accent, Color_t(175, 150, 255, 255), VISUAL)
			CVar(Background, Color_t(0, 0, 0, 250), VISUAL)
			CVar(Inactive, Color_t(150, 150, 150, 255), VISUAL)
			CVar(Active, Color_t(255, 255, 255, 255), VISUAL)
		SUBNAMESPACE_END(Theme)
	NAMESPACE_END(Menu)

	NAMESPACE_BEGIN(Colors)
		CVar(FOVCircle, Color_t(255, 255, 255, 100), VISUAL)
		CVar(Relative, false, VISUAL)
		CVar( TeamRed, Color_t( 225, 60, 60, 255 ), VISUAL )
		CVar( TeamRedName, Color_t( 225, 60, 60, 255 ), VISUAL )
		CVar( TeamBlu, Color_t( 75, 175, 225, 255 ), VISUAL )
		CVar( TeamBluName, Color_t( 75, 175, 225, 255 ), VISUAL )
		CVar( Enemy, Color_t( 225, 60, 60, 255 ), VISUAL )
		CVar( EnemyName, Color_t( 225, 60, 60, 255 ), VISUAL )
		CVar( Team, Color_t( 75, 175, 225, 255 ), VISUAL )
		CVar( TeamName, Color_t( 75, 175, 225, 255 ), VISUAL )
		CVar( Local, Color_t( 255, 255, 255, 255 ), VISUAL )
		CVar( LocalName, Color_t( 255, 255, 255, 255 ), VISUAL )
		CVar( Target, Color_t( 255, 0, 0, 255 ), VISUAL )
		CVar( TargetName, Color_t( 255, 0, 0, 255 ), VISUAL )
		CVar( Friend, Color_t( 255, 0, 0, 255 ), VISUAL )
		CVar( FriendName, Color_t( 255, 0, 0, 255 ), VISUAL )
		CVar(Health, Color_t(0, 225, 75, 255), VISUAL)
		CVar(Ammo, Color_t(127, 127, 127, 255), VISUAL)
		CVar(Money, Color_t(0, 150, 75, 255), VISUAL)
		CVar(Powerup, Color_t(255, 175, 0, 255), VISUAL)
		CVar(NPC, Color_t(255, 255, 255, 255), VISUAL)
		CVar(Halloween, Color_t(100, 0, 255, 255), VISUAL)
		CVar(Backtrack, Color_t(255, 0, 0, 0), VISUAL)
		CVar(FakeAngle, Color_t(255, 255, 255, 0), VISUAL)

		CVar(IndicatorGood, Color_t(0, 255, 100, 255), DEBUGVAR)
		CVar(IndicatorMid, Color_t(255, 200, 0, 255), DEBUGVAR)
		CVar(IndicatorBad, Color_t(255, 0, 0, 255), DEBUGVAR)
		CVar(IndicatorMisc, Color_t(75, 175, 255, 255), DEBUGVAR)
		CVar(IndicatorTextGood, Color_t(150, 255, 150, 255), DEBUGVAR)
		CVar(IndicatorTextMid, Color_t(255, 200, 0, 255), DEBUGVAR)
		CVar(IndicatorTextBad, Color_t(255, 150, 150, 255), DEBUGVAR)
		CVar(IndicatorTextMisc, Color_t(100, 255, 255, 255), DEBUGVAR)

		CVar(WorldModulation, Color_t(255, 255, 255, 255), VISUAL)
		CVar(SkyModulation, Color_t(255, 255, 255, 255), VISUAL)
		CVar(PropModulation, Color_t(255, 255, 255, 255), VISUAL)
		CVar(ParticleModulation, Color_t(255, 255, 255, 255), VISUAL)
		CVar(FogModulation, Color_t(255, 255, 255, 255), VISUAL)

		CVar(Line, Color_t(255, 255, 255, 0), VISUAL)
		CVar(LineClipped, Color_t(255, 255, 255, 255), VISUAL)

		CVar(PlayerPath, Color_t(255, 255, 255, 255), VISUAL)
		CVar(PlayerPathClipped, Color_t(255, 255, 255, 0), VISUAL)
		CVar(ProjectilePath, Color_t(255, 255, 255, 255), VISUAL)
		CVar(ProjectilePathClipped, Color_t(255, 255, 255, 0), VISUAL)
		CVar(TrajectoryPath, Color_t(255, 255, 255, 255), VISUAL)
		CVar(TrajectoryPathClipped, Color_t(255, 255, 255, 0), VISUAL)
		CVar(ShotPath, Color_t(255, 255, 255, 255), VISUAL)
		CVar(ShotPathClipped, Color_t(255, 255, 255, 0), VISUAL)
		CVar(SplashRadius, Color_t(255, 255, 255, 255), VISUAL)
		CVar(SplashRadiusClipped, Color_t(255, 255, 255, 0), VISUAL)

		CVar(BoneHitboxEdge, Color_t(255, 255, 255, 0), VISUAL)
		CVar(BoneHitboxEdgeClipped, Color_t(255, 255, 255, 255), VISUAL)
		CVar(BoneHitboxFace, Color_t(255, 255, 255, 0), VISUAL)
		CVar(BoneHitboxFaceClipped, Color_t(255, 255, 255, 0), VISUAL)
		CVar(TargetHitboxEdge, Color_t(255, 150, 150, 0), VISUAL)
		CVar(TargetHitboxEdgeClipped, Color_t(255, 150, 150, 255), VISUAL)
		CVar(TargetHitboxFace, Color_t(255, 150, 150, 0), VISUAL)
		CVar(TargetHitboxFaceClipped, Color_t(255, 150, 150, 0), VISUAL)
		CVar(BoundHitboxEdge, Color_t(255, 255, 255, 0), VISUAL)
		CVar(BoundHitboxEdgeClipped, Color_t(255, 255, 255, 255), VISUAL)
		CVar(BoundHitboxFace, Color_t(255, 255, 255, 0), VISUAL)
		CVar(BoundHitboxFaceClipped, Color_t(255, 255, 255, 0), VISUAL)

		CVar(SpellFootstep, Color_t(255, 255, 255, 255), VISUAL)
	NAMESPACE_END(Colors)

	NAMESPACE_BEGIN(Aimbot)
		SUBNAMESPACE_BEGIN(General)
			CVarEnum(AimType, 0, NONE, VA_LIST( { "Off", "Plain", "Smooth", "Silent", "Locking", "Assistive" } ), Off, Plain, Smooth, Silent, Locking, Assistive)
			CVarEnum(TargetSelection, 0, NONE, VA_LIST( { "FOV", "Distance" } ), FOV, Distance)
			CVarEnum(Target, 0b0000001, MULTI, VA_LIST( { "Players", "Sentries", "Dispensers", "Teleporters", "Stickies", "NPCs", "Bombs" } ), Players = 1 << 0, Sentry = 1 << 1, Dispenser = 1 << 2, Teleporter = 1 << 3, Stickies = 1 << 4, NPCs = 1 << 5, Bombs = 1 << 6)
			CVarEnum(Ignore, 0b000000000, MULTI, VA_LIST( { "Friends", "Party", "Invulnerable", "Cloaked", "Unsimulated players", "Dead Ringer", "Vaccinator", "Disguised", "Taunting" } ), Friends = 1 << 0, Party = 1 << 1, Invulnerable = 1 << 2, Cloaked = 1 << 3, Unsimulated = 1 << 4, DeadRinger = 1 << 5, Vaccinator = 1 << 6, Disguised = 1 << 7, Taunting = 1 << 8)
			CVar(AimFOV, 30.f)
			CVar(MaxTargets, 2)
			CVar(AssistStrength, 25.f)
			CVar(IgnoreCloakPercentage, 100)
			CVar(TickTolerance, 7)
			CVar(AutoShoot, true)
			CVar(FOVCircle, true)
			CVar(NoSpread, false)

			CVar(HitscanPeek, 1, DEBUGVAR)
			CVar(PeekDTOnly, true, DEBUGVAR)
			CVar(NoSpreadOffset, 0.f, DEBUGVAR)
			CVar(NoSpreadAverage, 5, DEBUGVAR)
			CVar(NoSpreadInterval, 0.1f, DEBUGVAR)
			CVar(NoSpreadBackupInterval, 2.f, DEBUGVAR)
			CVarEnum(AimHoldsFire, 2, DEBUGVAR, VA_LIST( { "False", "Minigun only", "Always" } ), False, MinigunOnly, Always)
		SUBNAMESPACE_END(Global)

		SUBNAMESPACE_BEGIN(Hitscan)
			CVarEnum(Hitboxes, 0b000111, MULTI, VA_LIST( { "Head", "Body", "Pelvis", "Arms", "Legs", "Bodyaim if lethal" } ), Head = 1 << 0, Body = 1 << 1, Pelvis = 1 << 2, Arms = 1 << 3, Legs = 1 << 4, BodyaimIfLethal = 1 << 5)
			CVarEnum(Modifiers, 0b0100000, MULTI, VA_LIST( { "Tapfire", "Wait for headshot", "Wait for charge", "Scoped only", "Auto scope", "Auto rev minigun", "Extinguish team" } ), Tapfire = 1 << 0, WaitForHeadshot = 1 << 1, WaitForCharge = 1 << 2, ScopedOnly = 1 << 3, AutoScope = 1 << 4, AutoRev = 1 << 5, ExtinguishTeam = 1 << 6)
			CVar(PointScale, 0.f)
			CVar(TapFireDist, 1000.f)

			CVar(BoneSizeSubtract, 1.f, DEBUGVAR)
			CVar(BoneSizeMinimumScale, 1.f, DEBUGVAR)
		SUBNAMESPACE_END(HITSCAN)

		SUBNAMESPACE_BEGIN(Projectile)
			CVarEnum(StrafePrediction, 0b11, MULTI, VA_LIST( { "Air", "Ground" } ), Air = 1 << 0, Ground = 1 << 1)
			CVarEnum(SplashPrediction, 0, NONE, VA_LIST( { "Off", "Include", "Prefer", "Only" } ), Off, Include, Prefer, Only)
			CVarEnum(AutoDetonate, 0b00, MULTI, VA_LIST( { "Stickies", "Flares", "Prevent self damage", "Ignore cloak" } ), Stickies = 1 << 0, Flares = 1 << 1, PreventSelfDamage = 1 << 2, IgnoreCloak = 1 << 3)
			CVarEnum(AutoAirblast, 0b1000, MULTI, VA_LIST( { "Enabled", "Redirect simple", "Redirect advanced", "Respect FOV" } ), Enabled = 1 << 0, RedirectSimple = 1 << 1, RedirectAdvanced = 1 << 2, RespectFOV = 1 << 3)
			CVarEnum( Hitboxes, 0b001111, MULTI, VA_LIST( { "Auto", "##Divider", "Head", "Body", "Feet", "##Divider", "Bodyaim if lethal", "Aim blast at feet" } ), Auto = 1 << 0, Head = 1 << 1, Body = 1 << 2, Feet = 1 << 3, BodyaimIfLethal = 1 << 4, AimBlastAtFeet = 1 << 5 )
			CVarEnum(Modifiers, 0b1010, MULTI, VA_LIST( { "Charge shot", "Cancel charge", "Use prime time" } ), ChargeWeapon = 1 << 0, CancelCharge = 1 << 1, UsePrimeTime = 1 << 2)
			CVar(PredictionTime, 2.f)
			CVar(Hitchance, 0.f)
			CVar(AutodetRadius, 90.f)
			CVar(SplashRadius, 90.f)
			CVar(AutoRelease, 0.f)

			CVar(GroundSamples, 33, DEBUGVAR)
			CVar(GroundStraightFuzzyValue, 100.f, DEBUGVAR)
			CVar(GroundLowMinimumSamples, 16, DEBUGVAR)

			CVar(GroundHighMinimumSamples, 33, DEBUGVAR)
			CVar(GroundLowMinimumDistance, 0.f, DEBUGVAR)
			CVar(GroundHighMinimumDistance, 1000.f, DEBUGVAR)
			CVar(GroundMaxChanges, 0, DEBUGVAR)
			CVar(GroundMaxChangeTime, 0, DEBUGVAR)

			CVar(AirSamples, 33, DEBUGVAR)
			CVar(AirStraightFuzzyValue, 0.f, DEBUGVAR)
			CVar(AirLowMinimumSamples, 16, DEBUGVAR)
			CVar(AirHighMinimumSamples, 16, DEBUGVAR)
			CVar(AirLowMinimumDistance, 100000.f, DEBUGVAR)
			CVar(AirHighMinimumDistance, 100000.f, DEBUGVAR)
			CVar(AirMaxChanges, 2, DEBUGVAR)
			CVar(AirMaxChangeTime, 16, DEBUGVAR)

			CVar(VelocityAverageCount, 5, DEBUGVAR)
			CVar(VerticalShift, 5.f, DEBUGVAR)
			CVar(DragOverride, 0.f, DEBUGVAR)
			CVar(TimeOverride, 0.f, DEBUGVAR)
			CVar(HuntsmanLerp, 50.f, DEBUGVAR)
			CVar(HuntsmanLerpLow, 100.f, DEBUGVAR)
			CVar(HuntsmanAdd, 0.f, DEBUGVAR)
			CVar(HuntsmanAddLow, 0.f, DEBUGVAR)
			CVar(HuntsmanClamp, 5.f, DEBUGVAR)

			CVar(HuntsmanPullPoint, false, DEBUGVAR)
			CVar(SplashPoints, 100, DEBUGVAR)
			CVar(SplashGrates, true, DEBUGVAR)
			CVar(SplashRotateX, 0.f, DEBUGVAR)
			CVar(SplashRotateY, 0.f, DEBUGVAR)
			CVar(SplashNthRoot, 1.f, DEBUGVAR)
			CVar(SplashCountDirect, 100, DEBUGVAR)
			CVar(SplashCountArc, 100, DEBUGVAR)
			CVar(SplashTraceInterval, 10, DEBUGVAR)
			CVarEnum(SplashMode, 0, DEBUGVAR, VA_LIST( { "Multi", "Single" } ), Multi, Single)
			CVarEnum(RocketSplashMode, 0, DEBUGVAR, VA_LIST( { "Regular", "Special light", "Special heavy" } ), Regular, SpecialLight, SpecialHeavy)
			CVar(DeltaCount, 5, DEBUGVAR)
			CVarEnum(DeltaMode, 0, DEBUGVAR, VA_LIST( { "Average", "Max" } ), Average, Max)
			CVarEnum(MovesimFrictionFlags, 0b01, DEBUGVAR | MULTI, VA_LIST( { "Run Reduce", "Calculate Increase" } ), RunReduce = 1 << 0, CalculateIncrease = 1 << 1)
		SUBNAMESPACE_END(Projectile)

		SUBNAMESPACE_BEGIN(Melee)
			CVar(AutoBackstab, true)
			CVar(IgnoreRazorback, true)
			CVar(SwingPrediction, false)
			CVar(WhipTeam, false)

			CVar(SwingTicks, 13, DEBUGVAR)
			CVar(SwingPredictLag, true, DEBUGVAR)
			CVar(BackstabAccountPing, true, DEBUGVAR)
			CVar(BackstabDoubleTest, true, DEBUGVAR)
		SUBNAMESPACE_END(Melee)

		SUBNAMESPACE_BEGIN(Healing)
			CVar(AutoHeal, false)
			CVar(FriendsOnly, false)
			CVar(ActivateOnVoice, false)
		SUBNAMESPACE_END(Healing)
	NAMESPACE_END(AIMBOT)
	
	NAMESPACE_BEGIN(CritHack)
		CVar(ForceCrits, false)
		CVar(AvoidRandom, false)
		CVar(AlwaysMeleeCrit, false)
	NAMESPACE_END(CritHack)

	NAMESPACE_BEGIN(Backtrack)
		CVar(Enabled, false)
		CVar(PreferOnShot, false)
		CVar(Latency, 0)
		CVar(Interp, 0)
		CVar(Window, 185)

		CVar(Offset, 0, DEBUGVAR)
	NAMESPACE_END(Backtrack)

	NAMESPACE_BEGIN(CL_Move)
		SUBNAMESPACE_BEGIN(Doubletap)
			CVar(Doubletap, false)
			CVar(Warp, false)
			CVar(RechargeTicks, false)
			CVar(AntiWarp, true)
			CVar(TickLimit, 22)
			CVar(WarpRate, 22)
			CVar(PassiveRecharge, 0)
			CVar(RechargeLimit, 24)
		SUBNAMESPACE_END(DoubleTap)

		SUBNAMESPACE_BEGIN(Fakelag)
			CVarEnum(Fakelag, 0, NONE, VA_LIST( { "Off", "Plain", "Random", "Adaptive" } ), Off, Plain, Random, Adaptive)
			CVar(PlainTicks, 12)
			CVar(RandomTicks, IntRange_t(14, 18));
			CVarEnum(Options, 0b000, MULTI, VA_LIST( { "Only moving", "On unduck", "Not airborne" } ), WhileMoving = 1 << 0, WhileUnducking = 1 << 1, WhileAirborne = 1 << 2)
			CVar(UnchokeOnAttack, true)
			CVar(RetainBlastJump, false)

			CVar(RetainSoldierOnly, true, DEBUGVAR)
		SUBNAMESPACE_END(FakeLag)

		CVar(AutoPeek, false)

		CVar(SpeedEnabled, false)
		CVar(SpeedFactor, 1)
	NAMESPACE_END(CL_Move)

	NAMESPACE_BEGIN(AntiHack)
		SUBNAMESPACE_BEGIN(AntiAim)
			CVar(Enabled, false)
			CVarEnum(PitchReal, 0, NONE, VA_LIST( { "None", "Up", "Down", "Zero", "Jitter", "Reverse jitter" } ), None, Up, Down, Zero, Jitter, ReverseJitter)
			CVarEnum(PitchFake, 0, NONE, VA_LIST( { "None", "Up", "Down", "Jitter", "Reverse jitter" } ), None, Up, Down, Jitter, ReverseJitter)
			Enum(Yaw, Forward, Left, Right, Backwards, Edge, Jitter, Spin)
			CVar(YawReal, 0)
			CVar(YawFake, 0)
			Enum(YawMode, View, Target)
			CVar(RealYawMode, 0)
			CVar(FakeYawMode, 0)
			CVar(RealYawOffset, 0)
			CVar(FakeYawOffset, 0)
			CVar(RealYawValue, 0)
			CVar(FakeYawValue, 0)
			CVar(SpinSpeed, 15.f)
			CVar(MinWalk, true)
			CVar(AntiOverlap, false)
			CVar(InvalidShootPitch, false)
		SUBNAMESPACE_END(AntiAim)

		SUBNAMESPACE_BEGIN(Resolver)
			CVar(Enabled, false)
			CVar(AutoResolve, false)
			CVar(AutoResolveCheatersOnly, false)
			CVar(AutoResolveHeadshotOnly, false)
			CVar(AutoResolveYawAmount, 90.f)
			CVar(AutoResolvePitchAmount, 90.f)
			CVar(CycleYaw, 0.f)
			CVar(CyclePitch, 0.f)
			CVar(CycleView, false)
			CVar(CycleMinwalk, false)
		SUBNAMESPACE_END(Resolver)
	NAMESPACE_END(AntiHack)

	NAMESPACE_BEGIN(CheaterDetection)
		CVarEnum(Methods, 0b0001, MULTI, VA_LIST( { "Invalid pitch", "Packet choking", "Aim flicking", "Duck Speed" } ), InvalidPitch = 1 << 0, PacketChoking = 1 << 1, AimFlicking = 1 << 2, DuckSpeed = 1 << 3)
		CVar(DetectionsRequired, 10)
		CVar(MinimumChoking, 20)
		CVar(MinimumFlick, 20.f) // min flick size to suspect
		CVar(MaximumNoise, 1.f) // max different between angles before and after flick
	NAMESPACE_END(CheaterDetection)

	NAMESPACE_BEGIN(ESP)
		CVarEnum(Draw, 0b0, VISUAL | MULTI, VA_LIST( { "Players", "Buildings", "Projectiles", "Objective", "NPCs", "Health", "Ammo", "Money", "Powerups", "Bombs", "Spellbook", "Gargoyle" } ), Players = 1 << 0, Buildings = 1 << 1, Projectiles = 1 << 2, Objective = 1 << 3, NPCs = 1 << 4, Health = 1 << 5, Ammo = 1 << 6, Money = 1 << 7, Powerups = 1 << 8, Bombs = 1 << 9, Spellbook = 1 << 10, Gargoyle = 1 << 11)
		CVarEnum(Player, 0b0, VISUAL | MULTI, VA_LIST( { "Enemy", "Team", "Local", "Prioritized", "Friends", "Party", "##Divider", "Name", "Box", "Distance", "Bones", "Health bar", "Health text", "Uber bar", "Uber text", "Class icon", "Class text", "Weapon icon", "Weapon text", "Priority", "Labels", "Buffs", "Debuffs", "Misc", "Lag compensation", "Ping", "KDR" } ), Enemy = 1 << 0, Team = 1 << 1, Local = 1 << 2, Prioritized = 1 << 3, Friends = 1 << 4, Party = 1 << 5, Name = 1 << 6, Box = 1 << 7, Distance = 1 << 8, Bones = 1 << 9, HealthBar = 1 << 10, HealthText = 1 << 11, UberBar = 1 << 12, UberText = 1 << 13, ClassIcon = 1 << 14, ClassText = 1 << 15, WeaponIcon = 1 << 16, WeaponText = 1 << 17, Priority = 1 << 18, Labels = 1 << 19, Buffs = 1 << 20, Debuffs = 1 << 21, Misc = 1 << 22, LagCompensation = 1 << 23, Ping = 1 << 24, KDR = 1 << 25)
		CVarEnum(Building, 0b0, VISUAL | MULTI, VA_LIST( { "Enemy", "Team", "Local", "Prioritized", "Friends", "Party", "##Divider", "Name", "Box", "Distance", "Health bar", "Health text", "Owner", "Level", "Flags" } ), Enemy = 1 << 0, Team = 1 << 1, Local = 1 << 2, Prioritized = 1 << 3, Friends = 1 << 4, Party = 1 << 5, Name = 1 << 6, Box = 1 << 7, Distance = 1 << 8, HealthBar = 1 << 9, HealthText = 1 << 10, Owner = 1 << 11, Level = 1 << 12, Flags = 1 << 13)
		CVarEnum(Projectile, 0b0, VISUAL | MULTI, VA_LIST( { "Enemy", "Team", "Local", "Prioritized", "Friends", "Party", "##Divider", "Name", "Box", "Distance", "Owner", "Flags" } ), Enemy = 1 << 0, Team = 1 << 1, Local = 1 << 2, Prioritized = 1 << 3, Friends = 1 << 4, Party = 1 << 5, Name = 1 << 6, Box = 1 << 7, Distance = 1 << 8, Owner = 1 << 9, Flags = 1 << 10)
		CVarEnum(Objective, 0b0, VISUAL | MULTI, VA_LIST( { "Enemy", "Team", "##Divider", "Name", "Box", "Distance", "Flags", "Intel return time" } ), Enemy = 1 << 0, Team = 1 << 1, Name = 1 << 2, Box = 1 << 3, Distance = 1 << 4, Flags = 1 << 5, IntelReturnTime = 1 << 6)
		CVar(ActiveAlpha, 255, VISUAL)
		CVar(DormantAlpha, 50, VISUAL)
		CVar(DormantPriority, false, VISUAL)
		CVar(DormantTime, 1.f, VISUAL)
	NAMESPACE_END(ESP)

	NAMESPACE_BEGIN(Chams)
		SUBNAMESPACE_BEGIN(Player)
			CVar(Local, false, VISUAL)
			CVar(Priority, false, VISUAL)
			CVar(Friend, false, VISUAL)
			CVar(Party, false, VISUAL)
			CVar(Target, false, VISUAL)
			CVar(Relative, true, VISUAL) // friendly and enemy become blu and red if off
		
			CVar(Visible, VA_LIST(std::vector<std::pair<std::string, Color_t>>) VA_LIST({ { "Original", {} } }), VISUAL)
			CVar(Occluded, VA_LIST(std::vector<std::pair<std::string, Color_t>>) {}, VISUAL)
		SUBNAMESPACE_END(Player)

		SUBNAMESPACE_BEGIN(Enemy)
			CVar(Players, false, VISUAL)
			CVar(Buildings, false, VISUAL)
			CVar(Ragdolls, false, VISUAL)
			CVar(Projectiles, false, VISUAL)

			CVar(Visible, VA_LIST(std::vector<std::pair<std::string, Color_t>>) VA_LIST({ { "Original", {} } }), VISUAL)
			CVar(Occluded, VA_LIST(std::vector<std::pair<std::string, Color_t>>) {}, VISUAL)
		SUBNAMESPACE_END(Enemy)

		SUBNAMESPACE_BEGIN(Team)
			CVar(Players, false, VISUAL)
			CVar(Buildings, false, VISUAL)
			CVar(Ragdolls, false, VISUAL)
			CVar(Projectiles, false, VISUAL)
			
			CVar(Visible, VA_LIST(std::vector<std::pair<std::string, Color_t>>) VA_LIST({ { "Original", {} } }), VISUAL)
			CVar(Occluded, VA_LIST(std::vector<std::pair<std::string, Color_t>>) {}, VISUAL)
		SUBNAMESPACE_END(Team)

		SUBNAMESPACE_BEGIN(World)
			CVar(NPCs, false, VISUAL)
			CVar(Pickups, false, VISUAL)
			CVar(Objective, false, VISUAL)
			CVar(Powerups, false, VISUAL)
			CVar(Bombs, false, VISUAL)
			CVar(Halloween, false, VISUAL)
		
			CVar(Visible, VA_LIST(std::vector<std::pair<std::string, Color_t>>) VA_LIST({ { "Original", {} } }), VISUAL)
			CVar(Occluded, VA_LIST(std::vector<std::pair<std::string, Color_t>>) {}, VISUAL)
		SUBNAMESPACE_END(World)

		SUBNAMESPACE_BEGIN(Backtrack)
			CVar(Enabled, false, VISUAL)
			CVar(IgnoreZ, false, VISUAL)
			CVarEnum(Draw, 0b0001, VISUAL | MULTI, VA_LIST( { "Last", "Last + first", "All", "Ignore Team" } ), 
					  Last = 1 << 0, First = 1 << 1, Always = 1 << 2, IgnoreTeam = 1 << 3 )
			CVar(Visible, VA_LIST(std::vector<std::pair<std::string, Color_t>>) VA_LIST({ { "Original", {} } }), VISUAL)
			//CVar(Occluded, VA_LIST(std::vector<std::pair<std::string, Color_t>>) {}, VISUAL) // unused
		SUBNAMESPACE_END(Backtrack)

		SUBNAMESPACE_BEGIN(FakeAngle)
			CVar(Enabled, false, VISUAL)
			CVar(IgnoreZ, false, VISUAL)

			CVar(Visible, VA_LIST(std::vector<std::pair<std::string, Color_t>>) VA_LIST({ { "Original", {} } }), VISUAL)
			//CVar(Occluded, VA_LIST(std::vector<std::pair<std::string, Color_t>>) {}, VISUAL) // unused
		SUBNAMESPACE_END(FakeAngle)

		SUBNAMESPACE_BEGIN(Viewmodel)
			CVar(Weapon, false, VISUAL)
			CVar(Hands, false, VISUAL)
			
			CVar(WeaponVisible, VA_LIST(std::vector<std::pair<std::string, Color_t>>) VA_LIST({ { "Original", {} } }), VISUAL)
			CVar(HandsVisible, VA_LIST(std::vector<std::pair<std::string, Color_t>>) VA_LIST({ { "Original", {} } }), VISUAL)
		SUBNAMESPACE_END(Viewmodel)
	NAMESPACE_END(Chams)

	NAMESPACE_BEGIN(Glow)
		SUBNAMESPACE_BEGIN(Player)
			CVar(Local, false, VISUAL)
			CVar(Priority, false, VISUAL)
			CVar(Friend, false, VISUAL)
			CVar(Party, false, VISUAL)
			CVar(Target, false, VISUAL)
				
			CVar(Stencil, 1, VISUAL)
			CVar(Blur, 0, VISUAL)
		SUBNAMESPACE_END(Player)

		SUBNAMESPACE_BEGIN(Enemy)
			CVar(Players, false, VISUAL)
			CVar(Buildings, false, VISUAL)
			CVar(Ragdolls, false, VISUAL)
			CVar(Projectiles, false, VISUAL)
				
			CVar(Stencil, 1, VISUAL)
			CVar(Blur, 0, VISUAL)
		SUBNAMESPACE_END(Enemy)
			
		SUBNAMESPACE_BEGIN(Team)
			CVar(Players, false, VISUAL)
			CVar(Buildings, false, VISUAL)
			CVar(Ragdolls, false, VISUAL)
			CVar(Projectiles, false, VISUAL)
				
			CVar(Stencil, 1, VISUAL)
			CVar(Blur, 0, VISUAL)
		SUBNAMESPACE_END(Team)

		SUBNAMESPACE_BEGIN(World)
			CVar(NPCs, false, VISUAL)
			CVar(Pickups, false, VISUAL)
			CVar(Objective, false, VISUAL)
			CVar(Powerups, false, VISUAL)
			CVar(Bombs, false, VISUAL)
			CVar(Halloween, false, VISUAL)
				
			CVar(Stencil, 1, VISUAL)
			CVar(Blur, 0, VISUAL)
		SUBNAMESPACE_END(World)

		SUBNAMESPACE_BEGIN(Backtrack)
			CVar(Enabled, false, VISUAL)
			CVarEnum( Draw, 0b0001, VISUAL | MULTI, VA_LIST( { "Last", "Last + first", "All", "Ignore Team" } ),
					Last = 1 << 0, First = 1 << 1, Always = 1 << 2, IgnoreTeam = 1 << 3 )
				
			CVar(Stencil, 1, VISUAL)
			CVar(Blur, 0, VISUAL)
		SUBNAMESPACE_END(Backtrack)

		SUBNAMESPACE_BEGIN(FakeAngle)
			CVar(Enabled, false, VISUAL)
				
			CVar(Stencil, 1, VISUAL)
			CVar(Blur, 0, VISUAL)
		SUBNAMESPACE_END(FakeAngle)

		SUBNAMESPACE_BEGIN(Viewmodel)
			CVar(Weapon, false, VISUAL)
			CVar(Hands, false, VISUAL)

			CVar(Stencil, 1, VISUAL)
			CVar(Blur, 0, VISUAL)
		SUBNAMESPACE_END(Viewmodel)
	NAMESPACE_END(GLOW)

	NAMESPACE_BEGIN(Visuals)
		SUBNAMESPACE_BEGIN(Removals)
			CVar(Scope, false, VISUAL)
			CVar(Interpolation, false)
			CVar(Disguises, false, VISUAL)
			CVar(ScreenOverlays, false, VISUAL)
			CVar(Taunts, false, VISUAL)
			CVar(ScreenEffects, false, VISUAL)
			CVar(ViewPunch, false, VISUAL)
			CVar(AngleForcing, false, VISUAL)
			CVar(PostProcessing, false, VISUAL)
			CVar(MOTD, false, VISUAL)
		SUBNAMESPACE_END(Removals)

		SUBNAMESPACE_BEGIN(UI)
			CVarEnum(StreamerMode, 0, VISUAL, VA_LIST( { "Off", "Local", "Friends", "Party", "All" } ), Off, Local, Friends, Party, All)
			CVarEnum(ChatTags, 0b000, VISUAL | MULTI, VA_LIST( { "Local", "Friends", "Party", "Assigned" } ), Local = 1 << 0, Friends = 1 << 1, Party = 1 << 2, Assigned = 1 << 3)
			CVar(FieldOfView, 0.f, VISUAL)
			CVar(ZoomFieldOfView, 0.f, VISUAL)
			CVar(AspectRatio, 0.f, VISUAL)
			CVar(RevealScoreboard, false, VISUAL)
			CVar(ScoreboardUtility, false)
			CVar(ScoreboardColors, false, VISUAL)
			CVar(CleanScreenshots, true)
			CVar(SniperSightlines, false, VISUAL)
			CVar(PickupTimers, false, VISUAL)
		SUBNAMESPACE_END(Viewmodel)

		SUBNAMESPACE_BEGIN(Viewmodel)
			CVar(CrosshairAim, false, VISUAL)
			CVar(ViewmodelAim, false, VISUAL)
			CVar(OffsetX, 0, VISUAL)
			CVar(OffsetY, 0, VISUAL)
			CVar(OffsetZ, 0, VISUAL)
			CVar(Pitch, 0, VISUAL)
			CVar(Yaw, 0, VISUAL)
			CVar(Roll, 0, VISUAL)
			CVar(SwayScale, 0.f, VISUAL)
			CVar(SwayInterp, 0.f, VISUAL)
			CVar(FieldOfView, 0.f, VISUAL)
		SUBNAMESPACE_END(Viewmodel)

		SUBNAMESPACE_BEGIN(Particles)
			CVar(BulletTrail, std::string("Off"), VISUAL)
			CVar(CritTrail, std::string("Off"), VISUAL)
			CVar(MedigunBeam, std::string("Off"), VISUAL)
			CVar(MedigunCharge, std::string("Off"), VISUAL)
			CVar(ProjectileTrail, std::string("Off"), VISUAL)
			CVarEnum(SpellFootsteps, 0, VISUAL, VA_LIST( { "Off", "Color", "Team", "Halloween" } ), Off, Color, Team, Halloween)
			CVar(DrawIconsThroughWalls, false, VISUAL)
			CVar(DrawDamageNumbersThroughWalls, false, VISUAL)
		SUBNAMESPACE_END(Tracers)

		SUBNAMESPACE_BEGIN(Beams) // as of now, these will stay out of the menu
			CVar(Active, false, VISUAL)
			CVar(BeamColor, Color_t(255, 255, 255, 255), VISUAL)
			CVar(Model, std::string("sprites/physbeam.vmt"), VISUAL)
			CVar(Life, 2.f, VISUAL)
			CVar(Width, 2.f, VISUAL)
			CVar(EndWidth, 2.f, VISUAL)
			CVar(FadeLength, 10.f, VISUAL)
			CVar(Amplitude, 2.f, VISUAL)
			CVar(Brightness, 255.f, VISUAL)
			CVar(Speed, 0.2f, VISUAL)
			CVar(Flags, 0b10000000100000000, VISUAL | MULTI ) // { Reverse, Halobeam, Forever, Is active, End visible, Start visible, Use hitboxes, No tile, Only noise once, Shade out, Shade in, Solid, Sine noise, Fade out, Fade in, End entity, Start entity }
			CVar(Segments, 2, VISUAL)
		SUBNAMESPACE_END(Beams)

		SUBNAMESPACE_BEGIN(Ragdolls)
			CVar(NoRagdolls, false, VISUAL)
			CVar(NoGib, false, VISUAL)
			CVar(Enabled, false, VISUAL)
			CVar(EnemyOnly, false, VISUAL)
			CVarEnum(Effects, 0b0000, VISUAL | MULTI, VA_LIST( { "Burning", "Electrocuted", "Ash", "Dissolve" } ), Burning = 1 << 0, Electrocuted = 1 << 1, Ash = 1 << 2, Dissolve = 1 << 3)
			CVarEnum(Type, 0, VISUAL, VA_LIST( { "None", "Gold", "Ice" } ), None, Gold, Ice)
			CVar(Force, 1.f, VISUAL)
			CVar(ForceHorizontal, 1.f, VISUAL)
			CVar(ForceVertical, 1.f, VISUAL)
		SUBNAMESPACE_END(RagdollEffects)

		SUBNAMESPACE_BEGIN(Line)
			CVar(Enabled, false, VISUAL)
			CVar(DrawDuration, 5.f, VISUAL)
		SUBNAMESPACE_END(Line)

		SUBNAMESPACE_BEGIN(Simulation)
			Enum(Style, Off, Line, Separators, Spaced, Arrows, Boxes, Nitro);
			CVar(PlayerPath, 0, VA_LIST( { "Off", "Line", "Separators", "Spaced", "Arrows", "Boxes", "Nitro" } ), VISUAL)
			CVar(ProjectilePath, 0, VA_LIST( { "Off", "Line", "Separators", "Spaced", "Arrows", "Boxes", "Nitro" } ), VISUAL)
			CVar(TrajectoryPath, 0, VA_LIST( { "Off", "Line", "Separators", "Spaced", "Arrows", "Boxes", "Nitro" } ), VISUAL)
			CVar(ShotPath, 0, VA_LIST( { "Off", "Line", "Separators", "Spaced", "Arrows", "Boxes", "Nitro" } ), VISUAL)
			CVarEnum(SplashRadius, 0b0, VISUAL | MULTI, VA_LIST( { "Simulation", "##Divider", "Priority", "Enemy", "Team", "Local", "Friends", "Party", "##Divider", "Rockets", "Stickies", "Pipes", "Scorch shot", "##Divider", "Trace" } ), Simulation = 1 << 0, Priority = 1 << 1, Enemy = 1 << 2, Team = 1 << 3, Local = 1 << 4, Friends = 1 << 5, Party = 1 << 6, Rockets = 1 << 7, Stickies = 1 << 8, Pipes = 1 << 9, ScorchShot = 1 << 10, Trace = 1 << 11)
			CVar(Timed, false, VISUAL)
			CVar(Box, true, VISUAL)
			CVar(ProjectileCamera, false, VISUAL)
			CVar(ProjectileWindow, WindowBox_t(), NOBIND)
			CVar(SwingLines, false, VISUAL)
			CVar(DrawDuration, 5.f, VISUAL)
			CVar(SeparatorSpacing, 4)
			CVar(SeparatorLength, 12)

		SUBNAMESPACE_END(ProjectileTrajectory)

		SUBNAMESPACE_BEGIN(Trajectory)
			CVar(Override, false, DEBUGVAR)
			CVar(OffX, 16.f, DEBUGVAR)
			CVar(OffY, 8.f, DEBUGVAR)
			CVar(OffZ, -6.f, DEBUGVAR)
			CVar(Pipes, true, DEBUGVAR)
			CVar(Hull, 5.f, DEBUGVAR)
			CVar(Speed, 1200.f, DEBUGVAR)
			CVar(Gravity, 1.f, DEBUGVAR)
			CVar(NoSpin, false, DEBUGVAR)
			CVar(LifeTime, 2.2f, DEBUGVAR)
			CVar(UpVelocity, 200.f, DEBUGVAR)
			CVar(AngVelocityX, 600.f, DEBUGVAR)
			CVar(AngVelocityY, -1200.f, DEBUGVAR)
			CVar(AngVelocityZ, 0.f, DEBUGVAR)
			CVar(Drag, 1.f, DEBUGVAR)
			CVar(DragBasisX, 0.003902f, DEBUGVAR)
			CVar(DragBasisY, 0.009962f, DEBUGVAR) 
			CVar(DragBasisZ, 0.009962f, DEBUGVAR)
			CVar(AngDragBasisX, 0.003618f, DEBUGVAR)
			CVar(AngDragBasisY, 0.001514f, DEBUGVAR)
			CVar(AngDragBasisZ, 0.001514f, DEBUGVAR)
			CVar(MaxVelocity, 2000.f, DEBUGVAR)
			CVar(MaxAngularVelocity, 3600.f, DEBUGVAR)
		SUBNAMESPACE_END(ProjectileTrajectory)

		SUBNAMESPACE_BEGIN(Hitbox)
			CVarEnum(BonesEnabled, 0b00, VISUAL | MULTI, VA_LIST( { "On shot", "On hit" } ), OnShot = 1 << 0, OnHit = 1 << 1)
			CVarEnum(BoundsEnabled, 0b000, VISUAL | MULTI, VA_LIST( { "On shot", "On hit", "Aim point" } ), OnShot = 1 << 0, OnHit = 1 << 1, AimPoint = 1 << 2)
			CVar(DrawDuration, 5.f, VISUAL)
		SUBNAMESPACE_END(Hitbox)

		SUBNAMESPACE_BEGIN(ThirdPerson)
			CVar(Enabled, false, VISUAL)
			CVar(Distance, 200.f, VISUAL)
			CVar(Right, 0.f, VISUAL)
			CVar(Up, 0.f, VISUAL)
			CVar(Crosshair, false, VISUAL)
			CVar(Scale, true, DEBUGVAR)
			CVar(Collide, true, DEBUGVAR)
		SUBNAMESPACE_END(ThirdPerson)

		SUBNAMESPACE_BEGIN(FOVArrows)
			CVar(Enabled, false, VISUAL)
			CVar(Offset, 25, VISUAL)
			CVar(MaxDist, 1000.f, VISUAL)
		SUBNAMESPACE_END(Arrows)

		SUBNAMESPACE_BEGIN(World)
			CVarEnum(Modulations, 0b00000, VISUAL | MULTI, VA_LIST( { "World", "Sky", "Prop", "Particle", "Fog" } ), World = 1 << 0, Sky = 1 << 1, Prop = 1 << 2, Particle = 1 << 3, Fog = 1 << 4)
			CVar(SkyboxChanger, std::string("Off"), VISUAL)
			CVar(WorldTexture, std::string("Default"), VISUAL)
			CVar(NearPropFade, false, VISUAL)
			CVar(NoPropFade, false, VISUAL)
		SUBNAMESPACE_END(World)

		SUBNAMESPACE_BEGIN(Misc)
			CVar(LocalDominationOverride, std::string(""), VISUAL)
			CVar(LocalRevengeOverride, std::string(""), VISUAL)
			CVar(DominationOverride, std::string(""), VISUAL)
			CVar(RevengeOverride, std::string(""), VISUAL)
		SUBNAMESPACE_END(Misc)
	NAMESPACE_END(Visuals)

	NAMESPACE_BEGIN(Radar)
		SUBNAMESPACE_BEGIN(Main)
			CVar(Enabled, false, VISUAL)
			CVar(AlwaysDraw, true, VISUAL)
			CVarEnum(Style, 0, VISUAL, VA_LIST( { "Circle", "Rectangle" } ), Circle, Rectangle)
			CVar(Window, WindowBox_t(), NOBIND)
			CVar(Range, 1500, VISUAL)
			CVar(BackAlpha, 128, VISUAL)
			CVar(LineAlpha, 255, VISUAL)
		SUBNAMESPACE_END(Main)

		SUBNAMESPACE_BEGIN(Players)
			CVar(Enabled, false, VISUAL)
			CVar(Background, true, VISUAL)
			CVarEnum(Draw, 0b1001010, VISUAL | MULTI, VA_LIST( { "Local", "Enemy", "Team", "Friends", "Party", "Prioritized", "Cloaked" } ), Local = 1 << 0, Enemy = 1 << 1, Team = 1 << 2, Prioritized = 1 << 3, Friends = 1 << 4, Party = 1 << 5, Cloaked = 1 << 6)
			CVarEnum(IconType, 1, VISUAL, VA_LIST( { "Icons", "Portraits", "Avatar" } ), Icons, Portraits, Avatars)
			CVar(IconSize, 24, VISUAL)
			CVar(Health, false, VISUAL)
			CVar(Height, false, VISUAL)
		SUBNAMESPACE_END(Players)

		SUBNAMESPACE_BEGIN(Buildings)
			CVar(Enabled, false, VISUAL)
			CVar(Background, true, VISUAL)
			CVarEnum(Draw, 0b001011, VISUAL | MULTI, VA_LIST( { "Local", "Enemy", "Team", "Friends", "Party", "Prioritized" } ), Local = 1 << 0, Enemy = 1 << 1, Team = 1 << 2, Prioritized = 1 << 3, Friends = 1 << 4, Party = 1 << 5)
			CVar(Health, false, VISUAL)
			CVar(IconSize, 18, VISUAL)
		SUBNAMESPACE_END(Buildings)

		SUBNAMESPACE_BEGIN(World)
			CVar(Enabled, false, VISUAL)
			CVar(Background, true, VISUAL)
			CVarEnum(Draw, 0b0000011, VISUAL | MULTI, VA_LIST( { "Health", "Ammo", "Money", "Bombs", "Powerup", "Spellbook", "Gargoyle" } ), Health = 1 << 0, Ammo = 1 << 1, Money = 1 << 2, Bombs = 1 << 3, Powerup = 1 << 4, Spellbook = 1 << 5, Gargoyle = 1 << 6)
			CVar(IconSize, 14, VISUAL)
		SUBNAMESPACE_END(World)
	NAMESPACE_END(Radar)

	NAMESPACE_BEGIN(Misc)
		SUBNAMESPACE_BEGIN(Movement)
			CVarEnum(AutoStrafe, 0, NONE, VA_LIST( { "Off", "Legit", "Directional" } ), Off, Legit, Directional)
			CVar(AutoStrafeTurnScale, 0.5f)
			CVar(AutoStrafeMaxDelta, 180.f)
			CVar(Bunnyhop, false)
			CVar(EdgeJump, false)
			CVar(AutoJumpbug, false)
			CVar(NoPush, false)
			CVar(AutoRocketJump, false)
			CVar(AutoCTap, false)
			CVar(FastStop, false)
			CVar(FastAccel, false)
			CVar(CrouchSpeed, false)
			CVar(MovementLock, false)
			CVar(BreakJump, false)
			CVar(ShieldTurnRate, false)

			CVar(TimingOffset, 0, DEBUGVAR)
			CVar(ChokeCount, 1, DEBUGVAR)
			CVar(ApplyAbove, 0, DEBUGVAR)
		SUBNAMESPACE_END(Movement)

		SUBNAMESPACE_BEGIN(Exploits)
			CVar(CheatsBypass, false)
			CVar(BypassPure, false)
			CVar(PingReducer, false)
			CVar(PingTarget, 1)
			CVar(EquipRegionUnlock, false)
		SUBNAMESPACE_END(Exploits)

		SUBNAMESPACE_BEGIN(Automation)
			CVarEnum(AntiBackstab, 0, NONE, VA_LIST( { "Off", "Yaw", "Pitch", "Fake" } ), Off, Yaw, Pitch, Fake)
			CVar(AntiAFK, false)
			CVar(AntiAutobalance, false)
			CVar(TauntControl, false)
			CVar(KartControl, false)
			CVar(BackpackExpander, true)
			CVar(AcceptItemDrops, false)
			CVar(AutoF2Ignored, false)
			CVar(AutoF1Priority, false)
		SUBNAMESPACE_END(Automation)

		SUBNAMESPACE_BEGIN(Sound)
			CVarEnum(Block, 0b0000, MULTI, VA_LIST( { "Footsteps", "Noisemaker", "Frying pan", "Water" } ), Footsteps = 1 << 0, Noisemaker = 1 << 1, FryingPan = 1 << 2, Water = 1 << 3)
			CVar(HitsoundAlways, false)
			CVar(RemoveDSP, false)
			CVar(GiantWeaponSounds, false)
		SUBNAMESPACE_END(Sound)

		SUBNAMESPACE_BEGIN(Game)
			CVar(NetworkFix, false)
			CVar(PredictionErrorJitterFix, false)
			CVar(SetupBonesOptimization, false)
			CVar(F2PChatBypass, false)
			CVar(AntiCheatCompatibility, false)
			CVar(AntiCheatCritHack, false, DEBUGVAR)
		SUBNAMESPACE_END(Game)

		SUBNAMESPACE_BEGIN(Queueing)
			CVarEnum(ForceRegions, 0b0, MULTI, VA_LIST( { "Atlanta", "Chicago", "Texas", "Los Angeles", "Moses Lake", "New York", "Seattle", "Virginia", "##Divider", "Amsterdam", "Frankfurt", "Helsinki", "London", "Madrid", "Paris", "Stockholm", "Vienna", "Warsaw", "##Divider", "Buenos Aires", "Lima", "Santiago", "Sao Paulo", "##Divider", "Bombay", "Chennai", "Dubai", "Hong Kong", "Madras", "Mumbai", "Seoul", "Singapore", "Tokyo", "Sydney", "##Divider", "Johannesburg" } ), // i'm not sure all of these are actually used for tf2 servers
				// North America
				DC_ATL = 1 << 0, // Atlanta
				DC_ORD = 1 << 1, // Chicago
				DC_DFW = 1 << 2, // Texas
				DC_LAX = 1 << 3, // Los Angeles
				DC_EAT = 1 << 4, // Moses Lake
				DC_JFK = 1 << 5, // New York
				DC_SEA = 1 << 6, // Seattle
				DC_IAD = 1 << 7, // Virginia
				// Europe
				DC_AMS = 1 << 8, // Amsterdam
				DC_FRA = 1 << 9, // Frankfurt
				DC_HEL = 1 << 10, // Helsinki
				DC_LHR = 1 << 11, // London
				DC_MAD = 1 << 12, // Madrid
				DC_PAR = 1 << 13, // Paris
				DC_STO = 1 << 14, /*& DC_STO2*/ // Stockholm
				DC_VIE = 1 << 15, // Vienna
				DC_WAW = 1 << 16, // Warsaw
				// South America
				DC_EZE = 1 << 17, // Buenos Aires
				DC_LIM = 1 << 18, // Lima
				DC_SCL = 1 << 19, // Santiago
				DC_GRU = 1 << 20, // Sao Paulo
				// Asia
				DC_BOM2 = 1 << 21, // Bombay
				DC_MAA = 1 << 22, // Chennai
				DC_DXB = 1 << 23, // Dubai
				DC_HKG = 1 << 24, // Hong Kong
				DC_MAA2 = 1 << 25, // Madras
				DC_BOM = 1 << 26, // Mumbai
				DC_SEO = 1 << 27, // Seoul
				DC_SGP = 1 << 28, // Singapore
				DC_TYO = 1 << 29, // Tokyo
				// Australia
				DC_SYD = 1 << 30, // Sydney
				// Africa
				DC_JNB = 1 << 31, // Johannesburg
			)
			CVar(FreezeQueue, false)
			CVar(AutoCasualQueue, false)
		SUBNAMESPACE_END(Queueing)

		SUBNAMESPACE_BEGIN(MannVsMachine)
			CVar(InstantRespawn, false)
			CVar(InstantRevive, false)
			CVar(AllowInspect, false)
		SUBNAMESPACE_END(Sound)

		SUBNAMESPACE_BEGIN(Steam)
			CVar(EnableRPC, false)
			CVar(OverrideMenu, false)
			CVarEnum(MatchGroup, 0, NONE, VA_LIST( { "Special Event", "MvM Mann Up", "Competitive", "Casual", "MvM Boot Camp" } ), SpecialEvent, MvMMannUp, Competitive, Casual, MvMBootCamp)
			CVar(MapText, std::string("Amalgam"))
			CVar(GroupSize, 1337)
		SUBNAMESPACE_END(Steam)
	NAMESPACE_END(Misc)

	NAMESPACE_BEGIN(Logging)
		CVarEnum(Logs, 0b0000011, MULTI, VA_LIST( { "Vote start", "Vote cast", "Class changes", "Damage", "Cheat detection", "Tags", "Aliases", "Resolver" } ), VoteStart = 1 << 0, VoteCast = 1 << 1, ClassChanges = 1 << 2, Damage = 1 << 3, CheatDetection = 1 << 4, Tags = 1 << 5, Aliases = 1 << 6, Resolver = 1 << 7)
		Enum(LogTo, Toasts = 1 << 0, Chat = 1 << 1, Party = 1 << 2, Console = 1 << 3, Menu = 1 << 4, Debug = 1 << 5)
		CVar(Lifetime, 5.f, VISUAL)

		SUBNAMESPACE_BEGIN(VoteStart)
			CVar(LogTo, 0b000001, VA_LIST( { "Toasts", "Chat", "Party", "Console", "Menu", "Debug" } ), MULTI )
		SUBNAMESPACE_END(VoteStart)

		SUBNAMESPACE_BEGIN(VoteCast)
			CVar(LogTo, 0b000001, VA_LIST( { "Toasts", "Chat", "Party", "Console", "Menu", "Debug" } ), MULTI )
		SUBNAMESPACE_END(VoteCast)

		SUBNAMESPACE_BEGIN(ClassChange)
			CVar(LogTo, 0b000001, VA_LIST( { "Toasts", "Chat", "Party", "Console", "Menu", "Debug" } ), MULTI )
		SUBNAMESPACE_END(ClassChange)

		SUBNAMESPACE_BEGIN(Damage)
			CVar(LogTo, 0b000001, VA_LIST( { "Toasts", "Chat", "Party", "Console", "Menu", "Debug" } ), MULTI )
		SUBNAMESPACE_END(Damage)

		SUBNAMESPACE_BEGIN(CheatDetection)
			CVar(LogTo, 0b000001, VA_LIST( { "Toasts", "Chat", "Party", "Console", "Menu", "Debug" } ), MULTI )
		SUBNAMESPACE_END(CheatDetection)

		SUBNAMESPACE_BEGIN(Tags)
			CVar(LogTo, 0b000001, VA_LIST( { "Toasts", "Chat", "Party", "Console", "Menu", "Debug" } ), MULTI )
		SUBNAMESPACE_END(Tags)

		SUBNAMESPACE_BEGIN(Aliases)
			CVar(LogTo, 0b000001, VA_LIST( { "Toasts", "Chat", "Party", "Console", "Menu", "Debug" } ), MULTI )
		SUBNAMESPACE_END(Aliases)

		SUBNAMESPACE_BEGIN(Resolver)
			CVar(LogTo, 0b000001, VA_LIST( { "Toasts", "Chat", "Party", "Console", "Menu", "Debug" } ), MULTI )
		SUBNAMESPACE_END(Resolver)
	NAMESPACE_END(Logging)

	NAMESPACE_BEGIN(Debug)
		CVar(Info, false, NOSAVE)
		CVar(Logging, false, NOSAVE)
		CVar(Options, false, NOSAVE)
		CVar(ServerHitbox, false, NOSAVE)
		CVar(AntiAimLines, false)
		CVar(CrashLogging, true)
#ifdef DEBUG_TRACES
		CVar(VisualizeTraces, false, NOSAVE)
		CVar(VisualizeTraceHits, false, NOSAVE)
#endif
	NAMESPACE_END(Debug)

#ifdef DEBUG_HOOKS
	NAMESPACE_BEGIN(Hooks)
		CVar(bf_read_ReadString, true, NOSAVE)
		CVar(CAchievementMgr_CheckAchievementsEnabled, true, NOSAVE)
		CVar(CAttributeManager_AttribHookValue, true, NOSAVE)
		CVar(CBaseAnimating_Interpolate, true, NOSAVE)
		CVar(CBaseAnimating_MaintainSequenceTransitions, true, NOSAVE)
		CVar(CBaseAnimating_SetSequence, true, NOSAVE)
		CVar(CBaseAnimating_SetupBones, true, NOSAVE)
		CVar(CBaseAnimating_UpdateClientSideAnimation, true, NOSAVE)
		CVar(CBaseEntity_BaseInterpolatePart1, true, NOSAVE)
		CVar(CBaseEntity_EstimateAbsVelocity, true, NOSAVE)
		CVar(CBaseEntity_FireBullets, true, NOSAVE)
		CVar(CBaseEntity_ResetLatched, true, NOSAVE)
		CVar(CBaseEntity_SetAbsVelocity, true, NOSAVE)
		CVar(CBaseEntity_WorldSpaceCenter, true, NOSAVE)
		CVar(CBaseHudChatLine_InsertAndColorizeText, true, NOSAVE)
		CVar(CBasePlayer_CalcPlayerView, true, NOSAVE)
		CVar(CBasePlayer_CalcViewModelView, true, NOSAVE)
		CVar(CBasePlayer_ItemPostFrame, true, NOSAVE)
		CVar(CBaseViewModel_ShouldFlipViewModel, true, NOSAVE)
		CVar(Cbuf_ExecuteCommand, true, NOSAVE)
		CVar(CClientModeShared_CreateMove, true, NOSAVE)
		CVar(CClientModeShared_DoPostScreenSpaceEffects, true, NOSAVE)
		CVar(CClientModeShared_OverrideView, true, NOSAVE)
		CVar(CClientModeShared_ShouldDrawViewModel, true, NOSAVE)
		CVar(CClientState_GetClientInterpAmount, true, NOSAVE)
		CVar(CClientState_ProcessFixAngle, true, NOSAVE)
		CVar(CHudCrosshair_GetDrawPosition, true, NOSAVE)
		CVar(CInventoryManager_ShowItemsPickedUp, true, NOSAVE)
		CVar(CL_CheckForPureServerWhitelist, true, NOSAVE)
		CVar(CL_Move, true, NOSAVE)
		CVar(CL_ProcessPacketEntities, true, NOSAVE)
		CVar(CL_ReadPackets, true, NOSAVE)
		CVar(ClientModeTFNormal_BIsFriendOrPartyMember, true, NOSAVE)
		CVar(ClientModeTFNormal_UpdateSteamRichPresence, true, NOSAVE)
		CVar(CMatchInviteNotification_OnTick, true, NOSAVE)
		CVar(CMaterial_Uncache, true, NOSAVE)
		CVar(CNetChannel_SendDatagram, true, NOSAVE)
		CVar(CNetChannel_SendNetMsg, true, NOSAVE)
		CVar(COPRenderSprites_Render, true, NOSAVE)
		CVar(CParticleProperty_Create, true, NOSAVE)
		CVar(CPlayerResource_GetPlayerName, true, NOSAVE)
		CVar(CPrediction_RunSimulation, true, NOSAVE)
		CVar(CRendering3dView_EnableWorldFog, true, NOSAVE)
		CVar(CSkyboxView_Enable3dSkyboxFog, true, NOSAVE)
		CVar(CSniperDot_GetRenderingPositions, true, NOSAVE)
		CVar(CSoundEmitterSystem_EmitSound, true, NOSAVE)
		CVar(CStaticPropMgr_ComputePropOpacity, true, NOSAVE)
		CVar(CStaticPropMgr_DrawStaticProps, true, NOSAVE)
		CVar(CStudioRender_DrawModelStaticProp, true, NOSAVE)
		CVar(CStudioRender_SetAlphaModulation, true, NOSAVE)
		CVar(CStudioRender_SetColorModulation, true, NOSAVE)
		CVar(CTFBadgePanel_SetupBadge, true, NOSAVE)
		CVar(CTFClientScoreBoardDialog_UpdatePlayerAvatar, true, NOSAVE)
		CVar(CTFGCClientSystem_UpdateAssignedLobby, true, NOSAVE)
		CVar(CTFHudDeathNotice_AddAdditionalMsg, true, NOSAVE)
		CVar(CTFInput_ApplyMouse, true, NOSAVE)
		CVar(CTFPlayer_AvoidPlayers, true, NOSAVE)
		CVar(CTFPlayer_BRenderAsZombie, true, NOSAVE)
		CVar(CTFPlayer_BuildTransformations, true, NOSAVE)
		CVar(CTFPlayer_DoAnimationEvent, true, NOSAVE)
		CVar(CTFPlayer_IsPlayerClass, true, NOSAVE)
		CVar(CTFPlayer_ShouldDraw, true, NOSAVE)
		CVar(CTFPlayer_UpdateStepSound, true, NOSAVE)
		CVar(CTFPlayerInventory_GetMaxItemCount, true, NOSAVE)
		CVar(CTFPlayerPanel_GetTeam, true, NOSAVE)
		CVar(CTFPlayerShared_InCond, true, NOSAVE)
		CVar(CTFPlayerShared_IsPlayerDominated, true, NOSAVE)
		CVar(CTFRagdoll_CreateTFRagdoll, true, NOSAVE)
		CVar(CTFScatterGun_FireBullet, true, NOSAVE)
		CVar(CTFWeaponBase_CalcIsAttackCritical, true, NOSAVE)
		CVar(CTFWeaponBase_GetShootSound, true, NOSAVE)
		CVar(CThirdPersonManager_Update, true, NOSAVE)
		CVar(CViewRender_DrawUnderwaterOverlay, true, NOSAVE)
		CVar(CViewRender_LevelInit, true, NOSAVE)
		CVar(CViewRender_PerformScreenOverlay, true, NOSAVE)
		CVar(CViewRender_RenderView, true, NOSAVE)
		CVar(DoEnginePostProcessing, true, NOSAVE)
		CVar(DSP_Process, true, NOSAVE)
		CVar(FX_FireBullets, true, NOSAVE)
		CVar(GenerateEquipRegionConflictMask, true, NOSAVE)
		CVar(GetClientInterpAmount, true, NOSAVE)
		CVar(HostState_Shutdown, true, NOSAVE)
		CVar(IBaseClientDLL_DispatchUserMessage, true, NOSAVE)
		CVar(IBaseClientDLL_FrameStageNotify, true, NOSAVE)
		CVar(IBaseClientDLL_LevelShutdown, true, NOSAVE)
		CVar(IEngineTrace_SetTraceEntity, true, NOSAVE)
		CVar(IEngineTrace_TraceRay, true, NOSAVE)
		CVar(IEngineVGui_Paint, true, NOSAVE)
		CVar(IInput_GetUserCmd, true, NOSAVE)
		CVar(IMatSystemSurface_OnScreenSizeChanged, true, NOSAVE)
		CVar(IPanel_PaintTraverse, true, NOSAVE)
		CVar(ISteamFriends_GetFriendPersonaName, true, NOSAVE)
		CVar(ISteamNetworkingUtils_GetPingToDataCenter, true, NOSAVE)
		CVar(IVEngineClient_ClientCmd_Unrestricted, true, NOSAVE)
		CVar(IVModelRender_DrawModelExecute, true, NOSAVE)
		CVar(IVModelRender_ForcedMaterialOverride, true, NOSAVE)
		CVar(KeyValues_SetInt, true, NOSAVE)
		CVar(NotificationQueue_Add, true, NOSAVE)
		CVar(R_DrawSkyBox, true, NOSAVE)
		CVar(RecvProxy_SimulationTime, true, NOSAVE)
		CVar(TF_IsHolidayActive, true, NOSAVE)
		CVar(VGuiMenuBuilder_AddMenuItem, true, NOSAVE)
	NAMESPACE_END(Hooks)
#endif
}