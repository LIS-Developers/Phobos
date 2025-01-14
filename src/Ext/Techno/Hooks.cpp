#include <AircraftClass.h>
#include "Body.h"

#include <AircraftClass.h>
#include <EventClass.h>
#include <ScenarioClass.h>
#include <TunnelLocomotionClass.h>

#include <Ext/BuildingType/Body.h>
#include <Ext/House/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Utilities/EnumFunctions.h>

DEFINE_HOOK(0x6F9E50, TechnoClass_AI, 0x5)
{
	GET(TechnoClass*, pThis, ECX);

	// Do not search this up again in any functions called here because it is costly for performance - Starkku
	TechnoExt::ExtMap.Find(pThis)->OnEarlyUpdate();
	TechnoExt::ApplyMindControlRangeLimit(pThis);

	return 0;
}

// Ares-hook jmp to this offset
DEFINE_HOOK(0x71A88D, TemporalClass_AI, 0x0)
{
	GET(TemporalClass*, pThis, ESI);

	if (auto const pTarget = pThis->Target)
	{
		pTarget->IsMouseHovering = false;

		const auto pExt = TechnoExt::ExtMap.Find(pTarget);
		pExt->UpdateTemporal();
	}

	// Recovering vanilla instructions that were broken by a hook call
	return R->EAX<int>() <= 0 ? 0x71A895 : 0x71AB08;
}

DEFINE_HOOK_AGAIN(0x51BAC7, FootClass_AI_Tunnel, 0x6)//InfantryClass_AI_Tunnel
DEFINE_HOOK(0x7363B5, FootClass_AI_Tunnel, 0x6)//UnitClass_AI_Tunnel
{
	GET(FootClass*, pThis, ESI);

	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	pExt->UpdateOnTunnelEnter();

	return 0;
}

DEFINE_HOOK(0x6FA793, TechnoClass_AI_SelfHealGain, 0x5)
{
	enum { SkipGameSelfHeal = 0x6FA941 };

	GET(TechnoClass*, pThis, ESI);

	TechnoExt::ApplyGainedSelfHeal(pThis);

	return SkipGameSelfHeal;
}

DEFINE_HOOK(0x6F42F7, TechnoClass_Init, 0x2)
{
	GET(TechnoClass*, pThis, ESI);

	auto const pType = pThis->GetTechnoType();

	if (!pType)
		return 0;

	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	pExt->TypeExtData = TechnoTypeExt::ExtMap.Find(pType);

	pExt->CurrentShieldType = pExt->TypeExtData->ShieldType;
	pExt->InitializeLaserTrails();
	pExt->InitializeAttachEffects();
	pExt->InitializeDisplayInfo();
	pExt->InitializeUnitIdleAction();

	return 0;
}

DEFINE_HOOK_AGAIN(0x7355C0, TechnoClass_Init_InitialStrength, 0x6) // UnitClass_Init
DEFINE_HOOK_AGAIN(0x517D69, TechnoClass_Init_InitialStrength, 0x6) // InfantryClass_Init
DEFINE_HOOK_AGAIN(0x442C7B, TechnoClass_Init_InitialStrength, 0x6) // BuildingClass_Init
DEFINE_HOOK(0x414057, TechnoClass_Init_InitialStrength, 0x6)       // AircraftClass_Init
{
	GET(TechnoClass*, pThis, ESI);
	auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	if (R->Origin() != 0x517D69)
	{
		if (R->Origin() != 0x442C7B)
			R->EAX(pTypeExt->InitialStrength.Get(R->EAX<int>()));
		else
			R->ECX(pTypeExt->InitialStrength.Get(R->ECX<int>()));
	}
	else
	{
		auto strength = pTypeExt->InitialStrength.Get(R->EDX<int>());
		pThis->Health = strength;
		pThis->EstimatedHealth = strength;
	}

	return 0;
}

DEFINE_HOOK(0x702E4E, TechnoClass_RegisterDestruction_SaveKillerInfo, 0x6)
{
	GET(TechnoClass*, pKiller, EDI);
	GET(TechnoClass*, pVictim, ECX);

	if (pKiller && pVictim)
		TechnoExt::ObjectKilledBy(pVictim, pKiller);

	return 0;
}

