#include "Body.h"

#include <OverlayTypeClass.h>
#include <ScenarioClass.h>
#include <TerrainClass.h>

#include <Ext/Building/Body.h>
#include <Ext/Bullet/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Utilities/EnumFunctions.h>

#pragma region TechnoClass_SelectWeapon
DEFINE_HOOK(0x6F3339, TechnoClass_WhatWeaponShouldIUse_Interceptor, 0x8)
{
	enum { SkipGameCode = 0x6F3341, ReturnValue = 0x6F3406 };

	GET(TechnoClass*, pThis, ESI);
	GET_STACK(AbstractClass*, pTarget, STACK_OFFSET(0x18, 0x4));

	if (pThis && pTarget && pTarget->WhatAmI() == AbstractType::Bullet)
	{
		const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

		if (pTypeExt->InterceptorType)
		{
			R->EAX(pTypeExt->InterceptorType->Weapon);
			return ReturnValue;
		}
	}

	// Restore overridden instructions.
	R->EAX(pThis->GetTechnoType());

	return SkipGameCode;
}

DEFINE_HOOK(0x6F33CD, TechnoClass_WhatWeaponShouldIUse_ForceFire, 0x6)
{
	enum { ReturnWeaponIndex = 0x6F37AF };

	GET(TechnoClass*, pThis, ESI);
	GET_STACK(AbstractClass*, pTarget, STACK_OFFSET(0x18, 0x4));

	if (const auto pCell = abstract_cast<CellClass*>(pTarget))
	{
		auto const pWeaponPrimary = pThis->GetWeapon(0)->WeaponType;
		auto const pWeaponSecondary = pThis->GetWeapon(1)->WeaponType;
		auto const pPrimaryExt = WeaponTypeExt::ExtMap.Find(pWeaponPrimary);

		if (pWeaponSecondary && (!EnumFunctions::IsCellEligible(pCell, pPrimaryExt->CanTarget, true, true)
			|| (pPrimaryExt->AttachEffect_CheckOnFirer && !pPrimaryExt->HasRequiredAttachedEffects(pThis, pThis))))
		{
			R->EAX(1);
			return ReturnWeaponIndex;
		}
		else if (pCell->OverlayTypeIndex != -1)
		{
			auto const pOverlayType = OverlayTypeClass::Array()->GetItem(pCell->OverlayTypeIndex);

			if (pOverlayType->Wall && pCell->OverlayData >> 4 != pOverlayType->DamageLevels)
			{
				R->EAX(TechnoExt::GetWeaponIndexAgainstWall(pThis, pOverlayType));
				return ReturnWeaponIndex;
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x6F3428, TechnoClass_WhatWeaponShouldIUse_ForceWeapon, 0x6)
{
	enum { UseWeaponIndex = 0x6F37AF };

	GET(TechnoClass*, pThis, ECX);

	if (pThis && pThis->Target)
	{
		auto const pTarget = abstract_cast<TechnoClass*>(pThis->Target);

		if (!pTarget)
			return 0;

		int forceWeaponIndex = -1;
		auto const pTargetType = pTarget->GetTechnoType();
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

		if (pTypeExt->ForceWeapon_Naval_Decloaked >= 0 &&
			pTargetType->Cloakable && pTargetType->Naval &&
			pTarget->CloakState == CloakState::Uncloaked)
		{
			forceWeaponIndex = pTypeExt->ForceWeapon_Naval_Decloaked;
		}
		else if (pTypeExt->ForceWeapon_Cloaked >= 0 &&
			pTarget->CloakState == CloakState::Cloaked)
		{
			forceWeaponIndex = pTypeExt->ForceWeapon_Cloaked;
		}
		else if (pTypeExt->ForceWeapon_Disguised >= 0 &&
			pTarget->IsDisguised())
		{
			forceWeaponIndex = pTypeExt->ForceWeapon_Disguised;
		}

		if (forceWeaponIndex >= 0)
		{
			R->EAX(forceWeaponIndex);
			return UseWeaponIndex;
		}
	}

	return 0;
}

DEFINE_HOOK(0x6F36DB, TechnoClass_WhatWeaponShouldIUse, 0x8)
{
	GET(TechnoClass*, pThis, ESI);
	GET_STACK(AbstractClass*, pTarget, STACK_OFFSET(0x18, 0x4));

	enum { Primary = 0x6F37AD, Secondary = 0x6F3745, OriginalCheck = 0x6F36E3 };

	const auto pTargetTechno = abstract_cast<TechnoClass*>(pTarget);
	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	bool allowFallback = !pTypeExt->NoSecondaryWeaponFallback;
	bool allowAAFallback = allowFallback ? true : pTypeExt->NoSecondaryWeaponFallback_AllowAA;
	int weaponIndex = TechnoExt::PickWeaponIndex(pThis, pTargetTechno, pTarget, 0, 1, allowFallback, allowAAFallback);

	if (weaponIndex != -1)
		return weaponIndex == 1 ? Secondary : Primary;

	if (!pTargetTechno)
		return Primary;

	if (const auto pTargetExt = TechnoExt::ExtMap.Find(pTargetTechno))
	{
		if (const auto pShield = pTargetExt->Shield.get())
		{
			if (pShield->IsActive())
			{
				auto const secondary = pThis->GetWeapon(1)->WeaponType;
				auto const pSecondProjExt = BulletTypeExt::ExtMap.Find(secondary->Projectile);
				bool secondaryIsAA = pTargetTechno && pTargetTechno->IsInAir() && secondary && secondary->Projectile->AA;
				bool secondaryIsAU = pTargetTechno && pTargetTechno->InWhichLayer() == Layer::Underground && pSecondProjExt && pSecondProjExt->AU;

				if (secondary && (allowFallback || (allowAAFallback && secondaryIsAA) || secondaryIsAU || TechnoExt::CanFireNoAmmoWeapon(pThis, 1)))
				{
					if (!pShield->CanBeTargeted(pThis->GetWeapon(0)->WeaponType))
						return Secondary;
				}
				else
				{
					return Primary;
				}
			}
		}
	}

	return OriginalCheck;
}

DEFINE_HOOK(0x6F37EB, TechnoClass_WhatWeaponShouldIUse_AntiAir, 0x6)
{
	enum { Primary = 0x6F37AD, Secondary = 0x6F3807 };

	GET_STACK(AbstractClass*, pTarget, STACK_OFFSET(0x18, 0x4));
	GET_STACK(WeaponTypeClass*, pWeapon, STACK_OFFSET(0x18, -0x4));
	GET(WeaponTypeClass*, pSecWeapon, EAX);

	const auto pTargetTechno = abstract_cast<TechnoClass*>(pTarget);
	const auto pPrimaryProj = pWeapon->Projectile;
	const auto pSecondaryProj = pSecWeapon->Projectile;

	if (!pPrimaryProj->AA && pSecondaryProj->AA && pTargetTechno && pTargetTechno->IsInAir())
		return Secondary;

	const auto pPrimaryProjExt = BulletTypeExt::ExtMap.Find(pPrimaryProj);
	const auto pSecondaryProjExt = BulletTypeExt::ExtMap.Find(pSecondaryProj);

	if (!pPrimaryProjExt->AU && pSecondaryProjExt->AU && pTargetTechno && (pTargetTechno->InWhichLayer()==Layer::Underground))
		return Secondary;

	return Primary;
}

DEFINE_HOOK(0x6F3432, TechnoClass_WhatWeaponShouldIUse_Gattling, 0xA)
{
	enum { UseWeaponIndex = 0x6F37AF };

	GET(TechnoClass*, pThis, ESI);
	GET_STACK(AbstractClass*, pTarget, STACK_OFFSET(0x18, 0x4));

	auto const pTargetTechno = abstract_cast<TechnoClass*>(pTarget);
	int oddWeaponIndex = 2 * pThis->CurrentGattlingStage;
	int evenWeaponIndex = oddWeaponIndex + 1;
	int chosenWeaponIndex = oddWeaponIndex;
	int eligibleWeaponIndex = TechnoExt::PickWeaponIndex(pThis, pTargetTechno, pTarget, oddWeaponIndex, evenWeaponIndex, true);

	if (eligibleWeaponIndex != -1)
	{
		chosenWeaponIndex = eligibleWeaponIndex;
	}
	else if (pTargetTechno)
	{
		auto const pTargetExt = TechnoExt::ExtMap.Find(pTargetTechno);
		auto const pWeaponOdd = pThis->GetWeapon(oddWeaponIndex)->WeaponType;
		auto const pWeaponEven = pThis->GetWeapon(evenWeaponIndex)->WeaponType;
		bool skipRemainingChecks = false;

		if (const auto pShield = pTargetExt->Shield.get())
		{
			if (pShield->IsActive() && !pShield->CanBeTargeted(pWeaponOdd))
			{
				chosenWeaponIndex = evenWeaponIndex;
				skipRemainingChecks = true;
			}
		}

		if (!skipRemainingChecks)
		{
			if (GeneralUtils::GetWarheadVersusArmor(pWeaponOdd->Warhead, pTargetTechno->GetTechnoType()->Armor) == 0.0)
			{
				chosenWeaponIndex = evenWeaponIndex;
			}
			else
			{
				auto const pCell = pTargetTechno->GetCell();
				bool isOnWater = (pCell->LandType == LandType::Water || pCell->LandType == LandType::Beach) && !pTargetTechno->IsInAir() && !(pTargetTechno->InWhichLayer() == Layer::Underground);
				auto const pOddProjExt = BulletTypeExt::ExtMap.Find(pWeaponOdd->Projectile);
				auto const pEvenProjExt = BulletTypeExt::ExtMap.Find(pWeaponEven->Projectile);

				if (!pTargetTechno->OnBridge && isOnWater)
				{
					int navalTargetWeapon = pThis->SelectNavalTargeting(pTargetTechno);

					if (navalTargetWeapon == 2)
						chosenWeaponIndex = evenWeaponIndex;
				}
				else if ((pTargetTechno->IsInAir() && !pWeaponOdd->Projectile->AA && pWeaponEven->Projectile->AA) ||
					!pTargetTechno->IsInAir() && pThis->GetTechnoType()->LandTargeting == LandTargetingType::Land_Secondary ||
					pTargetTechno->InWhichLayer() == Layer::Underground && pOddProjExt && !pOddProjExt->AU && pEvenProjExt && pEvenProjExt->AU)
				{
					chosenWeaponIndex = evenWeaponIndex;
				}
			}
		}
	}

	R->EAX(chosenWeaponIndex);
	return UseWeaponIndex;
}

DEFINE_HOOK(0x5218F3, InfantryClass_WhatWeaponShouldIUse_DeployFireWeapon, 0x6)
{
	GET(TechnoTypeClass*, pType, ECX);

	if (pType->DeployFireWeapon == -1)
		return 0x52194E;

	return 0;
}

#pragma endregion

#pragma region TechnoClass_GetFireError
DEFINE_HOOK(0x6FC339, TechnoClass_CanFire, 0x6)
{
	enum { CannotFire = 0x6FCB7E };

	GET(TechnoClass*, pThis, ESI);
	GET(WeaponTypeClass*, pWeapon, EDI);
	GET_STACK(AbstractClass*, pTarget, STACK_OFFSET(0x20, 0x4));

	// Checking for nullptr is not required here, since the game has already executed them before calling the hook  -- Belonit
	const auto pWH = pWeapon->Warhead;

	if (const auto pWHExt = WarheadTypeExt::ExtMap.Find(pWH))
	{
		const int nMoney = pWHExt->TransactMoney;

		if (nMoney < 0 && pThis->Owner->Available_Money() < -nMoney)
			return CannotFire;
	}

	if (const auto pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon))
	{
		const auto pTechno = abstract_cast<TechnoClass*>(pTarget);
		CellClass* pTargetCell = nullptr;

		// AAOnly doesn't need to be checked if LandTargeting=1.
		if (pThis->GetTechnoType()->LandTargeting != LandTargetingType::Land_Not_OK && pWeapon->Projectile->AA && pTarget && !pTarget->IsInAir())
		{
			auto const pBulletTypeExt = BulletTypeExt::ExtMap.Find(pWeapon->Projectile);

			if (pBulletTypeExt->AAOnly)
				return CannotFire;
		}

		if (pTarget)
		{
			if (const auto pCell = abstract_cast<CellClass*>(pTarget))
			{
				pTargetCell = pCell;
			}
			else if (const auto pObject = abstract_cast<ObjectClass*>(pTarget))
			{
				// Ignore target cell for technos that are in air.
				if ((pTechno && !pTechno->IsInAir()) || pObject != pTechno)
					pTargetCell = pObject->GetCell();
			}
		}

		if (pTargetCell)
		{
			if (!EnumFunctions::IsCellEligible(pTargetCell, pWeaponExt->CanTarget, true, true))
				return CannotFire;
		}

		if (pTechno)
		{
			if (!EnumFunctions::IsTechnoEligible(pTechno, pWeaponExt->CanTarget) ||
				!EnumFunctions::CanTargetHouse(pWeaponExt->CanTargetHouses, pThis->Owner, pTechno->Owner))
			{
				return CannotFire;
			}

			if (!pWeaponExt->HasRequiredAttachedEffects(pTechno, pThis))
				return CannotFire;
		}
	}

	return 0;
}

DEFINE_HOOK(0x6FC0C5, TechnoClass_CanFire_DisableWeapons, 0x6)
{
	enum { OutOfRange = 0x6FC0DF, Illegal = 0x6FC86A, Continue = 0x6FC0D3 };

	GET(TechnoClass*, pThis, ESI);

	if (pThis->SlaveOwner)
		return Illegal;

	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	if (pExt->AE.DisableWeapons)
		return OutOfRange;

	return Continue;
}

DEFINE_HOOK(0x6FC5C7, TechnoClass_CanFire_OpenTopped, 0x6)
{
	enum { Illegal = 0x6FC86A, OutOfRange = 0x6FC0DF, Continue = 0x6FC5D5 };

	// GET(TechnoClass*, pThis, ESI);
	GET(TechnoClass*, pTransport, EAX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pTransport->GetTechnoType());

	if (pTransport->Deactivated && !pTypeExt->OpenTopped_AllowFiringIfDeactivated)
		return Illegal;

	if (pTransport->Transporter)
		return Illegal;

	if (pTypeExt->OpenTopped_CheckTransportDisableWeapons && TechnoExt::ExtMap.Find(pTransport)->AE.DisableWeapons)
		return OutOfRange;

	return Continue;
}

DEFINE_HOOK(0x6FC689, TechnoClass_CanFire_LandNavalTarget, 0x6)
{
	enum { DisallowFiring = 0x6FC86A };

	GET(TechnoClass*, pThis, ESI);
	GET_STACK(AbstractClass*, pTarget, STACK_OFFSET(0x20, 0x4));

	const auto pType = pThis->GetTechnoType();
	auto pCell = abstract_cast<CellClass*>(pTarget);

	if (pCell)
	{
		if (pType->NavalTargeting == NavalTargetingType::Naval_None &&
			(pCell->LandType == LandType::Water || pCell->LandType == LandType::Beach))
		{
			return DisallowFiring;
		}
	}
	else if (const auto pTerrain = abstract_cast<TerrainClass*>(pTarget))
	{
		pCell = pTerrain->GetCell();

		if (pType->LandTargeting == LandTargetingType::Land_Not_OK &&
			pCell->LandType != LandType::Water && pCell->LandType != LandType::Beach)
		{
			return DisallowFiring;
		}
		else if (pType->NavalTargeting == NavalTargetingType::Naval_None &&
			(pCell->LandType == LandType::Water || pCell->LandType == LandType::Beach))
		{
			return DisallowFiring;
		}
	}

	return 0;
}

DEFINE_HOOK(0x51C9B8, InfantryClass_CanFire_OnMoving1, 0x17)
{
	enum { CheckPass = 0x51C9CF, CheckNotPass = 0x51CAFA };

	GET(InfantryClass*, pThis, EBX);

	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->FiringByPassMovingCheck || pThis->SpeedPercentage <= 0.1) ? CheckPass : CheckNotPass;
}

DEFINE_HOOK(0x51CAAC, InfantryClass_CanFire_OnMoving2, 0x13)
{
	enum { CheckPass = 0x51CACD, CheckNotPass = 0x51CABF };

	GET(InfantryClass*, pThis, EBX);

	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->FiringByPassMovingCheck || !pThis->Locomotor->Is_Moving_Now()) ? CheckPass : CheckNotPass;
}

