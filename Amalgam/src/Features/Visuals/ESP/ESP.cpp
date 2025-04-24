#include "ESP.h"

#include "../../Players/PlayerUtils.h"
#include "../../Spectate/Spectate.h"
#include "../../Simulation/MovementSimulation/MovementSimulation.h"

MAKE_SIGNATURE(CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot, "client.dll", "48 89 6C 24 ? 56 41 54 41 55 41 56 41 57 48 83 EC", 0x0);
MAKE_SIGNATURE(CEconItemView_GetItemName, "client.dll", "40 53 48 83 EC ? 48 8B D9 C6 81 ? ? ? ? ? E8 ? ? ? ? 48 8B 8B", 0x0);

void CESP::Store(CTFPlayer* pLocal)
{
	m_mPlayerCache.clear();
	m_mBuildingCache.clear();
	m_mWorldCache.clear();

	if (!Vars::ESP::Draw.Value || !pLocal)
		return;

	StorePlayers(pLocal);
	StoreBuildings(pLocal);
	StoreProjectiles(pLocal);
	StoreObjective(pLocal);
	StoreWorld();
}

void CESP::StorePlayers(CTFPlayer* pLocal)
{
	if (!(Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Players) || !Vars::ESP::Player.Value)
		return;

	auto pObserverTarget = pLocal->m_hObserverTarget( ).Get( );
	int iObserverMode = pLocal->m_iObserverMode( );
	if ( F::Spectate.m_iTarget != -1 )
	{
		pObserverTarget = F::Spectate.m_pTargetTarget;
		iObserverMode = F::Spectate.m_iTargetMode;
	}

	auto pResource = H::Entities.GetPR();
	for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ALL))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		int iIndex = pPlayer->entindex();

		bool bLocal = iIndex == I::EngineClient->GetLocalPlayer();
		bool bSpectate = iObserverMode == OBS_MODE_FIRSTPERSON || iObserverMode == OBS_MODE_THIRDPERSON;
		bool bTarget = bSpectate && pObserverTarget == pPlayer;

		if (!pPlayer->IsAlive() || pPlayer->IsAGhost())
			continue;

		if (bLocal || bTarget)
		{
			if (!(Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Local) || !I::Input->CAM_IsThirdPerson())
				continue;
		}
		else
		{
			if (pPlayer->IsDormant())
			{
				if (!H::Entities.GetDormancy(iIndex) || !Vars::ESP::DormantAlpha.Value
					|| Vars::ESP::DormantPriority.Value && !F::PlayerUtils.IsPrioritized(iIndex))
					continue;
			}

			if (!(Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Prioritized && F::PlayerUtils.IsPrioritized(iIndex))
				&& !(Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Friends && H::Entities.IsFriend(iIndex))
				&& !(Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Party && H::Entities.InParty(iIndex))
				&& pPlayer->m_iTeamNum() == pLocal->m_iTeamNum() ? !(Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Team) : !(Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Enemy))
				continue;
		}

		int iClassNum = pPlayer->m_iClass();
		auto pWeapon = pPlayer->m_hActiveWeapon().Get()->As<CTFWeaponBase>();

		PlayerCache& tCache = m_mPlayerCache[pEntity];
		tCache.m_flAlpha = (pPlayer->IsDormant() ? Vars::ESP::DormantAlpha.Value : Vars::ESP::ActiveAlpha.Value) / 255.f;
		tCache.m_tColor = H::Color.GetTeamColor(pLocal->m_iTeamNum(), pPlayer->m_iTeamNum(), Vars::Colors::Relative.Value);
		tCache.m_bBox = Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Box;
		tCache.m_bBones = Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Bones;

		if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Distance && !bLocal)
		{
			Vec3 vDelta = pPlayer->m_vecOrigin() - pLocal->m_vecOrigin();
			tCache.m_vText.push_back({ ESPTextEnum::Bottom, std::format("[{:.0f}M]", vDelta.Length2D() / 41), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });
		}

		PlayerInfo_t pi{};
		if (I::EngineClient->GetPlayerInfo(iIndex, &pi))
		{
			if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Name)
				tCache.m_vText.push_back({ ESPTextEnum::Top, F::PlayerUtils.GetPlayerName(iIndex, pi.name), H::Color.GetEntityNameColor(pLocal, pPlayer, Vars::Colors::Relative.Value), Vars::Menu::Theme::Background.Value });

			//std::vector<PriorityLabel_t> vTags = {};
			if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Priority)
			{
				if (auto pTag = F::PlayerUtils.GetSignificantTag(pi.friendsID, 1)) // 50 alpha as white outline tends to be more apparent
					tCache.m_vText.push_back({ ESPTextEnum::Bottom, pTag->Name, pTag->Color, H::Draw.IsColorDark(pTag->Color) ? Color_t(255, 255, 255, 50) : Color_t(0, 0, 0, 255) });
			}

			if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Labels)
			{
				std::vector<PriorityLabel_t*> vTags = {};
				for (auto& iID : F::PlayerUtils.m_mPlayerTags[pi.friendsID])
				{
					auto pTag = F::PlayerUtils.GetTag(iID);
					if (pTag && pTag->Label)
						vTags.push_back(pTag);
				}
				if (H::Entities.IsFriend(iIndex))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(FRIEND_TAG)];
					if (pTag->Label)
						vTags.push_back(pTag);
				}
				if (H::Entities.InParty(iIndex))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(PARTY_TAG)];
					if (pTag->Label)
						vTags.push_back(pTag);
				}

				if (vTags.size())
				{
					std::sort(vTags.begin(), vTags.end(), [&](const auto a, const auto b) -> bool
						{
							// sort by priority if unequal
							if (a->Priority != b->Priority)
								return a->Priority > b->Priority;

							return a->Name < b->Name;
						});

					for (auto& pTag : vTags) // 50 alpha as white outline tends to be more apparent
						tCache.m_vText.push_back({ ESPTextEnum::Right, pTag->Name, pTag->Color, H::Draw.IsColorDark(pTag->Color) ? Color_t(255, 255, 255, 50) : Color_t(0, 0, 0, 255) });
				}
			}
		}

		float flHealth = pPlayer->m_iHealth(), flMaxHealth = pPlayer->GetMaxHealth();
		if (tCache.m_bHealthBar = Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::HealthBar)
		{
			if (flHealth > flMaxHealth)
			{
				float flMaxOverheal = floorf(flMaxHealth / 10.f) * 5;
				tCache.m_flHealth = 1.f + std::clamp((flHealth - flMaxHealth) / flMaxOverheal, 0.f, 1.f);
			}
			else
				tCache.m_flHealth = std::clamp(flHealth / flMaxHealth, 0.f, 1.f);
		}

		if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::HealthText)
		{
			{
			
				tCache.m_vText.push_back({ ESPTextEnum::Health, std::format("{}", flHealth), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });
			}
		}

		if (iClassNum == TF_CLASS_MEDIC)
		{
			auto pMediGun = pPlayer->GetWeaponFromSlot(SLOT_SECONDARY);
			if (pMediGun && pMediGun->GetClassID() == ETFClassID::CWeaponMedigun)
			{
				tCache.m_flUber = std::clamp(pMediGun->As<CWeaponMedigun>()->m_flChargeLevel(), 0.f, 1.f);
				tCache.m_bUberBar = Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::UberBar;
				if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::UberText)
					tCache.m_vText.push_back({ ESPTextEnum::Uber, std::format("{:.0f}", tCache.m_flUber * 100.f), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });
			}
		}

		if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::ClassIcon)
			tCache.m_iClassIcon = iClassNum;
		if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::ClassText)
			tCache.m_vText.push_back({ ESPTextEnum::Right, GetPlayerClass(iClassNum), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });

		if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::WeaponIcon && pWeapon)
			tCache.m_pWeaponIcon = pWeapon->GetWeaponIcon();
		if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::WeaponText && pWeapon)
		{
			int iWeaponSlot = pWeapon->GetSlot();
			switch (pPlayer->m_iClass())
			{
			case TF_CLASS_SPY:
			{
				switch (iWeaponSlot)
				{
				case 0: iWeaponSlot = 1; break;
				case 1: iWeaponSlot = 4; break;
				case 3: iWeaponSlot = 5; break;
				}
				break;
			}
			case TF_CLASS_ENGINEER:
			{
				switch (iWeaponSlot)
				{
				case 3: iWeaponSlot = 5; break;
				case 4: iWeaponSlot = 6; break;
				}
				break;
			}
			}

			if (void* pCurItemData = S::CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot.Call<void*>(pPlayer, iWeaponSlot, nullptr))
				tCache.m_vText.emplace_back(ESPTextEnum::Bottom, SDK::ConvertWideToUTF8(S::CEconItemView_GetItemName.Call<const wchar_t*>(pCurItemData)), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		}

		if (Vars::Debug::Info.Value && !pPlayer->IsDormant() && pPlayer->entindex() != I::EngineClient->GetLocalPlayer())
		{
			int iAverage = TIME_TO_TICKS(F::MoveSim.GetPredictedDelta(pPlayer));
			int iCurrent = H::Entities.GetChoke(iIndex);
			tCache.m_vText.emplace_back(ESPTextEnum::Right, std::format("Lag {}, {}", iAverage, iCurrent), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		}

		{
			if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::LagCompensation && !pPlayer->IsDormant() && pPlayer != pLocal)
			{
				if (H::Entities.GetLagCompensation(iIndex))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Lagcomp", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			}

			if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Ping && pResource && pPlayer != pLocal)
			{
				auto pNetChan = I::EngineClient->GetNetChannelInfo();
				if (pNetChan && !pNetChan->IsLoopback())
				{
					int iPing = pResource->m_iPing(iIndex);
					if (iPing && (iPing >= 200 || iPing <= 5))
						tCache.m_vText.emplace_back(ESPTextEnum::Right, std::format("{} ms", iPing), Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				}
			}

			if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::KDR && pResource && pPlayer != pLocal)
			{
				int iKills = pResource->m_iScore(iIndex), iDeaths = pResource->m_iDeaths(iIndex);
				if (iKills >= 20)
				{
					int iKDR = iKills / std::max(iDeaths, 1);
					if (iKDR >= 10)
						tCache.m_vText.emplace_back(ESPTextEnum::Right, std::format("High KD [{} / {}]", iKills, iDeaths), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				}
			}

			// Buffs
			if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Buffs)
			{
				bool bCrits = false, bMiniCrits = false;
				if (pPlayer->IsCritBoosted())
					pWeapon&& pWeapon->GetWeaponID() == TF_WEAPON_PARTICLE_CANNON ? bMiniCrits = true : bCrits = true;
				if (pPlayer->IsMiniCritBoosted())

					pWeapon && SDK::AttribHookValue(0, "minicrits_become_crits", pWeapon) == 1 ? bCrits = true : bMiniCrits = true;
				if (pWeapon && SDK::AttribHookValue(0, "crit_while_airborne", pWeapon) && pPlayer->InCond(TF_COND_BLASTJUMPING))
					bCrits = true, bMiniCrits = false;

				if (bCrits)
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Crits", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				else if (bMiniCrits)
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Mini crits", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_RADIUSHEAL) ||
					pPlayer->InCond(TF_COND_HEALTH_BUFF) ||
					pPlayer->InCond(TF_COND_RADIUSHEAL_ON_DAMAGE) ||
					pPlayer->InCond(TF_COND_MEGAHEAL) ||
					pPlayer->InCond(TF_COND_HALLOWEEN_QUICK_HEAL) ||
					pPlayer->InCond(TF_COND_HALLOWEEN_HELL_HEAL) ||
					pPlayer->InCond(TF_COND_KING_BUFFED))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "HP++", Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value);
				else if (pPlayer->InCond(TF_COND_HEALTH_OVERHEALED))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "HP+", Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_INVULNERABLE) ||
					pPlayer->InCond(TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED) ||
					pPlayer->InCond(TF_COND_INVULNERABLE_USER_BUFF) ||
					pPlayer->InCond(TF_COND_INVULNERABLE_CARD_EFFECT))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Uber", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				else if (pPlayer->InCond(TF_COND_PHASE))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Bonk", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				/* vaccinator effects */
				if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_BULLET_RESIST) || pPlayer->InCond(TF_COND_BULLET_IMMUNE))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Bullet+", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				else if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_BULLET_RESIST))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Bullet", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_BLAST_RESIST) || pPlayer->InCond(TF_COND_BLAST_IMMUNE))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Blast+", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				else if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_BLAST_RESIST))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Blast", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_FIRE_RESIST) || pPlayer->InCond(TF_COND_FIRE_IMMUNE))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Fire+", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				else if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_FIRE_RESIST))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Fire", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_OFFENSEBUFF))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Banner", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_DEFENSEBUFF))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Battalions", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_REGENONDAMAGEBUFF))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Conch", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);

				//if (pPlayer->InCond(TF_COND_BLASTJUMPING))
				//	tCache.m_vText.emplace_back(ESPTextEnum::Right, "Blastjump", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_RUNE_STRENGTH))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Strength", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_HASTE))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Haste", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_REGEN))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Regen", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_RESIST))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Resistance", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_VAMPIRE))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Vampire", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_REFLECT))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Reflect", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_PRECISION))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Precision", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_AGILITY))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Agility", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_KNOCKOUT))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Knockout", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_KING))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "King", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_PLAGUE))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Plague", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_RUNE_SUPERNOVA))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Supernova", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (pPlayer->InCond(TF_COND_POWERUPMODE_DOMINANT))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Dominant", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
			}

			// Debuffs
			if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Debuffs)
			{
				if (pPlayer->InCond(TF_COND_MARKEDFORDEATH) || pPlayer->InCond(TF_COND_MARKEDFORDEATH_SILENT))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Marked", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_URINE))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Jarate", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_MAD_MILK))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Milk", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_STUNNED))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Stun", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_BURNING))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Burn", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_BLEEDING))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Bleed", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
			}

			// Misc
			if (Vars::ESP::Player.Value & Vars::ESP::PlayerEnum::Misc)
			{
				if (pPlayer->m_bFeignDeathReady())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Deadringer", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				else if (pPlayer->InCond(TF_COND_FEIGN_DEATH))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Feign", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_STEALTHED) || pPlayer->InCond(TF_COND_STEALTHED_BLINK) || pPlayer->InCond(TF_COND_STEALTHED_USER_BUFF) || pPlayer->InCond(TF_COND_STEALTHED_USER_BUFF_FADING))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, std::format("Invisible {:.0f}%%", pPlayer->GetInvisPercentage()), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				if (pPlayer->InCond(TF_COND_DISGUISING) || pPlayer->InCond(TF_COND_DISGUISED))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Disguise", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);


				if (pPlayer->InCond(TF_COND_AIMING) || pPlayer->InCond(TF_COND_ZOOMED))
				{
					switch (pWeapon ? pWeapon->GetWeaponID() : -1)
					{
					case TF_WEAPON_MINIGUN:
						tCache.m_vText.emplace_back(ESPTextEnum::Right, "Revved", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
						break;
					case TF_WEAPON_SNIPERRIFLE:
					case TF_WEAPON_SNIPERRIFLE_CLASSIC:
					case TF_WEAPON_SNIPERRIFLE_DECAP:
					{
						if (iIndex == I::EngineClient->GetLocalPlayer())
						{
							tCache.m_vText.emplace_back(ESPTextEnum::Right, std::format("Charging {:.0f}%%", Math::RemapVal(pWeapon->As<CTFSniperRifle>()->m_flChargedDamage(), 0.f, 150.f, 0.f, 100.f)), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
							break;
						}
						else
						{
							CSniperDot* pPlayerDot = nullptr;
							for (auto pDot : H::Entities.GetGroup(EGroupType::MISC_DOTS))
							{
								if (pDot->m_hOwnerEntity().Get() == pEntity)
								{
									pPlayerDot = pDot->As<CSniperDot>();
									break;
								}
							}
							if (pPlayerDot)
							{
								float flChargeTime = std::max(SDK::AttribHookValue(3.f, "mult_sniper_charge_per_sec", pWeapon), 1.5f);
								tCache.m_vText.emplace_back(ESPTextEnum::Right, std::format("Charging {:.0f}%%", Math::RemapVal(TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick) - pPlayerDot->m_flChargeStartTime() - 0.3f, 0.f, flChargeTime, 0.f, 100.f)), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
								break;
							}
						}
						tCache.m_vText.emplace_back(ESPTextEnum::Right, "Charging", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
						break;
					}
					case TF_WEAPON_COMPOUND_BOW:
						if (iIndex == I::EngineClient->GetLocalPlayer())
						{
							tCache.m_vText.emplace_back(ESPTextEnum::Right, std::format("Charging {:.0f}%%", Math::RemapVal(TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick) - pWeapon->As<CTFPipebombLauncher>()->m_flChargeBeginTime(), 0.f, 1.f, 0.f, 100.f)), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
							break;
						}
						tCache.m_vText.emplace_back(ESPTextEnum::Right, "Charging", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
						break;
					default:
						tCache.m_vText.emplace_back(ESPTextEnum::Right, "Charging", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
					}
				}

				if (pPlayer->InCond(TF_COND_SHIELD_CHARGE))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Charge", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

				if (Vars::Visuals::Removals::Taunts.Value && pPlayer->InCond(TF_COND_TAUNTING))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Taunt", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
			}
		}
	}
}

void CESP::StoreBuildings(CTFPlayer* pLocal)
{
	if (!(Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Buildings) || !Vars::ESP::Building.Value)
		return;

	for (auto pEntity : H::Entities.GetGroup(EGroupType::BUILDINGS_ALL))
	{
		auto pBuilding = pEntity->As<CBaseObject>();
		auto pOwner = pBuilding->m_hBuilder().Get();
		int iIndex = pOwner ? pOwner->entindex() : -1;

		if (pOwner)
		{
			if (iIndex == I::EngineClient->GetLocalPlayer())
			{
				if (!(Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Local))
					continue;
			}
			else
			{
				if (!(Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Prioritized && F::PlayerUtils.IsPrioritized(iIndex))
					&& !(Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Friends && H::Entities.IsFriend(iIndex))
					&& !(Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Party && H::Entities.InParty(iIndex))
					&& pOwner->m_iTeamNum() == pLocal->m_iTeamNum() ? !(Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Team) : !(Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Enemy))
					continue;
			}
		}
		else if (pEntity->m_iTeamNum() == pLocal->m_iTeamNum() ? !(Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Team) : !(Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Enemy))
			continue;

		bool bIsMini = pBuilding->m_bMiniBuilding();

		BuildingCache& tCache = m_mBuildingCache[pEntity];
		tCache.m_flAlpha = Vars::ESP::ActiveAlpha.Value / 255.f;
		tCache.m_tColor = H::Color.GetTeamColor(pLocal->m_iTeamNum(), (pOwner ? pOwner : pEntity)->m_iTeamNum(), Vars::Colors::Relative.Value);
		tCache.m_bBox = Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Box;

		if (Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Distance)
		{
			Vec3 vDelta = pEntity->m_vecOrigin() - pLocal->m_vecOrigin();
			tCache.m_vText.push_back({ ESPTextEnum::Bottom, std::format("[{:.0f}M]", vDelta.Length2D() / 41), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });
		}

		if (Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Name)
		{
			const char* szName = "Building";
			switch (pEntity->GetClassID())
			{
			case ETFClassID::CObjectSentrygun: szName = bIsMini ? "Mini-Sentry" : "Sentry"; break;
			case ETFClassID::CObjectDispenser: szName = "Dispenser"; break;
			case ETFClassID::CObjectTeleporter: szName = pBuilding->m_iObjectMode() ? "Teleporter Exit" : "Teleporter Entrance";
			}
			tCache.m_vText.push_back({ ESPTextEnum::Top, szName, H::Color.GetEntityNameColor(pLocal, (pOwner ? pOwner : pEntity), Vars::Colors::Relative.Value), Vars::Menu::Theme::Background.Value });
		}

		float flHealth = pBuilding->m_iHealth(), flMaxHealth = pBuilding->m_iMaxHealth();
		if (tCache.m_bHealthBar = Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::HealthBar)
			tCache.m_flHealth = std::clamp(flHealth / flMaxHealth, 0.f, 1.f);
		if (Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::HealthText)
			tCache.m_vText.push_back({ ESPTextEnum::Health, std::format("{}", flHealth), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });

		if (Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Owner && !pBuilding->m_bWasMapPlaced() && pOwner)
		{
			PlayerInfo_t pi{};
			if (I::EngineClient->GetPlayerInfo(iIndex, &pi))
				tCache.m_vText.push_back({ ESPTextEnum::Top, F::PlayerUtils.GetPlayerName(iIndex, pi.name), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });
		}

		if (Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Level && !bIsMini)
			tCache.m_vText.emplace_back(ESPTextEnum::Right, std::format("{} / {}", pBuilding->m_iUpgradeLevel(), bIsMini ? 1 : 3), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (Vars::ESP::Building.Value & Vars::ESP::BuildingEnum::Flags)
		{
			float flConstructed = pBuilding->m_flPercentageConstructed();
			if (flConstructed < 1.f)
				tCache.m_vText.emplace_back(ESPTextEnum::Right, std::format("{:.0f}%%", flConstructed * 100.f), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

			if (pBuilding->IsSentrygun() && pBuilding->As<CObjectSentrygun>()->m_bPlayerControlled())
				tCache.m_vText.emplace_back(ESPTextEnum::Right, "Wrangled", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);

			if (pBuilding->m_bHasSapper())
				tCache.m_vText.emplace_back(ESPTextEnum::Right, "Sapped", Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value);
			else if (pBuilding->m_bDisabled())
				tCache.m_vText.emplace_back(ESPTextEnum::Right, "Disabled", Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value);

			if (pBuilding->IsSentrygun() && !pBuilding->m_bBuilding())
			{
				int iShells, iMaxShells, iRockets, iMaxRockets; pBuilding->As<CObjectSentrygun>()->GetAmmoCount(iShells, iMaxShells, iRockets, iMaxRockets);
				if (!iShells)
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "No ammo", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				if (!bIsMini && !iRockets)
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "No rockets", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
			}
		}
	}
}

void CESP::StoreProjectiles(CTFPlayer* pLocal)
{
	if (!(Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Projectiles) || !Vars::ESP::Projectile.Value)
		return;

	for (auto pEntity : H::Entities.GetGroup(EGroupType::WORLD_PROJECTILES))
	{
		CBaseEntity* pOwner = nullptr;
		switch (pEntity->GetClassID())
		{
		case ETFClassID::CBaseProjectile:
		case ETFClassID::CBaseGrenade:
		case ETFClassID::CTFWeaponBaseGrenadeProj:
		case ETFClassID::CTFWeaponBaseMerasmusGrenade:
		case ETFClassID::CTFGrenadePipebombProjectile:
		case ETFClassID::CTFStunBall:
		case ETFClassID::CTFBall_Ornament:
		case ETFClassID::CTFProjectile_Jar:
		case ETFClassID::CTFProjectile_Cleaver:
		case ETFClassID::CTFProjectile_JarGas:
		case ETFClassID::CTFProjectile_JarMilk:
		case ETFClassID::CTFProjectile_SpellBats:
		case ETFClassID::CTFProjectile_SpellKartBats:
		case ETFClassID::CTFProjectile_SpellMeteorShower:
		case ETFClassID::CTFProjectile_SpellMirv:
		case ETFClassID::CTFProjectile_SpellPumpkin:
		case ETFClassID::CTFProjectile_SpellSpawnBoss:
		case ETFClassID::CTFProjectile_SpellSpawnHorde:
		case ETFClassID::CTFProjectile_SpellSpawnZombie:
		case ETFClassID::CTFProjectile_SpellTransposeTeleport:
		case ETFClassID::CTFProjectile_Throwable:
		case ETFClassID::CTFProjectile_ThrowableBreadMonster:
		case ETFClassID::CTFProjectile_ThrowableBrick:
		case ETFClassID::CTFProjectile_ThrowableRepel:
		{
			pOwner = pEntity->As<CTFWeaponBaseGrenadeProj>()->m_hThrower().Get();
			break;
		}
		case ETFClassID::CTFBaseRocket:
		case ETFClassID::CTFFlameRocket:
		case ETFClassID::CTFProjectile_Arrow:
		case ETFClassID::CTFProjectile_GrapplingHook:
		case ETFClassID::CTFProjectile_HealingBolt:
		case ETFClassID::CTFProjectile_Rocket:
		case ETFClassID::CTFProjectile_BallOfFire:
		case ETFClassID::CTFProjectile_MechanicalArmOrb:
		case ETFClassID::CTFProjectile_SentryRocket:
		case ETFClassID::CTFProjectile_SpellFireball:
		case ETFClassID::CTFProjectile_SpellLightningOrb:
		case ETFClassID::CTFProjectile_SpellKartOrb:
		case ETFClassID::CTFProjectile_EnergyBall:
		case ETFClassID::CTFProjectile_Flare:
		{
			auto pWeapon = pEntity->As<CTFBaseRocket>()->m_hLauncher().Get();
			pOwner = pWeapon ? pWeapon->As<CTFWeaponBase>()->m_hOwner().Get() : nullptr;
			break;
		}
		case ETFClassID::CTFBaseProjectile:
		case ETFClassID::CTFProjectile_EnergyRing:
			//case ETFClassID::CTFProjectile_Syringe:
		{
			auto pWeapon = pEntity->As<CTFBaseProjectile>()->m_hLauncher().Get();
			pOwner = pWeapon ? pWeapon->As<CTFWeaponBase>()->m_hOwner().Get() : nullptr;
			break;
		}
		}
		int iIndex = pOwner ? pOwner->entindex() : -1;

		if (pOwner)
		{
			if (iIndex == I::EngineClient->GetLocalPlayer())
			{
				if (!(Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Local))
					continue;
			}
			else
			{
				if (!(Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Prioritized && F::PlayerUtils.IsPrioritized(iIndex))
					&& !(Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Friends && H::Entities.IsFriend(iIndex))
					&& !(Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Party && H::Entities.InParty(iIndex))
					&& pOwner->m_iTeamNum() == pLocal->m_iTeamNum() ? !(Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Team) : !(Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Enemy))
					continue;
			}
		}
		else if (pEntity->m_iTeamNum() == pLocal->m_iTeamNum() ? !(Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Team) : !(Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Enemy))
			continue;

		WorldCache& tCache = m_mWorldCache[pEntity];
		tCache.m_flAlpha = Vars::ESP::ActiveAlpha.Value / 255.f;
		tCache.m_tColor = H::Color.GetTeamColor(pLocal->m_iTeamNum(), (pOwner ? pOwner : pEntity)->m_iTeamNum(), Vars::Colors::Relative.Value);
		tCache.m_bBox = Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Box;

		if (Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Distance)
		{
			Vec3 vDelta = pEntity->m_vecOrigin() - pLocal->m_vecOrigin();
			tCache.m_vText.push_back({ ESPTextEnum::Bottom, std::format("[{:.0f}M]", vDelta.Length2D() / 41), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });
		}

		if (Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Name)
		{
			const char* szName = "Projectile";
			switch (pEntity->GetClassID())
			{
				//case ETFClassID::CBaseProjectile:
				//case ETFClassID::CBaseGrenade:
				//case ETFClassID::CTFWeaponBaseGrenadeProj:
			case ETFClassID::CTFWeaponBaseMerasmusGrenade: szName = "Bomb"; break;
			case ETFClassID::CTFGrenadePipebombProjectile: szName = pEntity->As<CTFGrenadePipebombProjectile>()->HasStickyEffects() ? "Sticky" : "Pipe"; break;
			case ETFClassID::CTFStunBall: szName = "Baseball"; break;
			case ETFClassID::CTFBall_Ornament: szName = "Bauble"; break;
			case ETFClassID::CTFProjectile_Jar: szName = "Jarate"; break;
			case ETFClassID::CTFProjectile_Cleaver: szName = "Cleaver"; break;
			case ETFClassID::CTFProjectile_JarGas: szName = "Gas"; break;
			case ETFClassID::CTFProjectile_JarMilk:
			case ETFClassID::CTFProjectile_ThrowableBreadMonster: szName = "Milk"; break;
			case ETFClassID::CTFProjectile_SpellBats:
			case ETFClassID::CTFProjectile_SpellKartBats: szName = "Bats"; break;
			case ETFClassID::CTFProjectile_SpellMeteorShower: szName = "Meteor shower"; break;
			case ETFClassID::CTFProjectile_SpellMirv:
			case ETFClassID::CTFProjectile_SpellPumpkin: szName = "Pumpkin"; break;
			case ETFClassID::CTFProjectile_SpellSpawnBoss: szName = "Monoculus"; break;
			case ETFClassID::CTFProjectile_SpellSpawnHorde:
			case ETFClassID::CTFProjectile_SpellSpawnZombie: szName = "Skeleton"; break;
			case ETFClassID::CTFProjectile_SpellTransposeTeleport: szName = "Teleport"; break;
			//case ETFClassID::CTFProjectile_Throwable:
			//case ETFClassID::CTFProjectile_ThrowableBrick:
			//case ETFClassID::CTFProjectile_ThrowableRepel: szName = "Throwable"; break;
			case ETFClassID::CTFProjectile_Arrow: szName = pEntity->As<CTFProjectile_Arrow>()->m_iProjectileType() == TF_PROJECTILE_BUILDING_REPAIR_BOLT ? "Repair" : "Arrow"; break;
			case ETFClassID::CTFProjectile_GrapplingHook: szName = "Grapple"; break;
			case ETFClassID::CTFProjectile_HealingBolt: szName = "Heal"; break;
				//case ETFClassID::CTFBaseRocket:
				//case ETFClassID::CTFFlameRocket:
			case ETFClassID::CTFProjectile_Rocket:
			case ETFClassID::CTFProjectile_EnergyBall:
			case ETFClassID::CTFProjectile_SentryRocket: szName = "Rocket"; break;
			case ETFClassID::CTFProjectile_BallOfFire: szName = "Fire"; break;
			case ETFClassID::CTFProjectile_MechanicalArmOrb: szName = "Short circuit"; break;
			case ETFClassID::CTFProjectile_SpellFireball: szName = "Fireball"; break;
			case ETFClassID::CTFProjectile_SpellLightningOrb: szName = "Lightning"; break;
			case ETFClassID::CTFProjectile_SpellKartOrb: szName = "Fist"; break;
			case ETFClassID::CTFProjectile_Flare: szName = "Flare"; break;
				//case ETFClassID::CTFBaseProjectile:
			case ETFClassID::CTFProjectile_EnergyRing: szName = "Energy"; break;
				//case ETFClassID::CTFProjectile_Syringe: szName = "Syringe";
			}
			tCache.m_vText.push_back({ ESPTextEnum::Top, szName, H::Color.GetEntityNameColor(pLocal, (pOwner ? pOwner : pEntity), Vars::Colors::Relative.Value), Vars::Menu::Theme::Background.Value });
		}

		if (Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Owner && pOwner)
		{
			PlayerInfo_t pi{};
			if (I::EngineClient->GetPlayerInfo(iIndex, &pi))
				tCache.m_vText.emplace_back(ESPTextEnum::Top, F::PlayerUtils.GetPlayerName(iIndex, pi.name), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		}

		if (Vars::ESP::Projectile.Value & Vars::ESP::ProjectileEnum::Flags)
		{
			switch (pEntity->GetClassID())
			{
			case ETFClassID::CTFWeaponBaseGrenadeProj:
			case ETFClassID::CTFWeaponBaseMerasmusGrenade:
			case ETFClassID::CTFGrenadePipebombProjectile:
			case ETFClassID::CTFStunBall:
			case ETFClassID::CTFBall_Ornament:
			case ETFClassID::CTFProjectile_Jar:
			case ETFClassID::CTFProjectile_Cleaver:
			case ETFClassID::CTFProjectile_JarGas:
			case ETFClassID::CTFProjectile_JarMilk:
			case ETFClassID::CTFProjectile_SpellBats:
			case ETFClassID::CTFProjectile_SpellKartBats:
			case ETFClassID::CTFProjectile_SpellMeteorShower:
			case ETFClassID::CTFProjectile_SpellMirv:
			case ETFClassID::CTFProjectile_SpellPumpkin:
			case ETFClassID::CTFProjectile_SpellSpawnBoss:
			case ETFClassID::CTFProjectile_SpellSpawnHorde:
			case ETFClassID::CTFProjectile_SpellSpawnZombie:
			case ETFClassID::CTFProjectile_SpellTransposeTeleport:
			case ETFClassID::CTFProjectile_Throwable:
			case ETFClassID::CTFProjectile_ThrowableBreadMonster:
			case ETFClassID::CTFProjectile_ThrowableBrick:
			case ETFClassID::CTFProjectile_ThrowableRepel:
				if (pEntity->As<CTFWeaponBaseGrenadeProj>()->m_bCritical())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Crit", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				if (pEntity->As<CTFWeaponBaseGrenadeProj>()->m_iDeflected() && (pEntity->GetClassID() != ETFClassID::CTFGrenadePipebombProjectile || !pEntity->GetAbsVelocity().IsZero()))
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Reflected", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				break;
			case ETFClassID::CTFProjectile_Arrow:
			case ETFClassID::CTFProjectile_GrapplingHook:
			case ETFClassID::CTFProjectile_HealingBolt:
				if (pEntity->As<CTFProjectile_Arrow>()->m_bCritical())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Crit", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				if (pEntity->As<CTFBaseRocket>()->m_iDeflected())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Reflected", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				if (pEntity->As<CTFProjectile_Arrow>()->m_bArrowAlight())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Alight", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				break;
			case ETFClassID::CTFProjectile_Rocket:
			case ETFClassID::CTFProjectile_BallOfFire:
			case ETFClassID::CTFProjectile_MechanicalArmOrb:
			case ETFClassID::CTFProjectile_SentryRocket:
			case ETFClassID::CTFProjectile_SpellFireball:
			case ETFClassID::CTFProjectile_SpellLightningOrb:
			case ETFClassID::CTFProjectile_SpellKartOrb:
				if (pEntity->As<CTFProjectile_Rocket>()->m_bCritical())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Crit", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				if (pEntity->As<CTFBaseRocket>()->m_iDeflected())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Reflected", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				break;
			case ETFClassID::CTFProjectile_EnergyBall:
				if (pEntity->As<CTFProjectile_EnergyBall>()->m_bChargedShot())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Charge", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				if (pEntity->As<CTFBaseRocket>()->m_iDeflected())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Reflected", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				break;
			case ETFClassID::CTFProjectile_Flare:
				if (pEntity->As<CTFProjectile_Flare>()->m_bCritical())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Crit", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				if (pEntity->As<CTFBaseRocket>()->m_iDeflected())
					tCache.m_vText.emplace_back(ESPTextEnum::Right, "Reflected", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
				break;
			}
		}
	}
}

void CESP::StoreObjective(CTFPlayer* pLocal)
{
	if (!(Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Objective) || !Vars::ESP::Objective.Value)
		return;

	for (auto pEntity : H::Entities.GetGroup(EGroupType::WORLD_OBJECTIVE))
	{
		if (pEntity->m_iTeamNum() == pLocal->m_iTeamNum() ? !(Vars::ESP::Objective.Value & Vars::ESP::ObjectiveEnum::Team) : !(Vars::ESP::Objective.Value & Vars::ESP::ObjectiveEnum::Enemy))
			continue;

		WorldCache& tCache = m_mWorldCache[pEntity];
		tCache.m_flAlpha = Vars::ESP::ActiveAlpha.Value / 255.f;
		tCache.m_tColor = H::Color.GetTeamColor(pLocal->m_iTeamNum(), pEntity->m_iTeamNum(), Vars::Colors::Relative.Value);
		tCache.m_bBox = Vars::ESP::Objective.Value & Vars::ESP::ObjectiveEnum::Box;

		if (Vars::ESP::Objective.Value & Vars::ESP::ObjectiveEnum::Distance)
		{
			Vec3 vDelta = pEntity->m_vecOrigin() - pLocal->m_vecOrigin();
			tCache.m_vText.push_back({ ESPTextEnum::Bottom, std::format("[{:.0f}M]", vDelta.Length2D() / 41), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });
		}

		switch (pEntity->GetClassID())
		{
		case ETFClassID::CCaptureFlag:
		{
			auto pIntel = pEntity->As<CCaptureFlag>();

			if (Vars::ESP::Objective.Value & Vars::ESP::ObjectiveEnum::Name)
				tCache.m_vText.push_back({ ESPTextEnum::Top, "Intel", H::Color.GetEntityNameColor(pLocal, pEntity, Vars::Colors::Relative.Value), Vars::Menu::Theme::Background.Value });

			if (Vars::ESP::Objective.Value & Vars::ESP::ObjectiveEnum::Flags)
			{
				switch (pIntel->m_nFlagStatus())
				{
				case TF_FLAGINFO_HOME:
					tCache.m_vText.push_back({ ESPTextEnum::Right, "Home", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });
					break;
				case TF_FLAGINFO_DROPPED:
					tCache.m_vText.push_back({ ESPTextEnum::Right, "Dropped", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });
					break;
				default:
					tCache.m_vText.push_back({ ESPTextEnum::Right, "Stolen", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value });
				}
			}

			if (Vars::ESP::Objective.Value & Vars::ESP::ObjectiveEnum::IntelReturnTime && pIntel->m_nFlagStatus() == TF_FLAGINFO_DROPPED)
			{
				float flReturnTime = std::max(pIntel->m_flResetTime() - TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick), 0.f);
				tCache.m_vText.push_back({ ESPTextEnum::Right, std::format("RETURN {:.1f}s", pIntel->m_flResetTime() - TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick)).c_str(), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value });
			}

			break;
		}
		}
	}
}

void CESP::StoreWorld()
{
	const Color_t whiteColor = Color_t(255, 255, 255, 255);

	if (Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::NPCs)
	{
		for (auto pEntity : H::Entities.GetGroup(EGroupType::WORLD_NPC))
		{
			WorldCache& tCache = m_mWorldCache[pEntity];

			const char* szName = "NPC";
			switch (pEntity->GetClassID())
			{
			case ETFClassID::CEyeballBoss: szName = "Monoculus"; break;
			case ETFClassID::CHeadlessHatman: szName = "Horseless Headless Horsemann"; break;
			case ETFClassID::CMerasmus: szName = "Merasmus"; break;
			case ETFClassID::CTFBaseBoss: szName = "Boss"; break;
			case ETFClassID::CTFTankBoss: szName = "Tank"; break;
			case ETFClassID::CZombie: szName = "Skeleton"; break;
			}

			tCache.m_vText.push_back({ ESPTextEnum::Top, szName, whiteColor, Vars::Menu::Theme::Background.Value });
		}
	}

	if (Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Health)
	{
		for (auto pEntity : H::Entities.GetGroup(EGroupType::PICKUPS_HEALTH))
		{
			WorldCache& tCache = m_mWorldCache[pEntity];

			tCache.m_vText.push_back({ ESPTextEnum::Top, "Health", whiteColor, Vars::Menu::Theme::Background.Value });
		}
	}

	if (Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Ammo)
	{
		for (auto pEntity : H::Entities.GetGroup(EGroupType::PICKUPS_AMMO))
		{
			WorldCache& tCache = m_mWorldCache[pEntity];

			tCache.m_vText.push_back({ ESPTextEnum::Top, "Ammo", whiteColor, Vars::Menu::Theme::Background.Value });
		}
	}

	if (Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Money)
	{
		for (auto pEntity : H::Entities.GetGroup(EGroupType::PICKUPS_MONEY))
		{
			WorldCache& tCache = m_mWorldCache[pEntity];

			tCache.m_vText.push_back({ ESPTextEnum::Top, "Money", whiteColor, Vars::Menu::Theme::Background.Value });
		}
	}

	if (Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Powerups)
	{
		for (auto pEntity : H::Entities.GetGroup(EGroupType::PICKUPS_POWERUP))
		{
			WorldCache& tCache = m_mWorldCache[pEntity];

			const char* szName = "Powerup";
			switch (FNV1A::Hash32(I::ModelInfoClient->GetModelName(pEntity->GetModel())))
			{
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_agility.mdl"): szName = "Agility"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_crit.mdl"): szName = "Revenge"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_defense.mdl"): szName = "Resistance"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_haste.mdl"): szName = "Haste"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_king.mdl"): szName = "King"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_knockout.mdl"): szName = "Knockout"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_plague.mdl"): szName = "Plague"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_precision.mdl"): szName = "Precision"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_reflect.mdl"): szName = "Reflect"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_regen.mdl"): szName = "Regeneration"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_strength.mdl"): szName = "Strength"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_supernova.mdl"): szName = "Supernova"; break;
			case FNV1A::Hash32Const("models/pickups/pickup_powerup_vampire.mdl"): szName = "Vampire";
			}
			tCache.m_vText.push_back({ ESPTextEnum::Top, szName, whiteColor, Vars::Menu::Theme::Background.Value });
		}
	}

	if (Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Bombs)
	{
		for (auto pEntity : H::Entities.GetGroup(EGroupType::WORLD_BOMBS))
		{
			WorldCache& tCache = m_mWorldCache[pEntity];

			tCache.m_vText.push_back({ ESPTextEnum::Top, pEntity->GetClassID() == ETFClassID::CTFPumpkinBomb ? "Pumpkin Bomb" : "Bomb", whiteColor, Vars::Menu::Theme::Background.Value });
		}
	}

	if (Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Spellbook)
	{
		for (auto pEntity : H::Entities.GetGroup(EGroupType::PICKUPS_SPELLBOOK))
		{
			WorldCache& tCache = m_mWorldCache[pEntity];

			tCache.m_vText.push_back({ ESPTextEnum::Top, "Spellbook", whiteColor, Vars::Menu::Theme::Background.Value });
		}
	}

	if (Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Gargoyle)
	{
		for (auto pEntity : H::Entities.GetGroup(EGroupType::WORLD_GARGOYLE))
		{
			WorldCache& tCache = m_mWorldCache[pEntity];

			tCache.m_vText.push_back({ ESPTextEnum::Top, "Gargoyle", whiteColor, Vars::Menu::Theme::Background.Value });
		}
	}
}