DEFINE_HOOK(0x443C81, BuildingClass_ExitObject_InitialClonedHealth, 0x7)
{
	GET(BuildingClass*, pBuilding, ESI);
	if (auto const pInf = abstract_cast<InfantryClass*>(R->EDI<FootClass*>()))
	{
		if (pBuilding && pBuilding->Type->Cloning)
		{
			if (auto pTypeExt = BuildingTypeExt::ExtMap.Find(pBuilding->Type))
			{
				double percentage = GeneralUtils::GetRangedRandomOrSingleValue(pTypeExt->InitialStrength_Cloning);
				int strength = Math::clamp(static_cast<int>(pInf->Type->Strength * percentage), 1, pInf->Type->Strength);

				pInf->Health = strength;
				pInf->EstimatedHealth = strength;
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x702819, TechnoClass_ReceiveDamage_Decloak, 0xA)
{
	GET(TechnoClass* const, pThis, ESI);
	GET_STACK(WarheadTypeClass*, pWarhead, STACK_OFFSET(0xC4, 0xC));

	if (auto pExt = WarheadTypeExt::ExtMap.Find(pWarhead))
	{
		if (pExt->DecloakDamagedTargets)
			pThis->Uncloak(false);
	}

	return 0x702823;
}

DEFINE_HOOK(0x701DFF, TechnoClass_ReceiveDamage_FlyingStrings, 0x7)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(int* const, pDamage, EBX);

	if (Phobos::DisplayDamageNumbers && *pDamage)
		GeneralUtils::DisplayDamageNumberString(*pDamage, DamageDisplayType::Regular, pThis->GetRenderCoords(), TechnoExt::ExtMap.Find(pThis)->DamageNumberOffset);

	return 0;
}

DEFINE_HOOK(0x702603, TechnoClass_ReceiveDamage_Explodes, 0x6)
{
	enum { SkipExploding = 0x702672, SkipKillingPassengers = 0x702669 };

	GET(TechnoClass*, pThis, ESI);

	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	if (pThis->WhatAmI() == AbstractType::Building)
	{
		if (!pTypeExt->Explodes_DuringBuildup && (pThis->CurrentMission == Mission::Construction || pThis->CurrentMission == Mission::Selling))
			return SkipExploding;
	}

	if (!pTypeExt->Explodes_KillPassengers)
		return SkipKillingPassengers;

	return 0;
}

// Issue #237 NotHuman additional animations support
// Author: Otamaa
DEFINE_HOOK(0x518505, InfantryClass_ReceiveDamage_NotHuman, 0x4)
{
	GET(InfantryClass* const, pThis, ESI);
	REF_STACK(args_ReceiveDamage const, receiveDamageArgs, STACK_OFFSET(0xD0, 0x4));

	// Die1-Die5 sequences are offset by 10
	constexpr auto Die = [](int x) { return x + 10; };

	int resultSequence = Die(1);
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	if (pTypeExt->NotHuman_RandomDeathSequence.Get())
		resultSequence = ScenarioClass::Instance->Random.RandomRanged(Die(1), Die(5));

	if (receiveDamageArgs.WH)
	{
		if (auto const pWarheadExt = WarheadTypeExt::ExtMap.Find(receiveDamageArgs.WH))
		{
			int whSequence = pWarheadExt->NotHuman_DeathSequence.Get();
			if (whSequence > 0)
				resultSequence = Math::min(Die(whSequence), Die(5));
		}
	}

	R->ECX(pThis);
	pThis->PlayAnim(static_cast<Sequence>(resultSequence), true);

	return 0x518515;
}

DEFINE_HOOK(0x7098B9, TechnoClass_TargetSomethingNearby_AutoFire, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	if (auto pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType()))
	{
		if (pExt->AutoFire)
		{
			if (pExt->AutoFire_TargetSelf)
				pThis->SetTarget(pThis);
			else
				pThis->SetTarget(pThis->GetCell());

			return 0x7099B8;
		}
	}

	return 0;
}

DEFINE_HOOK(0x71067B, TechnoClass_EnterTransport_LaserTrails, 0x7)
{
	GET(TechnoClass*, pTechno, EDI);

	auto pTechnoExt = TechnoExt::ExtMap.Find(pTechno);

	if (pTechnoExt)
	{
		for (auto& trail : pTechnoExt->LaserTrails)
		{
			trail.Visible = false;
			trail.LastLocation = { };
		}
	}

	return 0;
}

// I don't think buildings should have laser-trails
DEFINE_HOOK(0x4D7221, FootClass_Unlimbo_LaserTrails, 0x6)
{
	GET(FootClass*, pTechno, ESI);

	if (auto pTechnoExt = TechnoExt::ExtMap.Find(pTechno))
	{
		for (auto& trail : pTechnoExt->LaserTrails)
		{
			trail.LastLocation = { };
			trail.Visible = true;
		}
	}

	return 0;
}

DEFINE_HOOK(0x4DEAEE, FootClass_IronCurtain_Organics, 0x6)
{
	GET(FootClass*, pThis, ESI);
	GET(TechnoTypeClass*, pType, EAX);
	GET_STACK(HouseClass*, pSource, STACK_OFFSET(0x10, 0x8));
	GET_STACK(bool, isForceShield, STACK_OFFSET(0x10, 0xC));

	enum { MakeInvulnerable = 0x4DEB38, SkipGameCode = 0x4DEBA2 };

	if (!pType->Organic && pThis->WhatAmI() != AbstractType::Infantry)
		return MakeInvulnerable;

	auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
	IronCurtainEffect icEffect = !isForceShield ? pTypeExt->IronCurtain_Effect.Get(RulesExt::Global()->IronCurtain_EffectOnOrganics) :
		pTypeExt->ForceShield_Effect.Get(RulesExt::Global()->ForceShield_EffectOnOrganics);

	switch (icEffect)
	{
	case IronCurtainEffect::Ignore:
		R->EAX(DamageState::Unaffected);
		break;
	case IronCurtainEffect::Invulnerable:
		return MakeInvulnerable;
		break;
	default:
		auto pWH = RulesClass::Instance->C4Warhead;

		if (!isForceShield)
			pWH = pTypeExt->IronCurtain_KillWarhead.Get(RulesExt::Global()->IronCurtain_KillOrganicsWarhead.Get(pWH));
		else
			pWH = pTypeExt->ForceShield_KillWarhead.Get(RulesExt::Global()->ForceShield_KillOrganicsWarhead.Get(pWH));

		R->EAX(pThis->ReceiveDamage(&pThis->Health, 0, pWH, nullptr, true, false, pSource));
		break;
	}

	return SkipGameCode;
}

DEFINE_JUMP(VTABLE, 0x7EB1AC, 0x4DEAE0); // Redirect InfantryClass::IronCurtain to FootClass::IronCurtain

namespace MapZoneTemp
{
	TargetZoneScanType zoneScanType;
}

DEFINE_HOOK(0x6F9C67, TechnoClass_GreatestThreat_MapZoneSetContext, 0x5)
{
	GET(TechnoClass*, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	MapZoneTemp::zoneScanType = pTypeExt->TargetZoneScanType;

	return 0;
}

DEFINE_HOOK(0x6F7E47, TechnoClass_EvaluateObject_MapZone, 0x7)
{
	enum { AllowedObject = 0x6F7EA2, DisallowedObject = 0x6F894F };

	GET(TechnoClass*, pThis, EDI);
	GET(ObjectClass*, pObject, ESI);
	GET(int, zone, EBP);

	if (auto const pTechno = abstract_cast<TechnoClass*>(pObject))
	{
		if (!TechnoExt::AllowedTargetByZone(pThis, pTechno, MapZoneTemp::zoneScanType, nullptr, true, zone))
			return DisallowedObject;
	}

	return AllowedObject;
}

DEFINE_HOOK(0x6F534E, TechnoClass_DrawExtras_Insignia, 0x5)
{
	enum { SkipGameCode = 0x6F5388 };

	GET(TechnoClass*, pThis, EBP);
	GET_STACK(Point2D*, pLocation, STACK_OFFSET(0x98, 0x4));
	GET(RectangleStruct*, pBounds, ESI);

	if (pThis->VisualCharacter(false, nullptr) != VisualType::Hidden)
	{
		if (RulesExt::Global()->DrawInsignia_OnlyOnSelected.Get() && !pThis->IsSelected && !pThis->IsMouseHovering)
			return SkipGameCode;
		else
			TechnoExt::DrawInsignia(pThis, pLocation, pBounds);
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x6F5EE3, TechnoClass_DrawExtras_DrawAboveHealth, 0x9)
{
	GET(TechnoClass*, pThis, EBP);
	GET_STACK(RectangleStruct*, pBounds, STACK_OFFSET(0x98, 0x8));

	CellClass* const pCell = MapClass::Instance->TryGetCellAt(pThis->GetCenterCoords());

	if ((pCell && !pCell->IsFogged() && !pCell->IsShrouded()) || pThis->IsSelected || pThis->IsMouseHovering)
	{
		const AbstractType absType = pThis->WhatAmI();

		if (absType == AbstractType::Building)
		{
			BuildingClass* const pBuilding = static_cast<BuildingClass*>(pThis);
			const Point2D basePosition = TechnoExt::GetBuildingSelectBracketPosition(pBuilding, BuildingSelectBracketPosition::Top);

			TechnoExt::DrawTemporalProgress(pThis, pBounds, basePosition, true, false);
			TechnoExt::DrawIronCurtainProgress(pThis, pBounds, basePosition, true, false);

			HouseClass* const pOwner = pThis->Owner;

			if (pOwner != HouseClass::FindSpecial() && pOwner != HouseClass::FindNeutral() && pOwner != HouseClass::FindCivilianSide())
			{
				TechnoExt::DrawSuperProgress(pBuilding, pBounds, basePosition);
				TechnoExt::DrawFactoryProgress(pBuilding, pBounds, basePosition);
			}
		}
		else
		{
			const bool isInfantry = absType == AbstractType::Infantry;
			const Point2D basePosition = TechnoExt::GetFootSelectBracketPosition(pThis, Anchor(HorizontalPosition::Left, VerticalPosition::Top));

			TechnoExt::DrawTemporalProgress(pThis, pBounds, basePosition, false, isInfantry);
			TechnoExt::DrawIronCurtainProgress(pThis, pBounds, basePosition, false, isInfantry);
		}
	}

	return 0;
}

DEFINE_HOOK(0x70EFE0, TechnoClass_GetMaxSpeed, 0x6)
{
	enum { SkipGameCode = 0x70EFF2 };

	GET(TechnoClass*, pThis, ECX);

	int maxSpeed = 0;

	if (pThis)
	{
		maxSpeed = pThis->GetTechnoType()->Speed;

		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

		if (pTypeExt->UseDisguiseMovementSpeed && pThis->IsDisguised())
		{
			if (auto const pType = TechnoTypeExt::GetTechnoType(pThis->Disguise))
				maxSpeed = pType->Speed;
		}
	}

	R->EAX(maxSpeed);
	return SkipGameCode;
}


DEFINE_HOOK(0x728F74, TunnelLocomotionClass_Process_KillAnims, 0x5)
{
	GET(ILocomotion*, pThis, ESI);

	const auto pLoco = static_cast<TunnelLocomotionClass*>(pThis);
	const auto pExt = TechnoExt::ExtMap.Find(pLoco->LinkedTo);
	pExt->IsBurrowed = true;

	if (const auto pShieldData = pExt->Shield.get())
		pShieldData->SetAnimationVisibility(false);

	for (auto const& attachEffect : pExt->AttachedEffects)
	{
		attachEffect->SetAnimationTunnelState(false);
	}

	return 0;
}

DEFINE_HOOK(0x728E5F, TunnelLocomotionClass_Process_RestoreAnims, 0x7)
{
	GET(ILocomotion*, pThis, ESI);

	const auto pLoco = static_cast<TunnelLocomotionClass*>(pThis);

	if (pLoco->State == TunnelLocomotionClass::State::PreDigOut)
	{
		const auto pExt = TechnoExt::ExtMap.Find(pLoco->LinkedTo);
		pExt->IsBurrowed = false;

		if (const auto pShieldData = pExt->Shield.get())
			pShieldData->SetAnimationVisibility(true);

		for (auto const& attachEffect : pExt->AttachedEffects)
		{
			attachEffect->SetAnimationTunnelState(true);
		}
	}

	return 0;
}

#pragma region Subterranean

DEFINE_HOOK(0x728F89, TunnelLocomotionClass_Process_SubterraneanHeight1, 0x5)
{
	enum { Skip = 0x728FA6, Continue = 0x728F90 };

	GET(TechnoClass*, pLinkedTo, ECX);
	GET(int, height, EAX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pLinkedTo->GetTechnoType());

	if (height == pTypeExt->SubterraneanHeight.Get(RulesExt::Global()->SubterraneanHeight))
		return Continue;

	return Skip;
}

DEFINE_HOOK(0x728FC6, TunnelLocomotionClass_Process_SubterraneanHeight2, 0x5)
{
	enum { Skip = 0x728FCD, Continue = 0x729021 };

	GET(TechnoClass*, pLinkedTo, ECX);
	GET(int, height, EAX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pLinkedTo->GetTechnoType());

	if (height <= pTypeExt->SubterraneanHeight.Get(RulesExt::Global()->SubterraneanHeight))
		return Continue;

	return Skip;
}

DEFINE_HOOK(0x728FF2, TunnelLocomotionClass_Process_SubterraneanHeight3, 0x6)
{
	enum { SkipGameCode = 0x72900C };

	GET(TechnoClass*, pLinkedTo, ECX);
	GET(int, heightOffset, EAX);
	REF_STACK(int, height, 0x14);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pLinkedTo->GetTechnoType());
	const int digInSpeed = pTypeExt->DigInSpeed;

	if (digInSpeed > 0)
		heightOffset = (int)(digInSpeed * TechnoExt::GetCurrentSpeedMultiplier((FootClass*)pLinkedTo));

	height -= heightOffset;
	const int subHeight = pTypeExt->SubterraneanHeight.Get(RulesExt::Global()->SubterraneanHeight);

	if (height < subHeight)
		height = subHeight;

	return SkipGameCode;
}

DEFINE_HOOK(0x7295E2, TunnelLocomotionClass_ProcessStateDigging_SubterraneanHeight, 0xC)
{
	enum { SkipGameCode = 0x7295EE };

	GET(TechnoClass*, pLinkedTo, EAX);
	REF_STACK(int, height, STACK_OFFSET(0x44, -0x8));

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pLinkedTo->GetTechnoType());
	height = pTypeExt->SubterraneanHeight.Get(RulesExt::Global()->SubterraneanHeight);

	return SkipGameCode;
}

DEFINE_HOOK(0x7295C5, TunnelLocomotionClass_ProcessDigging_DiggingSpeed, 0x9)
{
	enum { Move = 0x7298C7, ShouldStop = 0x7295CE };

	GET(int, deltaRange, EAX);
	GET(TunnelLocomotionClass* const, pThis, ESI);

	auto const pTechno = pThis->LinkedTo;
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pTechno->GetTechnoType());
	const double speed = TechnoExt::GetCurrentSpeedMultiplier(pTechno) * (pTypeExt ? pTypeExt->DiggingSpeed : 19.0);

	if (deltaRange < static_cast<int>(speed) + 1)
		return ShouldStop;

	CoordStruct currentCrd = pTechno->Location;
	CoordStruct targetCrd = pThis->Coords;
	int newCrdX = (int)(currentCrd.X + speed * (targetCrd.X - currentCrd.X) / (double)deltaRange);
	int newCrdY = (int)(currentCrd.Y + speed * (targetCrd.Y - currentCrd.Y) / (double)deltaRange);

	R->EAX(newCrdX);
	R->EDX(newCrdY);
	R->EDI(currentCrd.Z);
	return Move;
}