// Skips bridge-related coord checks to allow AA to target air units while both are on a bridge.
DEFINE_HOOK(0x6FCBE6, TechnoClass_CanFire_BridgeAAFix, 0x6)
{
	enum { SkipChecks = 0x6FCCBD };

	GET(TechnoClass*, pTarget, EBP);

	if (pTarget->IsInAir())
		return SkipChecks;

	return 0;
}

DEFINE_HOOK(0x6FC749, TechnoClass_GetFireError_AntiUnderground, 0x5)
{
	enum { Illegal = 0x6FC86A, GoOtherChecks = 0x6FC762 };

	GET(Layer, layer, EAX);
	//GET(TechnoClass*, pThis, EBX);
	GET(WeaponTypeClass*, pWeapon, EDI);

	auto const pProj = pWeapon->Projectile;
	auto const pProjExt = BulletTypeExt::ExtMap.Find(pProj);

	if (layer == Layer::Underground && !(pProjExt && pProjExt->AU))
		return Illegal;

	if ((layer == Layer::Air || layer == Layer::Top) && !pProj->AA)
		return Illegal;

	return GoOtherChecks;
}

#pragma endregion

#pragma region TechnoClass_Fire
DEFINE_HOOK(0x6FDD7D, TechnoClass_FireAt_UpdateWeaponType, 0x5)
{
	enum { CanNotFire = 0x6FDE03 };

	GET(WeaponTypeClass* const, pWeapon, EBX);
	GET(TechnoClass* const, pThis, ESI);

	if (TechnoExt::ExtData* const pExt = TechnoExt::ExtMap.Find(pThis))
	{
		if (pExt->TypeExtData->RecountBurst.Get(RulesExt::Global()->RecountBurst) && pWeapon != pExt->LastWeaponType)
		{
			if (pExt->LastWeaponType && pThis->CurrentBurstIndex)
			{
				const double ratio = static_cast<double>(pThis->CurrentBurstIndex) / pExt->LastWeaponType->Burst;
				const int rof = static_cast<int>(ratio * pExt->LastWeaponType->ROF * pExt->AE.ROFMultiplier);
				pThis->ChargeTurretDelay = rof;
				pThis->RearmTimer.Start(rof);
				pThis->CurrentBurstIndex = 0;
				pExt->LastWeaponType = pWeapon;

				return CanNotFire;
			}

			pThis->CurrentBurstIndex = 0;
		}

		pExt->LastWeaponType = pWeapon;
	}

	return 0;
}

