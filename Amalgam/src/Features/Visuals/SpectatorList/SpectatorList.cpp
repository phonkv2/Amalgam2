#include "SpectatorList.h"

#include "../../Players/PlayerUtils.h"
#include "../../Spectate/Spectate.h"

bool CSpectatorList::GetSpectators(CTFPlayer* pTarget)
{
	m_vSpectators.clear();

	for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ALL))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		int iIndex = pPlayer->entindex();
		bool bLocal = pEntity->entindex() == I::EngineClient->GetLocalPlayer();

		auto pObserverTarget = pPlayer->m_hObserverTarget().Get();
		int iObserverMode = pPlayer->m_iObserverMode();
		if (bLocal && F::Spectate.m_iTarget != -1)
		{
			pObserverTarget = F::Spectate.m_pOriginalTarget;
			iObserverMode = F::Spectate.m_iOriginalMode;
		}

		if (pPlayer->IsAlive() || pObserverTarget != pTarget
			|| bLocal && !I::EngineClient->IsPlayingDemo() && F::Spectate.m_iTarget == -1)
		{
			if (m_mRespawnCache.contains(pPlayer->entindex()))
				m_mRespawnCache.erase(pPlayer->entindex());
			continue;
		}

		std::string sMode;
		switch (iObserverMode)
		{
		case OBS_MODE_FIRSTPERSON: sMode = "1st"; break;
		case OBS_MODE_THIRDPERSON: sMode = "3rd"; break;
		default: continue;
		}

		float flRespawnTime = 0.f, flRespawnIn = 0.f;
		bool bRespawnTimeIncreased = false; // theoretically the respawn times could be changed by the map but oh well
		if (auto pResource = H::Entities.GetPR())
		{
			flRespawnTime = pResource->m_flNextRespawnTime(iIndex);
			flRespawnIn = std::max(flRespawnTime - TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick), 0.f);
		}
		if (!m_mRespawnCache.contains(iIndex))
			m_mRespawnCache[iIndex] = flRespawnTime;
		if (m_mRespawnCache[iIndex] + 0.9f < flRespawnTime)
		{
			bRespawnTimeIncreased = true;
			m_mRespawnCache[iIndex] = -1.f;
		}

		PlayerInfo_t pi{};
		if (I::EngineClient->GetPlayerInfo(iIndex, &pi))
		{
			std::string sName = F::PlayerUtils.GetPlayerName(iIndex, pi.name);
			m_vSpectators.emplace_back(sName, sMode, flRespawnIn, bRespawnTimeIncreased, H::Entities.IsFriend(pPlayer->entindex()), H::Entities.InParty(pPlayer->entindex()), pPlayer->entindex());
		}
	}

	return !m_vSpectators.empty();
}

void CSpectatorList::Draw(CTFPlayer* pLocal)
{
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::Spectators))
	{
		m_mRespawnCache.clear();
		return;
	}

	auto pTarget = pLocal;
	switch (pLocal->m_iObserverMode())
	{
	case OBS_MODE_FIRSTPERSON:
	case OBS_MODE_THIRDPERSON:
		pTarget = pLocal->m_hObserverTarget().Get()->As<CTFPlayer>();
	}
	PlayerInfo_t pi{};
	if (!pTarget || pTarget != pLocal && !I::EngineClient->GetPlayerInfo(pTarget->entindex(), &pi))
		return;

	if (!GetSpectators(pTarget))
		return;

	int x = Vars::Menu::SpectatorsDisplay.Value.x;
	int y = Vars::Menu::SpectatorsDisplay.Value.y + 8;
	int iconOffset = 0;
	const auto& fFont = H::Fonts.GetFont(FONT_INDICATORS);
	const int nTall = fFont.m_nTall + H::Draw.Scale(3);

	EAlign align = ALIGN_TOP;
	if (x <= 100 + H::Draw.Scale(50, Scale_Round))
	{
		x -= H::Draw.Scale(42, Scale_Round);
		align = ALIGN_TOPLEFT;
	}
	else if (x >= H::Draw.m_nScreenW - 100 + H::Draw.Scale(50, Scale_Round))
	{
		x += H::Draw.Scale(42, Scale_Round);
		align = ALIGN_TOPRIGHT;
	}

	std::string sName = pTarget != pLocal ? F::PlayerUtils.GetPlayerName(pTarget->entindex(), pi.name) : "You";
	H::Draw.String(fFont, x, y, Vars::Menu::Theme::Accent.Value, align, std::format("Spectating {}:", sName).c_str());
	for (auto& Spectator : m_vSpectators)
	{
		y += nTall;

		Color_t tColor = Vars::Menu::Theme::Active.Value;
		if (Spectator.m_bIsFriend)
			tColor = F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(FRIEND_TAG)].Color;
		else if (Spectator.m_bInParty)
			tColor = F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(PARTY_TAG)].Color;
		else if (Spectator.m_bRespawnTimeIncreased)
			tColor = F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(CHEATER_TAG)].Color;
		else if (FNV1A::Hash32(Spectator.m_sMode.c_str()) == FNV1A::Hash32Const("1st"))
			tColor = tColor.Lerp({ 255, 150, 0, 255 }, 0.5f);
		H::Draw.StringOutlined(fFont, x + iconOffset, y, tColor, Vars::Menu::Theme::Background.Value, align, std::format("{} ({} - respawn {}s)", Spectator.m_sName, Spectator.m_sMode, ceilf(Spectator.m_flRespawnIn)).c_str());
	}
}