DEFINE_HOOK(0x7292BF, TunnelLocomotionClass_ProcessPreDigIn_DigStartROT, 0x6)
{
	GET(TunnelLocomotionClass* const, pThis, ESI);
	GET(int, time, EAX);

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->LinkedTo->GetTechnoType()))
	{
		const int rot = pTypeExt->DigStartROT;

		if (rot > 0)
			time = (int)(64 / (double)rot);
	}

	R->EAX(time);
	return 0;
}

DEFINE_HOOK(0x729A65, TunnelLocomotionClass_ProcessPreDigOut_DigEndROT, 0x6)
{
	GET(TunnelLocomotionClass* const, pThis, ESI);
	GET(int, time, EAX);

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->LinkedTo->GetTechnoType()))
	{
		const int rot = pTypeExt->DigEndROT;

		if (rot > 0)
			time = (int)(64 / (double)rot);
	}

	R->EAX(time);
	return 0;
}

DEFINE_HOOK(0x729969, TunnelLocomotionClass_ProcessPreDigOut_DigOutSpeed, 0x6)
{
	GET(TunnelLocomotionClass* const, pThis, ESI);
	GET(int, speed, EAX);

	auto const pTechno = pThis->LinkedTo;

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pTechno->GetTechnoType()))
	{
		const int digOutSpeed = pTypeExt->DigOutSpeed;

		if (digOutSpeed > 0)
			speed = (int)(digOutSpeed * TechnoExt::GetCurrentSpeedMultiplier(pTechno));
	}

	R->EAX(speed);
	return 0;
}