DEFINE_HOOK(0x6FE43B, TechnoClass_FireAt_OpenToppedDmgMult, 0x8)
{
	enum { ApplyDamageMult = 0x6FE45A, ContinueCheck = 0x6FE460 };

	GET(TechnoClass* const, pThis, ESI);

	//replacing whole check due to `fild`
	if (pThis->InOpenToppedTransport)
	{
		GET_STACK(int, nDamage, STACK_OFFSET(0xB0, -0x84));
		float nDamageMult = static_cast<float>(RulesClass::Instance->OpenToppedDamageMultiplier);

		if (auto pTransport = pThis->Transporter)
		{
			if (auto pExt = TechnoTypeExt::ExtMap.Find(pTransport->GetTechnoType()))
			{
				//it is float isnt it YRPP ? , check tomson26 YR-IDB !
				nDamageMult = pExt->OpenTopped_DamageMultiplier.Get(nDamageMult);
			}
		}

		R->EAX(Game::F2I(nDamage * nDamageMult));
		return ApplyDamageMult;
	}

	return ContinueCheck;
}

DEFINE_HOOK(0x6FE19A, TechnoClass_FireAt_AreaFire, 0x6)
{
	enum { DoNotFire = 0x6FE4E7, SkipSetTarget = 0x6FE1D5 };

	GET(TechnoClass* const, pThis, ESI);
	GET(CellClass* const, pCell, EAX);
	GET_STACK(WeaponTypeClass*, pWeaponType, STACK_OFFSET(0xB0, -0x70));

	if (auto pExt = WeaponTypeExt::ExtMap.Find(pWeaponType))
	{
		if (pExt->AreaFire_Target == AreaFireTarget::Random)
		{
			auto const range = WeaponTypeExt::GetRangeWithModifiers(pWeaponType, pThis) / static_cast<double>(Unsorted::LeptonsPerCell);

			std::vector<CellStruct> adjacentCells = GeneralUtils::AdjacentCellsInRange(static_cast<size_t>(range + 0.99));
			size_t size = adjacentCells.size();

			for (unsigned int i = 0; i < size; i++)
			{
				int rand = ScenarioClass::Instance->Random.RandomRanged(0, size - 1);
				unsigned int cellIndex = (i + rand) % size;
				CellStruct tgtPos = pCell->MapCoords + adjacentCells[cellIndex];
				CellClass* tgtCell = MapClass::Instance->GetCellAt(tgtPos);
				bool allowBridges = tgtCell && tgtCell->ContainsBridge() && (pThis->OnBridge || tgtCell->Level + CellClass::BridgeLevels == pThis->GetCell()->Level);

				if (EnumFunctions::AreCellAndObjectsEligible(tgtCell, pExt->CanTarget, pExt->CanTargetHouses, pThis->Owner, true, false, allowBridges))
				{
					R->EAX(tgtCell);
					return 0;
				}
			}

			return DoNotFire;
		}
		else if (pExt->AreaFire_Target == AreaFireTarget::Self)
		{
			if (!EnumFunctions::AreCellAndObjectsEligible(pThis->GetCell(), pExt->CanTarget, pExt->CanTargetHouses, nullptr, false, false, pThis->OnBridge))
				return DoNotFire;

			R->EAX(pThis);
			return SkipSetTarget;
		}

		bool allowBridges = pCell && pCell->ContainsBridge() && (pThis->OnBridge || pCell->Level + CellClass::BridgeLevels == pThis->GetCell()->Level);

		if (!EnumFunctions::AreCellAndObjectsEligible(pCell, pExt->CanTarget, pExt->CanTargetHouses, nullptr, false, false, allowBridges))
			return DoNotFire;
	}

	return 0;
}