void CESP::Draw()
{
	if (!Vars::ESP::Draw.Value)
		return;

	DrawWorld();
	DrawBuildings();
	DrawPlayers();
}

void CESP::DrawPlayers()
{
	if (!(Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Players) || !Vars::ESP::Player.Value)
		return;

	const auto& fFont = H::Fonts.GetFont(FONT_ESP);
	const int nTall = fFont.m_nTall;
	const auto& fFontSide = H::Fonts.GetFont(FONT_ESP_FLAG);
	const int nTallSide = fFontSide.m_nTall;

	for (auto& [pEntity, tCache] : m_mPlayerCache)
	{
		float x, y, w, h;
		if (!GetDrawBounds(pEntity, x, y, w, h))
			continue;

		int l = x - H::Draw.Scale(6), r = x + w + H::Draw.Scale(6), m = x + w / 2;
		int t = y - H::Draw.Scale(5), b = y + h + H::Draw.Scale(5);
		int lOffset = 0, rOffset = 0, bOffset = 0, tOffset = 0;

		I::MatSystemSurface->DrawSetAlphaMultiplier(tCache.m_flAlpha);

		if (tCache.m_bBox)
			H::Draw.LineRectOutline(x, y, w, h, tCache.m_tColor, { 0, 0, 0, 255 });

		if (tCache.m_bBones)
		{
			auto pPlayer = pEntity->As<CTFPlayer>();
			switch (H::Entities.GetModel(pPlayer->entindex()))
			{
			case FNV1A::Hash32Const("models/vsh/player/saxton_hale.mdl"):
				DrawBones(pPlayer, { HITBOX_SAXTON_HEAD, HITBOX_SAXTON_CHEST, HITBOX_SAXTON_PELVIS }, tCache.m_tColor);
				DrawBones(pPlayer, { HITBOX_SAXTON_CHEST, HITBOX_SAXTON_LEFT_UPPER_ARM, HITBOX_SAXTON_LEFT_FOREARM, HITBOX_SAXTON_LEFT_HAND }, tCache.m_tColor);
				DrawBones(pPlayer, { HITBOX_SAXTON_CHEST, HITBOX_SAXTON_RIGHT_UPPER_ARM, HITBOX_SAXTON_RIGHT_FOREARM, HITBOX_SAXTON_RIGHT_HAND }, tCache.m_tColor);
				DrawBones(pPlayer, { HITBOX_SAXTON_PELVIS, HITBOX_SAXTON_LEFT_THIGH, HITBOX_SAXTON_LEFT_CALF, HITBOX_SAXTON_LEFT_FOOT }, tCache.m_tColor);
				DrawBones(pPlayer, { HITBOX_SAXTON_PELVIS, HITBOX_SAXTON_RIGHT_THIGH, HITBOX_SAXTON_RIGHT_CALF, HITBOX_SAXTON_RIGHT_FOOT }, tCache.m_tColor);
				break;
			default:
				DrawBones(pPlayer, { HITBOX_HEAD, HITBOX_CHEST, HITBOX_PELVIS }, tCache.m_tColor);
				DrawBones(pPlayer, { HITBOX_CHEST, HITBOX_LEFT_UPPER_ARM, HITBOX_LEFT_FOREARM, HITBOX_LEFT_HAND }, tCache.m_tColor);
				DrawBones(pPlayer, { HITBOX_CHEST, HITBOX_RIGHT_UPPER_ARM, HITBOX_RIGHT_FOREARM, HITBOX_RIGHT_HAND }, tCache.m_tColor);
				DrawBones(pPlayer, { HITBOX_PELVIS, HITBOX_LEFT_THIGH, HITBOX_LEFT_CALF, HITBOX_LEFT_FOOT }, tCache.m_tColor);
				DrawBones(pPlayer, { HITBOX_PELVIS, HITBOX_RIGHT_THIGH, HITBOX_RIGHT_CALF, HITBOX_RIGHT_FOOT }, tCache.m_tColor);
			}
		}

		if (tCache.m_bHealthBar)
		{
			const int healthBarWidth = H::Draw.Scale(2);
			if (tCache.m_flHealth > 1.f)
			{
				Color_t cColor = { 255, 255, 255, 255 };
				H::Draw.FillRectPercent(x - H::Draw.Scale(6), y, healthBarWidth, h, 1.f, cColor, { 0, 0, 0, 255 }, ALIGN_BOTTOM, true);
			}
			else if (tCache.m_flHealth > 0.5f)
			{
				float midLerpFactor = (tCache.m_flHealth - 0.5f) * 2.f;
				Color_t cColor = Vars::Colors::IndicatorMid.Value.Lerp(Vars::Colors::IndicatorGood.Value, midLerpFactor);
				H::Draw.FillRectPercent(x - H::Draw.Scale(6), y, healthBarWidth, h, tCache.m_flHealth, cColor, { 0, 0, 0, 255 }, ALIGN_BOTTOM, true);
			}
			else
			{
				float lowLerpFactor = tCache.m_flHealth * 2.f;
				Color_t cColor = Vars::Colors::IndicatorBad.Value.Lerp(Vars::Colors::IndicatorMid.Value, lowLerpFactor);
				H::Draw.FillRectPercent(x - H::Draw.Scale(6), y, healthBarWidth, h, tCache.m_flHealth, cColor, { 0, 0, 0, 255 }, ALIGN_BOTTOM, true);
			}
			lOffset += H::Draw.Scale(6);
		}

		if (tCache.m_bUberBar)
		{
			const int uberBarHeight = H::Draw.Scale(2);
			H::Draw.FillRectPercent(x, y + h + H::Draw.Scale(4), w, uberBarHeight, tCache.m_flUber, Vars::Colors::IndicatorMisc.Value);
			bOffset += H::Draw.Scale(6);
		}

		int iVerticalOffset = H::Draw.Scale(3, Scale_Floor) - 1;
		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vText)
		{
			switch (iMode)
			{
			case ESPTextEnum::Top:
				H::Draw.String(fFont, m, t - tOffset, tColor, ALIGN_BOTTOM, sText.c_str());
				tOffset += nTall;
				break;
			case ESPTextEnum::Bottom:
				H::Draw.String(fFont, m, b + bOffset, tColor, ALIGN_TOP, sText.c_str());
				bOffset += nTall;
				break;
			case ESPTextEnum::Right:
				H::Draw.String(fFont, r, t + iVerticalOffset + rOffset, tColor, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nTall;
				break;
			case ESPTextEnum::Health:
			{
				const int healthBarWidthLocal = H::Draw.Scale(-5); // Define it here with same value as above
				int healthBarCenterX = x - H::Draw.Scale(6) + (healthBarWidthLocal / 2) - H::Draw.Scale(1); // Center minus 1-2 pixels
				int healthTextY = y + h - h * std::min(tCache.m_flHealth, 1.f); // Follow the health bar's height

				// Draw the text centered horizontally and aligned with the health bar's height
				H::Draw.String(fFont, healthBarCenterX, healthTextY, tColor, ALIGN_CENTER, sText.c_str());
			}
			break;
			case ESPTextEnum::Uber:
				H::Draw.String(fFont, r, y + h, tColor, ALIGN_TOPLEFT, sText.c_str());
			}
		}

		if (tCache.m_iClassIcon)
		{
			int size = H::Draw.Scale(18, Scale_Round);
			H::Draw.Texture(m, t - tOffset, size, size, tCache.m_iClassIcon - 1, ALIGN_BOTTOM);
		}

		if (tCache.m_pWeaponIcon)
		{
			float flW = tCache.m_pWeaponIcon->Width(), flH = tCache.m_pWeaponIcon->Height();
			float flScale = H::Draw.Scale(std::min((w + 40) / 2.f, 80.f) / std::max(flW, flH * 2));
			H::Draw.DrawHudTexture(m - flW / 2.f * flScale, b + bOffset, flScale, tCache.m_pWeaponIcon, Vars::Menu::Theme::Active.Value);
		}
	}

	I::MatSystemSurface->DrawSetAlphaMultiplier(1.f);
}