#pragma endregion

#pragma region Fly Layer Update

// Update attached anim layers after parent unit changes layer.
void __fastcall DisplayClass_Submit_Wrapper(DisplayClass* pThis, void* _, ObjectClass* pObject)
{
	pThis->Submit(pObject);

	if (auto const pTechno = abstract_cast<TechnoClass*>(pObject))
		TechnoExt::UpdateAttachedAnimLayers(pTechno);
}

DEFINE_JUMP(CALL, 0x54B18E, GET_OFFSET(DisplayClass_Submit_Wrapper));  // JumpjetLocomotionClass_Process
DEFINE_JUMP(CALL, 0x4CD4E7, GET_OFFSET(DisplayClass_Submit_Wrapper));  // FlyLocomotionClass_Update

#pragma endregion

// Move to UnitClass hooks file if it is ever created.
DEFINE_HOOK(0x736234, UnitClass_ChronoSparkleDelay, 0x5)
{
	R->ECX(RulesExt::Global()->ChronoSparkleDisplayDelay);
	return 0x736239;
}

// Move to InfantryClass hooks file if it is ever created.
DEFINE_HOOK(0x51BAFB, InfantryClass_ChronoSparkleDelay, 0x5)
{
	R->ECX(RulesExt::Global()->ChronoSparkleDisplayDelay);
	return 0x51BB00;
}