DEFINE_HOOK(0x6FF43F, TechnoClass_FireAt_FeedbackWeapon, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET(WeaponTypeClass*, pWeapon, EBX);

	if (auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon))
	{
		if (auto const pWeaponFeedback = pWeaponExt->FeedbackWeapon)
		{
			if (pThis->InOpenToppedTransport && !pWeaponFeedback->FireInTransport)
				return 0;

			WeaponTypeExt::DetonateAt(pWeaponFeedback, pThis, pThis);
		}
	}

	return 0;
}

DEFINE_HOOK(0x6FF905, TechnoClass_FireAt_FireOnce, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	if (auto const pInf = abstract_cast<InfantryClass*>(pThis))
	{
		GET(WeaponTypeClass*, pWeapon, EBX);
		auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);

		if (!pWeaponExt->FireOnce_ResetSequence)
			TechnoExt::ExtMap.Find(pInf)->SkipTargetChangeResetSequence = true;
	}

	return 0;
}

DEFINE_HOOK(0x6FF660, TechnoClass_FireAt_Interceptor, 0x6)
{
	GET(TechnoClass* const, pSource, ESI);
	GET_BASE(AbstractClass* const, pTarget, 0x8);
	GET_STACK(BulletClass* const, pBullet, STACK_OFFSET(0xB0, -0x74));

	auto const pSourceTypeExt = TechnoTypeExt::ExtMap.Find(pSource->GetTechnoType());

	if (pSourceTypeExt->InterceptorType)
	{
		if (auto const pTargetObject = specific_cast<BulletClass* const>(pTarget))
		{
			if (auto const pBulletExt = BulletExt::ExtMap.Find(pBullet))
			{
				pBulletExt->IsInterceptor = true;
				pBulletExt->InterceptedStatus = InterceptedStatus::Targeted;
			}
		}
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x6FF660, TechnoClass_FireAt_ToggleLaserWeaponIndex, 0x6)
DEFINE_HOOK(0x6FF4CC, TechnoClass_FireAt_ToggleLaserWeaponIndex, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(WeaponTypeClass* const, pWeapon, EBX);
	GET_BASE(int, weaponIndex, 0xC);

	if (pThis->WhatAmI() == AbstractType::Building && pWeapon->IsLaser)
	{
		if (auto const pExt = BuildingExt::ExtMap.Find(abstract_cast<BuildingClass*>(pThis)))
		{
			if (pExt->CurrentLaserWeaponIndex.empty())
				pExt->CurrentLaserWeaponIndex = weaponIndex;
			else
				pExt->CurrentLaserWeaponIndex.clear();
		}
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x6FE71C, TechnoClass_Fire_BallisticScatterPhobos, 0x6);
DEFINE_HOOK(0x6FE821, TechnoClass_Fire_BallisticScatterPhobos, 0x6)
{
	GET_STACK(BulletTypeClass*, pProjectile, 0x68);
	GET(TechnoClass*, pThis, ESI);
	GET_BASE(AbstractClass*, pTarget, 0x8);
	GET_STACK(WeaponTypeClass*, pWeapon, STACK_OFFSET(0xB0, -0x70));

	auto pExt = BulletTypeExt::ExtMap.Find(pProjectile);

	bool shouldScatter = ScenarioClass::Instance->Random.RandomRanged(0, 100) <= pExt->BallisticScatter_Chance * 100;

	if (!shouldScatter)
	{
		R->EAX(0);
		return 0;
	}

	// defaults for !FlakScatter || Inviso
	const int globalScatter = RulesClass::Instance->BallisticScatter;
	int min = pExt->BallisticScatter_Min.Get(Leptons(globalScatter / 2));
	int max = pExt->BallisticScatter_Max.Get(Leptons(globalScatter));

	if (pExt->BallisticScatter_IncreaseByRange)
	{
		const int minInMinRange = pExt->BallisticScatter_Min_InMinRange.Get(Leptons(min));
		const int minInMaxRange = pExt->BallisticScatter_Min_InMaxRange.Get(Leptons(min));
		const int maxInMinRange = pExt->BallisticScatter_Max_InMinRange.Get(Leptons(max));
		const int maxInMaxRange = pExt->BallisticScatter_Max_InMaxRange.Get(Leptons(max));
		const int minRange = pExt->BallisticScatter_MinRange.Get(Leptons(pWeapon->MinimumRange));
		const int maxRange = pExt->BallisticScatter_MaxRange.Get(Leptons(pWeapon->Range));
		const int deltaRange = maxRange - minRange;
		const int deltaRangeReal = static_cast<int>((pTarget->GetCoords() - pThis->GetCoords()).Magnitude()) - minRange;
		const double rangePercent = Math::clamp((deltaRange == 0 ? 0.5 : deltaRangeReal / static_cast<double>(deltaRange)), 0, 1);
		min = minInMinRange + static_cast<int>(rangePercent * (minInMaxRange - minInMinRange));
		max = maxInMinRange + static_cast<int>(rangePercent * (maxInMaxRange - maxInMinRange));
	}

	int scatter = ScenarioClass::Instance->Random.RandomRanged(min, max);

	R->EAX(scatter);
	return 0;
}

DEFINE_HOOK(0x6FF7EF, TechnoClass_Fire_LimboLaunchRecord, 0xA)
{
	enum { SkipGameCode = 0x6FF7F9 };

	GET(TechnoClass* const, pThis, ESI);
	GET_STACK(BulletClass* const, pBullet, STACK_OFFSET(0xB0, -0x74));

	if (!pThis->InLimbo)
	{
		auto const pBulletExt = BulletExt::ExtMap.Find(pBullet);
		pBulletExt->LimboedLauncher = pThis;
		pBulletExt->LimboedDir = pThis->PrimaryFacing.Current().GetDir();
	}

	pThis->Limbo();

	return SkipGameCode;
}

#pragma endregion

#pragma region TechnoClass_GetFLH
// Feature: Allow Units using AlternateFLHs - by Trsdy
// I don't want to rewrite something new, so I use the Infantry one directly
// afaik it has no check for infantry-specific stuff here so far
// and neither Ares nor Phobos has touched it, even that crawling flh one was in TechnoClass
DEFINE_JUMP(VTABLE, 0x7F5D20, 0x523250);// Redirect UnitClass::GetFLH to InfantryClass::GetFLH (used to be TechnoClass::GetFLH)

DEFINE_HOOK(0x6F3AF9, TechnoClass_GetFLH_AlternateFLH, 0x6)
{
	GET(TechnoClass*, pThis, EBX);
	GET(int, weaponIdx, ESI);

	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	weaponIdx = -weaponIdx - 1;

	const CoordStruct& flh =
		weaponIdx < static_cast<int>(pTypeExt->AlternateFLHs.size())
		? pTypeExt->AlternateFLHs[weaponIdx]
		: CoordStruct::Empty;

	R->ECX(flh.X);
	R->EBP(flh.Y);
	R->EAX(flh.Z);

	return 0x6F3B37;
}

namespace BurstFLHTemp
{
	bool FLHFound;
}

DEFINE_HOOK(0x6F3B37, TechnoClass_GetFLH_BurstFLH_1, 0x7)
{
	GET(TechnoClass*, pThis, EBX);
	GET(int, OriginalX, ECX);
	GET(int, OriginalY, EBP);
	GET(int, OriginalZ, EAX);
	GET_STACK(int, weaponIndex, STACK_OFFSET(0xD8, 0x8));

	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	if (!pExt)
		return 0;

	auto const pTypeExt = pExt->TypeExtData;

	if (weaponIndex < 0)
	{
		FootClass* currentPassenger = pThis->Passengers.FirstPassenger;
		const int passengerIndex = -weaponIndex - 1;

		for (int i = 0; i < passengerIndex && currentPassenger; i++)
			currentPassenger = abstract_cast<FootClass*>(currentPassenger->NextObject);

		if (auto const pPassengerExt = TechnoExt::ExtMap.Find(currentPassenger))
			pPassengerExt->LastWeaponFLH = { OriginalX, OriginalY, OriginalZ };

		return 0;
	}

	bool FLHFound = false;
	CoordStruct FLH = CoordStruct::Empty;

	FLH = TechnoExt::GetBurstFLH(pThis, weaponIndex, FLHFound, pTypeExt);
	BurstFLHTemp::FLHFound = FLHFound;

	if (!FLHFound)
	{
		if (auto pInf = abstract_cast<InfantryClass*>(pThis))
			FLH = TechnoExt::GetSimpleFLH(pInf, weaponIndex, FLHFound, pTypeExt);
	}

	if (FLHFound)
	{
		pExt->LastWeaponFLH = FLH;

		R->ECX(FLH.X);
		R->EBP(FLH.Y);
		R->EAX(FLH.Z);
	}
	else
	{
		pExt->LastWeaponFLH = { OriginalX, ((pThis->CurrentBurstIndex % 2 == 1) ? -OriginalY : OriginalY), OriginalZ };
	}

	return 0;
}

DEFINE_HOOK(0x6F3C88, TechnoClass_GetFLH_BurstFLH_2, 0x6)
{
	GET_STACK(int, weaponIndex, STACK_OFFSET(0xD8, 0x8));

	if (BurstFLHTemp::FLHFound || weaponIndex < 0)
		R->EAX(0);

	BurstFLHTemp::FLHFound = false;

	return 0;
}
#pragma endregion

// Basically a hack to make game and Ares pick laser properties from non-Primary weapons.
DEFINE_HOOK(0x70E1A0, TechnoClass_GetTurretWeapon_LaserWeapon, 0x5)
{
	enum { ReturnResult = 0x70E1C8 };

	GET(TechnoClass* const, pThis, ECX);

	if (auto const pBuilding = abstract_cast<BuildingClass*>(pThis))
	{
		auto const pExt = BuildingExt::ExtMap.Find(pBuilding);

		if (!pExt->CurrentLaserWeaponIndex.empty())
		{
			auto weaponStruct = pThis->GetWeapon(pExt->CurrentLaserWeaponIndex.get());
			R->EAX(weaponStruct);
			return ReturnResult;
		}
	}

	return 0;
}

DEFINE_HOOK(0x6FCFE0, TechnoClass_RearmDelay_CanCloakDuringRearm, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET(WeaponTypeClass*, pWeapon, EDI);

	TechnoExt::ExtMap.Find(pThis)->CanCloakDuringRearm = !pWeapon->DecloakToFire;

	return 0;
}

DEFINE_HOOK(0x6FD0B5, TechnoClass_RearmDelay_ROF, 0x6)
{
	enum { SkipGameCode = 0x6FD0BB };

	GET(TechnoClass*, pThis, ESI);
	GET(WeaponTypeClass*, pWeapon, EDI);

	auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);
	auto const pTechnoExt = TechnoExt::ExtMap.Find(pThis);
	auto range = pWeaponExt->ROF_RandomDelay.Get(RulesExt::Global()->ROF_RandomDelay);
	double rof = pWeapon->ROF * pTechnoExt->AE.ROFMultiplier;

	R->EAX(GeneralUtils::GetRangedRandomOrSingleValue(range));
	__asm { fld rof };

	return SkipGameCode;
}

DEFINE_HOOK(0x6FD054, TechnoClass_RearmDelay_ForceFullDelay, 0x6)
{
	enum { ApplyFullRearmDelay = 0x6FD09E };

	GET(TechnoClass*, pThis, ESI);

	// Currently only used with infantry, so a performance saving measure.
	if (pThis->WhatAmI() == AbstractType::Infantry)
	{
		if (const auto pExt = TechnoExt::ExtMap.Find(pThis))
		{
			if (pExt->ForceFullRearmDelay)
			{
				pExt->ForceFullRearmDelay = false;
				pThis->CurrentBurstIndex = 0;
				return ApplyFullRearmDelay;
			}
		}
	}

	return 0;
}

// Issue #271: Separate burst delay for weapon type
// Author: Starkku
DEFINE_HOOK(0x6FD05E, TechnoClass_RearmDelay_BurstDelays, 0x7)
{
	GET(TechnoClass*, pThis, ESI);
	GET(WeaponTypeClass*, pWeapon, EDI);

	const auto pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);
	int burstDelay = pWeaponExt->GetBurstDelay(pThis->CurrentBurstIndex);

	if (burstDelay >= 0)
	{
		R->EAX(burstDelay);
		return 0x6FD099;
	}

	// Restore overridden instructions
	GET(int, idxCurrentBurst, ECX);
	return idxCurrentBurst <= 0 || idxCurrentBurst > 4 ? 0x6FD084 : 0x6FD067;
}