void CESP::DrawBuildings()
{
	if (!(Vars::ESP::Draw.Value & Vars::ESP::DrawEnum::Buildings) || !Vars::ESP::Building.Value)
		return;

	const auto& fFont = H::Fonts.GetFont(FONT_ESP);
	const int nTall = fFont.m_nTall + H::Draw.Scale(2);

	for (auto& [pEntity, tCache] : m_mBuildingCache)
	{
		float x, y, w, h;
		if (!GetDrawBounds(pEntity, x, y, w, h))
			continue;

		int l = x - H::Draw.Scale(6), r = x + w + H::Draw.Scale(6), m = x + w / 2;
		int t = y - H::Draw.Scale(5), b = y + h + H::Draw.Scale(5);
		int lOffset = 0, rOffset = 0, bOffset = 0, tOffset = 0;

		I::MatSystemSurface->DrawSetAlphaMultiplier(tCache.m_flAlpha);

		if (tCache.m_bBox)
			H::Draw.LineRectOutline(x, y, w, h, tCache.m_tColor, { 0, 0, 0, 255 });

		if (tCache.m_bHealthBar)
		{
			const int healthBarWidth = H::Draw.Scale(2);
			if (tCache.m_flHealth > 1.f)
			{
				Color_t cColor = { 255, 255, 255, 255 };
				H::Draw.FillRectPercent(x - H::Draw.Scale(6), y, healthBarWidth, h, 1.f, cColor, { 0, 0, 0, 255 }, ALIGN_BOTTOM, true);
			}
			else if (tCache.m_flHealth > 0.5f)
			{
				float midLerpFactor = (tCache.m_flHealth - 0.5f) * 2.f;
				Color_t cColor = Vars::Colors::IndicatorMid.Value.Lerp(Vars::Colors::IndicatorGood.Value, midLerpFactor);
				H::Draw.FillRectPercent(x - H::Draw.Scale(6), y, healthBarWidth, h, tCache.m_flHealth, cColor, { 0, 0, 0, 255 }, ALIGN_BOTTOM, true);
			}
			else
			{
				float lowLerpFactor = tCache.m_flHealth * 2.f;
				Color_t cColor = Vars::Colors::IndicatorBad.Value.Lerp(Vars::Colors::IndicatorMid.Value, lowLerpFactor);
				H::Draw.FillRectPercent(x - H::Draw.Scale(6), y, healthBarWidth, h, tCache.m_flHealth, cColor, { 0, 0, 0, 255 }, ALIGN_BOTTOM, true);
			}
			lOffset += H::Draw.Scale(6);
		}

		int iVerticalOffset = H::Draw.Scale(3, Scale_Floor) - 1;
		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vText)
		{
			switch (iMode)
			{
			case ESPTextEnum::Top:
				H::Draw.String(fFont, m, t - tOffset, tColor, ALIGN_BOTTOM, sText.c_str());
				tOffset += nTall;
				break;
			case ESPTextEnum::Bottom:
				H::Draw.String(fFont, m, b + bOffset, tColor, ALIGN_TOP, sText.c_str());
				bOffset += nTall;
				break;
			case ESPTextEnum::Right:
				H::Draw.String(fFont, r, t + iVerticalOffset + rOffset, tColor, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nTall;
				break;
			case ESPTextEnum::Health:
			{
				const int healthBarWidthLocal = H::Draw.Scale(-5); // Define it here with same value as above
				int healthBarCenterX = x - H::Draw.Scale(6) + (healthBarWidthLocal / 2) - H::Draw.Scale(1); // Center minus 1-2 pixels
				int healthTextY = y + h - h * std::min(tCache.m_flHealth, 1.f); // Follow the health bar's height

				// Draw the text centered horizontally and aligned with the health bar's height
				H::Draw.String(fFont, healthBarCenterX, healthTextY, tColor, ALIGN_CENTER, sText.c_str());
			}
			break;
			}
		}
	}

	I::MatSystemSurface->DrawSetAlphaMultiplier(1.f);
}