DEFINE_HOOK_AGAIN(0x5F4718, ObjectClass_Select, 0x7)
DEFINE_HOOK(0x5F46AE, ObjectClass_Select, 0x7)
{
	GET(ObjectClass*, pThis, ESI);

	pThis->IsSelected = true;

	if (RulesExt::Global()->SelectionFlashDuration > 0 && pThis->GetOwningHouse()->IsControlledByCurrentPlayer())
		pThis->Flash(RulesExt::Global()->SelectionFlashDuration);

	return 0;
}

DEFINE_HOOK(0x708FC0, TechnoClass_ResponseMove_Pickup, 0x5)
{
	enum { SkipResponse = 0x709015 };

	GET(TechnoClass*, pThis, ECX);

	if (auto const pAircraft = abstract_cast<AircraftClass*>(pThis))
	{
		if (pAircraft->Type->Carryall && pAircraft->HasAnyLink() &&
			generic_cast<FootClass*>(pAircraft->Destination))
		{
			auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pAircraft->Type);

			if (pTypeExt->VoicePickup.isset())
			{
				pThis->QueueVoice(pTypeExt->VoicePickup.Get());

				R->EAX(1);
				return SkipResponse;
			}
		}
	}

	return 0;
}

FireError __fastcall TechnoClass_TargetSomethingNearby_CanFire_Wrapper(TechnoClass* pThis, void* _, AbstractClass* pTarget, int weaponIndex, bool ignoreRange)
{
	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	bool disableWeapons = pExt->AE.DisableWeapons;
	pExt->AE.DisableWeapons = false;
	auto const fireError = pThis->GetFireError(pTarget, weaponIndex, ignoreRange);
	pExt->AE.DisableWeapons = disableWeapons;
	return fireError;
}

DEFINE_JUMP(CALL6, 0x7098E6, GET_OFFSET(TechnoClass_TargetSomethingNearby_CanFire_Wrapper));

// Can't hook where unit promotion happens in vanilla because of Ares - Fryone, Kerbiter
DEFINE_HOOK(0x6F9FA9, TechnoClass_AI_PromoteAnim, 0x6)
{
	GET(TechnoClass*, pThis, ECX);

	auto aresProcess = [pThis]() { return (pThis->GetTechnoType()->Turret) ? 0x6F9FB7 : 0x6FA054; };

	if (!RulesExt::Global()->Promote_VeteranAnimation && !RulesExt::Global()->Promote_EliteAnimation)
		return aresProcess();

	if (pThis->CurrentRanking != pThis->Veterancy.GetRemainingLevel() && pThis->CurrentRanking != Rank::Invalid && (pThis->Veterancy.GetRemainingLevel() != Rank::Rookie))
	{
		AnimClass* promAnim = nullptr;
		if (pThis->Veterancy.GetRemainingLevel() == Rank::Veteran && RulesExt::Global()->Promote_VeteranAnimation)
			promAnim = GameCreate<AnimClass>(RulesExt::Global()->Promote_VeteranAnimation, pThis->GetCenterCoords());
		else if (RulesExt::Global()->Promote_EliteAnimation)
			promAnim = GameCreate<AnimClass>(RulesExt::Global()->Promote_EliteAnimation, pThis->GetCenterCoords());
		promAnim->SetOwnerObject(pThis);
	}

	return aresProcess();
}