// Update ammo rounds
DEFINE_HOOK(0x6FB086, TechnoClass_Reload_ReloadAmount, 0x8)
{
	GET(TechnoClass* const, pThis, ECX);

	TechnoExt::UpdateSharedAmmo(pThis);

	return 0;
}

namespace FiringAITemp
{
	int weaponIndex;
}

DEFINE_HOOK(0x5206D2, InfantryClass_FiringAI_SetContext, 0x6)
{
	GET(int, weaponIndex, EDI);

	FiringAITemp::weaponIndex = weaponIndex;

	return 0;
}

DEFINE_HOOK(0x5209AF, InfantryClass_FiringAI_BurstDelays, 0x6)
{
	enum { Continue = 0x5209CD, ReturnFromFunction = 0x520AD9 };

	GET(InfantryClass*, pThis, EBP);
	GET(int, firingFrame, EDX);

	int cumulativeDelay = 0;
	int projectedDelay = 0;
	int weaponIndex = FiringAITemp::weaponIndex;
	auto const pWeapon = pThis->GetWeapon(weaponIndex)->WeaponType;
	auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);

	// Calculate cumulative burst delay as well cumulative delay after next shot (projected delay).
	if (pWeaponExt && pWeaponExt->Burst_FireWithinSequence)
	{
		for (int i = 0; i <= pThis->CurrentBurstIndex; i++)
		{
			int burstDelay = pWeaponExt->GetBurstDelay(i);
			int delay = 0;

			if (burstDelay > -1)
				delay = burstDelay;
			else
				delay = ScenarioClass::Instance->Random.RandomRanged(3, 5);

			// Other than initial delay, treat 0 frame delays as 1 frame delay due to per-frame processing.
			if (i != 0)
				delay = Math::max(delay, 1);

			cumulativeDelay += delay;

			if (i == pThis->CurrentBurstIndex)
				projectedDelay = cumulativeDelay + delay;
		}
	}

	if (pThis->Animation.Value == firingFrame + cumulativeDelay)
	{
		if (pWeaponExt && pWeaponExt->Burst_FireWithinSequence)
		{
			int frameCount = pThis->Type->Sequence->GetSequence(pThis->SequenceAnim).CountFrames;

			// If projected frame for firing next shot goes beyond the sequence frame count, cease firing after this shot and start rearm timer.
			if (firingFrame + projectedDelay > frameCount)
			{
				auto const pExt = TechnoExt::ExtMap.Find(pThis);
				pExt->ForceFullRearmDelay = true;
			}
		}

		R->EAX(weaponIndex); // Reuse the weapon index to save some time.
		return Continue;
	}

	return ReturnFromFunction;
}