void CESP::DrawWorld()
{
	const auto& fFont = H::Fonts.GetFont(FONT_ESP);
	const int nTall = fFont.m_nTall + H::Draw.Scale(2);
	for (auto& [pEntity, tCache] : m_mWorldCache)
	{
		float x, y, w, h;
		if (!GetDrawBounds(pEntity, x, y, w, h))
			continue;

		int l = x - H::Draw.Scale(6), r = x + w + H::Draw.Scale(6), m = x + w / 2;
		int t = y - H::Draw.Scale(5), b = y + h + H::Draw.Scale(5);
		int lOffset = 0, rOffset = 0, bOffset = 0, tOffset = 0;

		I::MatSystemSurface->DrawSetAlphaMultiplier(tCache.m_flAlpha);

		if (tCache.m_bBox)
			H::Draw.LineRectOutline(x, y, w, h, tCache.m_tColor, { 0, 0, 0, 255 });

		int iVerticalOffset = H::Draw.Scale(3, Scale_Floor) - 1;
		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vText)
		{
			switch (iMode)
			{
			case ESPTextEnum::Top:
				H::Draw.String(fFont, m, t - tOffset, tColor, ALIGN_BOTTOM, sText.c_str());
				tOffset += nTall;
				break;
			case ESPTextEnum::Bottom:
				H::Draw.String(fFont, m, b + bOffset, tColor, ALIGN_TOP, sText.c_str());
				bOffset += nTall;
				break;
			case ESPTextEnum::Right:
				H::Draw.String(fFont, r, t + iVerticalOffset + rOffset, tColor, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nTall;
			}
		}
	}

	I::MatSystemSurface->DrawSetAlphaMultiplier(1.f);
}