// TunnelLocomotionClass_IsToHaveShadow, skip shadow on all but idle.
// TODO: Investigate if it is possible to fix the shadows not tilting on the burrowing etc. states.
DEFINE_JUMP(LJMP, 0x72A070, 0x72A07F);

#pragma region KeepTargetOnMove

// Do not explicitly reset target for KeepTargetOnMove vehicles when issued move command.
DEFINE_HOOK(0x4C7462, EventClass_Execute_KeepTargetOnMove, 0x5)
{
	enum { SkipGameCode = 0x4C74C0 };

	GET(EventClass*, pThis, ESI);
	GET(TechnoClass*, pTechno, EDI);
	GET(AbstractClass*, pTarget, EBX);

	if (pTechno->WhatAmI() != AbstractType::Unit)
		return 0;

	auto const mission = static_cast<Mission>(pThis->MegaMission.Mission);
	auto const pExt = TechnoExt::ExtMap.Find(pTechno);

	if ((mission == Mission::Move)
		&& TechnoTypeExt::ExtMap.Find(pTechno->GetTechnoType())->KeepTargetOnMove
		&& pTechno->Target && !pTarget)
	{
		auto const pDestination = pThis->MegaMission.Destination.As_Abstract();
		pTechno->SetDestination(pDestination, true);

		pExt->KeepTargetOnMove = true;

		return SkipGameCode;
	}

	pExt->KeepTargetOnMove = false;

	return 0;
}