// Author: Otamaa
DEFINE_HOOK(0x5223B3, InfantryClass_Approach_Target_DeployFireWeapon, 0x6)
{
	GET(InfantryClass*, pThis, ESI);
	R->EDI(pThis->Type->DeployFireWeapon == -1 ? pThis->SelectWeapon(pThis->Target) : pThis->Type->DeployFireWeapon);
	return 0x5223B9;
}

#pragma region WallWeaponStuff

DEFINE_HOOK(0x70095A, TechnoClass_WhatAction_WallWeapon, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET_STACK(OverlayTypeClass*, pOverlayTypeClass, STACK_OFFSET(0x2C, -0x18));

	int weaponIndex = TechnoExt::GetWeaponIndexAgainstWall(pThis, pOverlayTypeClass);
	R->EAX(pThis->GetWeapon(weaponIndex));

	return 0;
}

DEFINE_HOOK(0x51C1F1, InfantryClass_CanEnterCell_WallWeapon, 0x5)
{
	enum { SkipGameCode = 0x51C1FE };

	GET(InfantryClass*, pThis, EBP);
	GET(OverlayTypeClass*, pOverlayTypeClass, ESI);

	R->EAX(pThis->GetWeapon(TechnoExt::GetWeaponIndexAgainstWall(pThis, pOverlayTypeClass)));

	return SkipGameCode;
}