Color_t CESP::GetColor(CTFPlayer* pLocal, CBaseEntity* pEntity)
{
	if (pEntity->entindex() == I::EngineClient->GetLocalPlayer())
		return Vars::Colors::Local.Value;
	if (pEntity->entindex() == G::AimTarget.m_iEntIndex)
		return Vars::Colors::Target.Value;
	return H::Color.GetTeamColor(pLocal->m_iTeamNum(), pEntity->m_iTeamNum(), Vars::Colors::Relative.Value);
}

bool CESP::GetDrawBounds(CBaseEntity* pEntity, float& x, float& y, float& w, float& h)
{
	Vec3 vOrigin = pEntity->GetAbsOrigin();
	matrix3x4 mTransform = { { 1, 0, 0, vOrigin.x }, { 0, 1, 0, vOrigin.y }, { 0, 0, 1, vOrigin.z } };
	//if (pEntity->entindex() == I::EngineClient->GetLocalPlayer())
		Math::AngleMatrix({ 0.f, I::EngineClient->GetViewAngles().y, 0.f }, mTransform, false);

	float flLeft, flRight, flTop, flBottom;
	if (!SDK::IsOnScreen(pEntity, mTransform, &flLeft, &flRight, &flTop, &flBottom))
		return false;

	x = flLeft;
	y = flBottom;
	w = flRight - flLeft;
	h = flTop - flBottom;

	switch (pEntity->GetClassID())
	{
	case ETFClassID::CTFPlayer:
	case ETFClassID::CObjectSentrygun:
	case ETFClassID::CObjectDispenser:
	case ETFClassID::CObjectTeleporter:
		x += w * 0.125f;
		w *= 0.75f;
	}

	return !(x > H::Draw.m_nScreenW || x + w < 0 || y > H::Draw.m_nScreenH || y + h < 0);
}

const char* CESP::GetPlayerClass(int iClassNum)
{
	static const char* szClasses[] = {
		"Unknown", "Scout", "Sniper", "Soldier", "Demoman", "Medic", "Heavy", "Pyro", "Spy", "Engineer"
	};

	return iClassNum < 10 && iClassNum > 0 ? szClasses[iClassNum] : szClasses[0];
}

void CESP::DrawBones(CTFPlayer* pPlayer, std::vector<int> vecBones, Color_t clr)
{
	for (size_t n = 1; n < vecBones.size(); n++)
	{
		const auto vBone1 = pPlayer->GetHitboxCenter(vecBones[n]);
		const auto vBone2 = pPlayer->GetHitboxCenter(vecBones[n - 1]);

		Vec3 vScreenBone, vScreenParent;
		if (SDK::W2S(vBone1, vScreenBone) && SDK::W2S(vBone2, vScreenParent))
			H::Draw.Line(vScreenBone.x, vScreenBone.y, vScreenParent.x, vScreenParent.y, clr);
	}
}