// Reset the target if beyond weapon range.
// This was originally in UnitClass::Mission_Move() but because that
// is only checked every ~15 frames, it can cause responsiveness issues.
DEFINE_HOOK(0x736480, UnitClass_AI_KeepTargetOnMove, 0x6)
{
	GET(UnitClass*, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	if (pTypeExt->KeepTargetOnMove && pThis->Target
		&& pThis->CurrentMission == Mission::Move && TechnoExt::ExtMap.Find(pThis)->KeepTargetOnMove)
	{
		const int weaponIndex = pThis->SelectWeapon(pThis->Target);

		if (auto const pWeapon = pThis->GetWeapon(weaponIndex)->WeaponType)
		{
			const int extraDistance = static_cast<int>(pTypeExt->KeepTargetOnMove_ExtraDistance.Get());
			const int range = pWeapon->Range;
			pWeapon->Range += extraDistance; // Temporarily adjust weapon range based on the extra distance.

			if (!pThis->IsCloseEnough(pThis->Target, weaponIndex))
				pThis->SetTarget(nullptr);

			pWeapon->Range = range;
		}
	}

	return 0;
}

#pragma endregion

DEFINE_HOOK(0x51B20E, InfantryClass_AssignTarget_FireOnce, 0x6)
{
	enum { SkipGameCode = 0x51B255 };

	GET(InfantryClass*, pThis, ESI);
	GET(AbstractClass*, pTarget, EBX);

	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	if (!pTarget && pExt->SkipTargetChangeResetSequence)
	{
		pThis->IsFiring = false;
		pExt->SkipTargetChangeResetSequence = false;
		return SkipGameCode;
	}

	return 0;
}

#pragma region DestroyAnimGeneralize

namespace PlayDestroyAnimGeneralize
{
	TechnoTypeClass* pType;
}

DEFINE_HOOK(0x738687, UnitClass_PlayDestroyAnim_SetContext, 0x6)
{
	enum { SkipGameCode = 0x73868D };

	GET(TechnoClass*, pThis, ESI);

	PlayDestroyAnimGeneralize::pType = pThis->GetTechnoType();
	R->EAX(PlayDestroyAnimGeneralize::pType);

	return SkipGameCode;
}

DEFINE_HOOK_AGAIN(0x738822, UnitClass_PlayDestroyAnim_Generalize1, 0x6);
DEFINE_HOOK_AGAIN(0x7387C4, UnitClass_PlayDestroyAnim_Generalize1, 0x6);
DEFINE_HOOK(0x7386AC, UnitClass_PlayDestroyAnim_Generalize1, 0x6)
{
	R->ECX(PlayDestroyAnimGeneralize::pType);
	return R->Origin() + 6;
}

DEFINE_HOOK_AGAIN(0x738801, UnitClass_PlayDestroyAnim_Generalize2, 0x6);
DEFINE_HOOK(0x7386DA, UnitClass_PlayDestroyAnim_Generalize2, 0x6)
{
	R->EAX(PlayDestroyAnimGeneralize::pType);
	return R->Origin() + 6;
}

DEFINE_HOOK(0x746D61, UnitClass_Destroy_ToggleAnim, 0x7)
{
	enum { SkipGameCode = 0x746D68 };

	GET(TechnoClass*, pThis, ECX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	if (pTypeExt->ExplodeOnDestroy.Get(pThis->WhatAmI() == AbstractType::Unit || RulesExt::Global()->NonVehExplodeOnDestroy))
		static_cast<UnitClass*>(pThis)->Explode();

	R->ESI(pThis);
	return SkipGameCode;
}

DEFINE_JUMP(VTABLE, 0x7EB1C8, 0x746D60); // Redirect InfantryClass::Explode(TechnoClass::Explode) to UnitClass::Explode
DEFINE_JUMP(VTABLE, 0x7E402C, 0x746D60); // Redirect BuildingClass::Explode(TechnoClass::Explode) to UnitClass::Explode
DEFINE_JUMP(VTABLE, 0x7E2414, 0x746D60); // Redirect AircraftClass::Explode(TechnoClass::Explode) to UnitClass::Explode

DEFINE_HOOK(0x7418D4, UnitClass_CrushCell_FireDeathWeapon, 0x6)
{
	GET(ObjectClass*, pThis, ESI);

	if ((pThis->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None)
	{
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

		if (pTypeExt && pTypeExt->FireDeathWeaponOnCrushed.Get(RulesExt::Global()->FireDeathWeaponOnCrushed))
		{
			auto const pTechno = static_cast<TechnoClass*>(pThis);
			pTechno->FireDeathWeapon(0);
		}
	}

	return 0;
}

DEFINE_HOOK(0x70D703, TechnoClass_FireDeathWeapon_UseGlobalDeathWeaponDamage, 0xA)
{
	enum { ReplaceDamage = 0x70D724 };

	if (RulesExt::Global()->UseGlobalDeathWeaponDamage)
	{
		auto const pDeathWeapon = RulesClass::Instance()->DeathWeapon;
		R->EDI(pDeathWeapon);
		R->EAX(pDeathWeapon ? pDeathWeapon->Damage : 0);
		return ReplaceDamage;
	}

	return 0;
}

#pragma endregion

#pragma region RallyPointEnhancement

DEFINE_HOOK(0x44368D, BuildingClass_ObjectClickedAction_RallyPoint, 0x7)
{
	enum { OnTechno = 0x44363C };

	return RulesExt::Global()->RallyPointOnTechno ? OnTechno : 0;
}

DEFINE_HOOK(0x4473F4, BuildingClass_MouseOverObject_JustHasRallyPoint, 0x6)
{
	enum { JustRally = 0x447413 };

	GET(BuildingClass* const, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->JustHasRallyPoint) ? JustRally : 0;
}

DEFINE_HOOK(0x447413, BuildingClass_MouseOverObject_RallyPointForceMove, 0x5)
{
	enum { AlwaysAlt = 0x44744E };

	return RulesExt::Global()->RallyPointForceMove ? AlwaysAlt : 0;
}

DEFINE_HOOK(0x70000E, TechnoClass_MouseOverObject_RallyPointForceMove, 0x5)
{
	enum { AlwaysAlt = 0x700038 };

	GET(TechnoClass* const, pThis, ESI);

	if (pThis->WhatAmI() == AbstractType::Building && RulesExt::Global()->RallyPointForceMove)
	{
		auto const pType = static_cast<BuildingClass*>(pThis)->Type;
		auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pType);
		bool HasRallyPoint = (pTypeExt ? pTypeExt->JustHasRallyPoint : false) || pType->Factory == AbstractType::UnitType || pType->Factory == AbstractType::InfantryType || pType->Factory == AbstractType::AircraftType;
		return HasRallyPoint ? AlwaysAlt : 0;
	}

	return 0;
}

DEFINE_HOOK(0x44748E, BuildingClass_MouseOverObject_JustHasRallyPointAircraft, 0x6)
{
	enum { JustRally = 0x44749D };

	GET(BuildingClass* const, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->JustHasRallyPoint) ? JustRally : 0;
}

DEFINE_HOOK(0x447674, BuildingClass_MouseOverCell_JustHasRallyPoint, 0x6)
{
	enum { JustRally = 0x447683 };

	GET(BuildingClass* const, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->JustHasRallyPoint) ? JustRally : 0;
}

DEFINE_HOOK(0x447643, BuildingClass_MouseOverCell_RallyPointForceMove, 0x5)
{
	enum { AlwaysAlt = 0x447674 };

	return RulesExt::Global()->RallyPointForceMove ? AlwaysAlt : 0;
}

DEFINE_HOOK(0x700B28, TechnoClass_MouseOverCell_RallyPointForceMove, 0x6)
{
	enum { AlwaysAlt = 0x700B30 };

	GET(TechnoClass* const, pThis, ESI);

	if (pThis->WhatAmI() == AbstractType::Building && RulesExt::Global()->RallyPointForceMove)
	{
		auto const pType = static_cast<BuildingClass*>(pThis)->Type;
		auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pType);
		bool HasRallyPoint = (pTypeExt ? pTypeExt->JustHasRallyPoint : false) || pType->Factory == AbstractType::UnitType || pType->Factory == AbstractType::InfantryType || pType->Factory == AbstractType::AircraftType;

		return HasRallyPoint ? AlwaysAlt : 0;
	}

	return 0;
}

DEFINE_HOOK(0x455DA0, BuildingClass_IsUnitFactory_JustHasRallyPoint, 0x6)
{
	enum { SkipGameCode = 0x455DCC };

	GET(BuildingClass* const, pThis, ECX);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->JustHasRallyPoint) ? SkipGameCode : 0;
}