DEFINE_HOOK(0x73F495, UnitClass_CanEnterCell_WallWeapon, 0x6)
{
	enum { SkipGameCode = 0x73F4A1 };

	GET(UnitClass*, pThis, EBX);
	GET(OverlayTypeClass*, pOverlayTypeClass, ESI);

	R->EAX(pThis->GetWeapon(TechnoExt::GetWeaponIndexAgainstWall(pThis, pOverlayTypeClass)));

	return SkipGameCode;
}

namespace CellEvalTemp
{
	int weaponIndex;
}

DEFINE_HOOK(0x6F8C9D, TechnoClass_EvaluateCell_SetContext, 0x7)
{
	GET(int, weaponIndex, EAX);

	CellEvalTemp::weaponIndex = weaponIndex;

	return 0;
}

WeaponStruct* __fastcall TechnoClass_EvaluateCellGetWeaponWrapper(TechnoClass* pThis)
{
	return pThis->GetWeapon(CellEvalTemp::weaponIndex);
}

int __fastcall TechnoClass_EvaluateCellGetWeaponRangeWrapper(TechnoClass* pThis, void* _, int weaponIndex)
{
	return pThis->GetWeaponRange(CellEvalTemp::weaponIndex);
}

DEFINE_JUMP(CALL6, 0x6F8CE3, GET_OFFSET(TechnoClass_EvaluateCellGetWeaponWrapper));
DEFINE_JUMP(CALL6, 0x6F8DD2, GET_OFFSET(TechnoClass_EvaluateCellGetWeaponRangeWrapper));

#pragma endregion

#pragma region EngineerAutoFire

DEFINE_HOOK(0x707E84, TechnoClass_GetGuardRange_Engineer, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return 0;
}

DEFINE_HOOK(0x6F8EF1, TechnoClass_SelectAutoTarget_Engineer, 0x6)
{
	enum { SkipGameCode = 0x6F8EF7 };

	GET(InfantryTypeClass* const, pType, EAX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	R->CL(pType->Engineer && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

DEFINE_HOOK(0x709249, TechnoClass_CanPassiveAcquireNow_Engineer1, 0xA)
{
	enum { SkipGameCode = 0x709253 };

	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

DEFINE_HOOK(0x6F8AEC, TechnoClass_TryAutoTargetObject_Engineer1, 0x6)
{
	enum { SkipGameCode = 0x6F8AF2 };

	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

DEFINE_HOOK(0x6F8BB2, TechnoClass_TryAutoTargetObject_Engineer2, 0x6)
{
	enum { SkipGameCode = 0x6F8BB8 };

	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

#pragma endregion

#pragma region NoBurstDelay

DEFINE_HOOK(0x5209EE, InfantryClass_UpdateFiring_BurstNoDelay, 0x5)
{
	enum { SkipVanillaFire = 0x520A57 };

	GET(InfantryClass* const, pThis, EBP);
	GET(const int, wpIdx, ESI);
	GET(AbstractClass* const, pTarget, EAX);

	if (const WeaponTypeClass* const pWeapon = pThis->GetWeapon(wpIdx)->WeaponType)
	{
		if (pWeapon->Burst > 1)
		{
			if (const WeaponTypeExt::ExtData* const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon))
			{
				if (pWeaponExt->Burst_NoDelay)
				{
					for (int i = 1; pThis->Fire(pTarget, wpIdx) && i != pWeapon->Burst; ++i)
					{
						const int rof = pThis->RearmTimer.TimeLeft;
						pThis->RearmTimer.Start(0);

						if (pThis->GetFireError(pTarget, wpIdx, true) != FireError::OK)
						{
							pThis->RearmTimer.Start(rof);
							break;
						}
					}

					return SkipVanillaFire;
				}
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x736F67, UnitClass_UpdateFiring_BurstNoDelay, 0x6)
{
	enum { SkipVanillaFire = 0x737063 };

	GET(UnitClass* const, pThis, ESI);
	GET(const int, wpIdx, EDI);
	GET(AbstractClass* const, pTarget, EAX);

	if (const WeaponTypeClass* const pWeapon = pThis->GetWeapon(wpIdx)->WeaponType)
	{
		if (pWeapon->Burst > 1)
		{
			if (const WeaponTypeExt::ExtData* const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon))
			{
				if (pWeaponExt->Burst_NoDelay)
				{
					for (int i = 1; pThis->Fire(pTarget, wpIdx) && i != pWeapon->Burst; ++i)
					{
						const int rof = pThis->RearmTimer.TimeLeft;
						pThis->RearmTimer.Start(0);

						if (pThis->GetFireError(pTarget, wpIdx, true) != FireError::OK)
						{
							pThis->RearmTimer.Start(rof);
							break;
						}
					}

					return SkipVanillaFire;
				}
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x44B630, BuildingClass_MissionAttack_AnimDelayedFire, 0x6)
{
	enum { JustFire = 0x44B6C4, VanillaCheck = 0 };

	GET(BuildingClass* const, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->AnimDontDelayBurst && pThis->CurrentBurstIndex != 0) ? JustFire : VanillaCheck;
}

#pragma endregion

#pragma region AttackWall

DEFINE_HOOK(0x6F8C18, TechnoClass_ScanToAttackWall_PlayerDestroyWall, 0x6)
{
	enum { SkipIsAIChecks = 0x6F8C52 };
	return RulesExt::Global()->PlayerDestroyWalls ? SkipIsAIChecks : 0;
}

DEFINE_HOOK(0x6F8D32, TechnoClass_ScanToAttackWall_DestroyOwnerlessWalls, 0x9)
{
	enum { GoOtherChecks = 0x6F8D58, NotOkToFire = 0x6F8DE3 };

	GET(int, OwnerIdx, EAX);
	GET(TechnoClass*, pThis, ESI);

	if (auto const pOwner = (OwnerIdx != -1) ? HouseClass::Array->Items[OwnerIdx] : nullptr)
	{
		if (pOwner->IsAlliedWith(pThis->Owner)
			&& (!RulesExt::Global()->DestroyOwnerlessWalls
			|| (pOwner != HouseClass::FindSpecial()
			&& pOwner != HouseClass::FindCivilianSide()
			&& pOwner != HouseClass::FindNeutral())))
		{
			return NotOkToFire;
		}
	}

	return GoOtherChecks;
}

#pragma endregion