// Handle the rally of infantry.
DEFINE_HOOK(0x444CA3, BuildingClass_KickOutUnit_RallyPointAreaGuard1, 0x6)
{
	enum { SkipQueueMove = 0x444D11 };

	GET(BuildingClass*, pThis, ESI);
	GET(FootClass*, pProduct, EDI);

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetFocus(pThis->Focus);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

// Vehicle but without BuildingClass::Unload calling, e.g. the building has WeaponsFactory = no set.
// Currently I have no idea about how to deal with the normally unloaded vehicles.
// Also fix the bug that WeaponsFactory = no will make the product ignore the rally point.
// Also fix the bug that WeaponsFactory = no will make the Jumpjet product park on the ground.
DEFINE_HOOK(0x4448CE, BuildingClass_KickOutUnit_RallyPointAreaGuard2, 0x6)
{
	enum { SkipGameCode = 0x4448F8 };

	GET(FootClass*, pProduct, EDI);
	GET(BuildingClass*, pThis, ESI);

	auto const pFocus = pThis->Focus;

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetFocus(pFocus);
		pProduct->QueueMission(Mission::Area_Guard, true);
	}
	else
	{
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Move, true);
	}

	if (!pFocus && pProduct->GetTechnoType()->Locomotor == LocomotionClass::CLSIDs::Jumpjet)
		pProduct->Scatter(CoordStruct::Empty, false, false);

	return SkipGameCode;
}

// This makes the building has WeaponsFactory = no to kick out units in the cell same as WeaponsFactory = yes.
// Also enhanced the ExitCoord.
DEFINE_HOOK(0x4448B0, BuildingClass_KickOutUnit_ExitCoords, 0x6)
{
	if (!RulesExt::Global()->EnableEnhancedExitCoords)
		return 0;

	GET(FootClass*, pProduct, EDI);
	GET(BuildingClass*, pThis, ESI);
	GET(CoordStruct*, pCrd, ECX);

	auto const isJJ = pProduct->GetTechnoType()->Locomotor == LocomotionClass::CLSIDs::Jumpjet;
	auto const pProductType = pProduct->GetTechnoType();
	auto const buildingExitCrd = isJJ ? BuildingTypeExt::ExtMap.Find(pThis->Type)->JumpjetExitCoord.Get(pThis->Type->ExitCoord)
		: TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType())->ExitCoord.Get(pThis->Type->ExitCoord);
	auto const exitCrd = TechnoTypeExt::ExtMap.Find(pProductType)->ExitCoord.Get(buildingExitCrd);

	pCrd->X += exitCrd.X;
	pCrd->Y += exitCrd.Y;
	pCrd->Z += exitCrd.Z;

	if (!isJJ)
	{
		auto nCell = CellClass::Coord2Cell(*pCrd);
		auto const pCell = MapClass::Instance->GetCellAt(nCell);
		bool isBridge = pCell->ContainsBridge();
		nCell = MapClass::Instance->NearByLocation(CellClass::Coord2Cell(*pCrd),
			pProductType->SpeedType, -1, pProductType->MovementZone, isBridge, 1, 1, false,
			false, false, isBridge, nCell, false, false);
		*pCrd = CellClass::Cell2Coord(nCell, pCrd->Z);
	}

	return 0;
}

// Ships.
DEFINE_HOOK(0x444424, BuildingClass_KickOutUnit_RallyPointAreaGuard3, 0x5)
{
	enum { SkipQueueMove = 0x44443F };

	GET(FootClass*, pProduct, EDI);
	GET(AbstractClass*, pFocus, ESI);

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetFocus(pFocus);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

// For common aircrafts.
// Also make AirportBound aircraft not ignore the rally point.
DEFINE_HOOK(0x444061, BuildingClass_KickOutUnit_RallyPointAreaGuard4, 0x6)
{
	enum { SkipQueueMove = 0x444091, NotSkip = 0x444075 };

	GET(FootClass*, pProduct, EBP);
	GET(AbstractClass*, pFocus, ESI);

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetFocus(pFocus);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return NotSkip;
}

// For aircrafts with AirportBound = no and the airport is full.
// Still some other bug in it.
DEFINE_HOOK(0x443EB8, BuildingClass_KickOutUnit_RallyPointAreaGuard5, 0x5)
{
	enum { SkipQueueMove = 0x443ED3 };

	GET(FootClass*, pProduct, EBP);
	GET(AbstractClass*, pFocus, EAX);

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetFocus(pFocus);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

#pragma endregion

#pragma region CrushBuildingOnAnyCell

namespace CrushBuildingOnAnyCell
{
	CellClass* pCell;
}

DEFINE_HOOK(0x741733, UnitClass_CrushCell_SetContext, 0x6)
{
	GET(CellClass*, pCell, ESI);

	CrushBuildingOnAnyCell::pCell = pCell;

	return 0;
}

DEFINE_HOOK(0x741925, UnitClass_CrushCell_CrushBuilding, 0x5)
{
	GET(UnitClass*, pThis, EDI);

	if (RulesExt::Global()->CrushBuildingOnAnyCell)
	{
		if (auto const pBuilding = CrushBuildingOnAnyCell::pCell->GetBuilding())
		{
			if (reinterpret_cast<bool(__thiscall*)(BuildingClass*, TechnoClass*)>(0x5F6CD0)(pBuilding, pThis)) // IsCrushable
			{
				VocClass::PlayAt(pBuilding->Type->CrushSound, pThis->Location, 0);
				pBuilding->Destroy();
				pBuilding->RegisterDestruction(pThis);
				pBuilding->Mark(MarkType::Up);
				// pBuilding->Limbo(); // Vanilla do this. May be not necessary?
				pBuilding->UnInit();

				R->AL(true);
			}
		}
	}

	return 0;
}

#pragma endregion
