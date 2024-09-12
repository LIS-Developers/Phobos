#include <Utilities/Macro.h>
#include <Utilities/TemplateDef.h>
#include <GameOptionsClass.h>
#include <WWMouseClass.h>
#include <EventClass.h>
#include <SuperClass.h>
#include <MessageListClass.h>
#include <Ext/House/Body.h>
#include <Ext/SWType/Body.h>
#include <Utilities/AresHelper.h>
#include "PhobosToolTip.h"

// Test function for extra pressable buttons above tactical map
namespace MousePressHelper
{
	// Generic
	Point2D LastPosition = Point2D::Empty; // Check moving
	int ButtonIndex = -1; // -1 -> above no buttons, 0 -> above buttons background, POSITIVE -> above button who have this index
	bool PressedInButtonsLayer = false; // Check press
	int CheckMouseOverButtons(const Point2D* pMousePosition, int mode);
	void PressDesignatedButton(int buttonIndex);

	// Button index 1-8 : Super weapons buttons
	void DrawButtonForSuperWeapon();
	bool MoveButtonForSuperWeapon(SuperWeaponTypeClass* pDataType, SuperWeaponTypeClass* pAddType, SWTypeExt::ExtData* pAddTypeExt, unsigned int ownerBits);
}

// Note 1:
// After long pressing the right button to move the in-game perspective,
// both the mouse lift action and the next press action will be invalid.
// Therefore, it is recommended to trigger the function when the button
// is lifted, just like in the vanilla game. - CrimRecya

// Note 2:
// If some of the functions are intended for some multiplayer gamemodes,
// I think they should be triggered through the EventClass. But in fact,
// there are too few existing events can be used that I don't know what
// universal functionality can be achieved. - CrimRecya

// TODO ActiveButtonClass::ButtonGroup::ButtonShape  ->  then fix " 'inline' const wchar_t* PhobosToolTip::GetBuffer() const"

// Functions

// Return button index
int MousePressHelper::CheckMouseOverButtons(const Point2D* pMousePosition, int mode)
{
	// mode == 0 -> Check Only

	if (!mode || mode == 1) // Left Press
	{
		if (pMousePosition->X < 60 && pMousePosition->X >= 0) // Button index 1-8 : Super weapons buttons
		{
			const int currentCounts = HouseExt::ExtMap.Find(HouseClass::CurrentPlayer)->SuperWeaponButtonCount;
			const int height = DSurface::Composite->GetHeight();
			Point2D position { 0, (height - 32 - 48 * currentCounts - 2 * (currentCounts - 1)) / 2 };

			for (int i = 0; i < currentCounts; ++i)
			{
				if (pMousePosition->Y < position.Y)
					break;

				position.Y += 48;

				if (pMousePosition->Y < position.Y)
					return i + 1;

				position.Y += 2;
			}
		}

		// TODO New buttons (Start from index = 9)
	}
/*
	if (!mode || mode == 2) // Left Release
	{
		; // TODO New buttons (Start from index = 9)
	}

	if (!mode || mode == 3) // Right Press
	{
		; // TODO New buttons (Start from index = 9)
	}

	if (!mode || mode == 4) // Right Release
	{
		; // TODO New buttons (Start from index = 9)
	}
*/
	if (false) // TODO Check in background
	{
		return 0; // Background -> Button index 0
	}

	return -1;
}

void MousePressHelper::PressDesignatedButton(int buttonIndex) // TODO Keyboard shortcuts
{
	if (!buttonIndex) // In buttons background
		return;

	HouseClass* const pHouse = HouseClass::CurrentPlayer;

	if (buttonIndex <= 8) // Button index 1-8 : Super weapons buttons
	{
		const int index = HouseExt::ExtMap.Find(pHouse)->SuperWeaponButtonData[buttonIndex - 1];
		SuperClass* const pSuper = pHouse->Supers.Items[index];
		SWTypeExt::ExtData* const pTypeExt = SWTypeExt::ExtMap.Find(pSuper->Type);

		if (CAN_USE_ARES && AresHelper::CanUseAres)
		{
			const bool autoFire = !pTypeExt->SW_ManualFire && pTypeExt->SW_AutoFire;

			if (!pSuper->CanFire() && !autoFire)
			{
				VoxClass::PlayIndex(pTypeExt->EVA_Impatient);
				return;
			}

			if (!pHouse->CanTransactMoney(pTypeExt->Money_Amount))
			{
				if (pTypeExt->EVA_InsufficientFunds != -1)
					VoxClass::PlayIndex(pTypeExt->EVA_InsufficientFunds);
				else
					VoxClass::Play(&Make_Global<const char>(0x819044)); // 0x819044 -> EVA_InsufficientFunds

				const CSFText csf = pTypeExt->Message_InsufficientFunds;

				if (!csf.empty())
				{
					int color = ColorScheme::FindIndex("Gold");

					if (pTypeExt->Message_FirerColor)
					{
						if (pHouse)
							color = pHouse->ColorSchemeIndex;
					}
					else
					{
						if (pTypeExt->Message_ColorScheme > -1)
							color = pTypeExt->Message_ColorScheme;
						else if (pHouse)
							color = pHouse->ColorSchemeIndex;
					}

					MessageListClass::Instance->PrintMessage(csf, RulesClass::Instance->MessageDelay, color);
				}

				return;
			}

			const bool unstoppable = pSuper->Type->UseChargeDrain && pSuper->ChargeDrainState == ChargeDrainState::Draining
				&& pTypeExt->SW_Unstoppable;

			if (autoFire || unstoppable)
				return;
		}
		else if (!pSuper->CanFire())
		{
			return;
		}

		if (pSuper->Type->Action == Action::None)
		{
			EventClass event
			(
				pHouse->ArrayIndex,
				EventType::SpecialPlace,
				pSuper->Type->ArrayIndex,
				CellStruct::Empty
			);
			EventClass::AddEvent(event);
		}
		else
		{
			DisplayClass::Instance->CurrentBuilding = nullptr;
			DisplayClass::Instance->CurrentBuildingType = nullptr;
			DisplayClass::Instance->unknown_11AC = 0xFFFFFFFF;
			DisplayClass::Instance->SetActiveFoundation(nullptr);
			MapClass::Instance->SetRepairMode(0);
			MapClass::Instance->SetSellMode(0);
			DisplayClass::Instance->PowerToggleMode = false;
			DisplayClass::Instance->PlanningMode = false;
			DisplayClass::Instance->PlaceBeaconMode = false;
			DisplayClass::Instance->CurrentSWTypeIndex = index;
			MapClass::UnselectAll();

			if (CAN_USE_ARES && AresHelper::CanUseAres && pTypeExt->EVA_SelectTarget != -1)
				VoxClass::PlayIndex(pTypeExt->EVA_SelectTarget);
			else
				VoxClass::Play(&Make_Global<const char>(0x83FB78)); // 0x83FB78 -> EVA_SelectTarget
		}
	}
/*	else if (?) // TODO New buttons (Start from index = 9)
	{

	}*/
}

void MousePressHelper::DrawButtonForSuperWeapon()
{
	// TODO Draw background

	HouseExt::ExtData* const pHouseExt = HouseExt::ExtMap.Find(HouseClass::CurrentPlayer);
	auto& data = pHouseExt->SuperWeaponButtonData;
	const int currentCounts = pHouseExt->SuperWeaponButtonCount;

	const int height = DSurface::Composite->GetHeight();
	const int color = Drawing::RGB_To_Int(Drawing::TooltipColor);
	Point2D position { 0, (height - 32 - 48 * currentCounts - 2 * (currentCounts - 1)) / 2 };
	RectangleStruct rect { 0, 0, 60, position.Y + 48 };
	int recordHeight = -1;

	for (int i = 0; i < currentCounts; position.Y += 50, rect.Height += 50) // Button index 1-8
	{
		SuperClass* const pSuper = HouseClass::CurrentPlayer->Supers.Items[data[i]];
		SuperWeaponTypeClass* const pSWType = pSuper->Type;

		if (!CAN_USE_ARES || !AresHelper::CanUseAres)
		{
			SHPStruct* const pSHP = pSWType->SidebarImage;

			if (pSHP)
			{
				DSurface::Composite->DrawSHP(FileSystem::CAMEO_PAL, pSHP, 0, &position, &rect,
					BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			}
		}
		else
		{
			// TODO PCX or SHP

			DSurface::Composite->DrawSHP(FileSystem::SIDEBAR_PAL, Make_Global<SHPStruct*>(0xB07BC0), 0, &position, &rect,
				BlitterFlags(0x401), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}

		// Flash
		if (pSuper->IsReady && pSuper->ShouldFlashTab())
		{
			DSurface::Composite->DrawSHP(FileSystem::SIDEBAR_PAL, Make_Global<SHPStruct*>(0xB07BC0), 0, &position, &rect,
				BlitterFlags(0x404), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}

		// Progress
		if (pSuper->ShouldDrawProgress())
		{
			const int frame = pSuper->AnimStage() + 1;
			DSurface::Composite->DrawSHP(FileSystem::SIDEBAR_PAL, Make_Global<SHPStruct*>(0xB0B484), frame, &position, &rect,
				BlitterFlags(0x404), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}

		// Status
		if (const wchar_t* pName = pSuper->NameReadiness())
		{
			Point2D textLocation { 30, position.Y + 1 };
			const TextPrintType printType = TextPrintType::Center | TextPrintType::FullShadow | TextPrintType::Point8;
			RectangleStruct textRect = Drawing::GetTextDimensions(pName, textLocation, static_cast<WORD>(printType), 2, 1);

			reinterpret_cast<void(__fastcall*)(RectangleStruct*, DSurface*, unsigned short, unsigned char)>(0x621B80)(&textRect, DSurface::Composite, 0, 0xAFu);
			DSurface::Composite->DrawTextA(pName, &rect, &textLocation, static_cast<COLORREF>(color), COLOR_BLACK, printType);
		}

		if (++i == MousePressHelper::ButtonIndex)
			recordHeight = position.Y;
	}

	if (recordHeight >= 0)
	{
		// Select
		position.Y = recordHeight;
		rect.Height = recordHeight + 48;

		RectangleStruct drawRect { 0, position.Y, 60, 48 };
		DSurface::Composite->DrawRectEx(&rect, &drawRect, color);

		// ToolTip
		const int index = HouseExt::ExtMap.Find(HouseClass::CurrentPlayer)->SuperWeaponButtonData[MousePressHelper::ButtonIndex - 1];
		SuperClass* const pSuper = HouseClass::CurrentPlayer->Supers.Items[index];
		PhobosToolTip::Instance.HelpText(pSuper);
	}
}

bool MousePressHelper::MoveButtonForSuperWeapon(SuperWeaponTypeClass* pDataType, SuperWeaponTypeClass* pAddType, SWTypeExt::ExtData* pAddTypeExt, unsigned int ownerBits)
{
	SWTypeExt::ExtData* const pDataTypeExt = SWTypeExt::ExtMap.Find(pDataType);

	if ((pDataTypeExt->SW_ButtonsRequiredHouses & ownerBits) && !(pAddTypeExt->SW_ButtonsRequiredHouses & ownerBits))
		return false;
	else if (!(pDataTypeExt->SW_ButtonsRequiredHouses & ownerBits) && (pAddTypeExt->SW_ButtonsRequiredHouses & ownerBits))
		return true;
	else if (pDataTypeExt->CameoPriority > pAddTypeExt->CameoPriority)
		return false;
	else if (pDataTypeExt->CameoPriority < pAddTypeExt->CameoPriority)
		return true;
	else if (pDataType->RechargeTime < pAddType->RechargeTime)
		return false;
	else if (pDataType->RechargeTime > pAddType->RechargeTime)
		return true;

	return wcscmp(pDataType->UIName, pAddType->UIName) > 0;
}

// Hooks for trigger
DEFINE_HOOK(0x6931A5, ScrollClass_WindowsProcedure_LeftMouseButtonDown, 0x6)
{
	enum { SkipGameCode = 0x6931B4 };

	const Point2D mousePosition = WWMouseClass::Instance->XY1;
	const int buttonIndex = MousePressHelper::CheckMouseOverButtons(&mousePosition, 1);

	if (buttonIndex >= 0)
	{
		MousePressHelper::PressedInButtonsLayer = true;

		// Functions (Recommended)
		MousePressHelper::PressDesignatedButton(buttonIndex);

		R->Stack(STACK_OFFSET(0x28, 0x8), nullptr);
		R->EAX(Action::None);
		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x693268, ScrollClass_WindowsProcedure_LeftMouseButtonUp, 0x5)
{
	enum { SkipGameCode = 0x693276 };

	if (MousePressHelper::PressedInButtonsLayer)
	{
		MousePressHelper::PressedInButtonsLayer = false;
/*
		const Point2D mousePosition = WWMouseClass::Instance->XY1;
		const int buttonIndex = MousePressHelper::CheckMouseOverButtons(&mousePosition, 2);

		if (buttonIndex >= 0)
		{
			; // Functions (Not recommended)
		}
*/
		R->Stack(STACK_OFFSET(0x28, 0x8), nullptr);
		R->EAX(Action::None);
		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x69330E, ScrollClass_WindowsProcedure_RightMouseButtonDown, 0x6)
{
	enum { SkipGameCode = 0x69334A };

	const Point2D mousePosition = WWMouseClass::Instance->XY1;
	const int buttonIndex = MousePressHelper::CheckMouseOverButtons(&mousePosition, 3);

	if (buttonIndex >= 0)
	{
		MousePressHelper::PressedInButtonsLayer = true;

		// Functions (Recommended)

		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x693397, ScrollClass_WindowsProcedure_RightMouseButtonUp, 0x6)
{
	enum { SkipGameCode = 0x6933CB };

	if (MousePressHelper::PressedInButtonsLayer)
	{
		MousePressHelper::PressedInButtonsLayer = false;
/*
		const Point2D mousePosition = WWMouseClass::Instance->XY1;
		const int buttonIndex = MousePressHelper::CheckMouseOverButtons(&mousePosition, 4);

		if (buttonIndex >= 0)
		{
			; // Functions (Not recommended)
		}
*/
		return SkipGameCode;
	}

	return 0;
}

// Hooks for suspend
DEFINE_HOOK(0x692F85, ScrollClass_MouseUpdate_SkipMouseLongPress, 0x7)
{
	enum { CheckMousePress = 0x692F8E, CheckMouseNoPress = 0x692FDC, SkipGameCode = 0x692FAE };

	GET(ScrollClass*, pThis, EBX);

	if (pThis->unknown_byte_554A) // 555A: AnyMouseButtonDown
		return !MousePressHelper::PressedInButtonsLayer ? CheckMousePress : SkipGameCode;

	return CheckMouseNoPress;
}

DEFINE_HOOK(0x69300B, ScrollClass_MouseUpdate_SkipMouseAction, 0x6)
{
	enum { SkipGameCode = 0x69301A };

	const Point2D mousePosition = WWMouseClass::Instance->XY1;

	if (MousePressHelper::LastPosition != mousePosition)
	{
		MousePressHelper::LastPosition = mousePosition;
		MousePressHelper::ButtonIndex = MousePressHelper::CheckMouseOverButtons(&mousePosition, 0);
	}

	if (MousePressHelper::ButtonIndex < 0)
		return 0;

	R->Stack(STACK_OFFSET(0x30, -0x24), nullptr);
	R->EAX(Action::None);
	return SkipGameCode;
}

DEFINE_HOOK(0x4AE511, DisplayClass_GetToolTip_SkipTacticalTip, 0x5)
{
	enum { UseButtonTip = 0x4AE5F8, SkipGameCode = 0x4AE69B };

	const int buttonIndex = MousePressHelper::ButtonIndex;

	if (buttonIndex < 0)
		return 0;

	if (!buttonIndex)
		return SkipGameCode;

	if (buttonIndex <= 8)
		R->EAX(PhobosToolTip::Instance.GetBuffer());
/*	else if (?) // TODO New buttons (Start from index = 9)
		R->EAX(?);*/
	else
		R->EAX(nullptr);

	return UseButtonTip;
}

// Hooks for display
DEFINE_HOOK(0x6D4941, TacticalClass_Render_DrawButtonCameo, 0x6)
{
	// TODO New buttons (The later draw, the higher layer)

	MousePressHelper::DrawButtonForSuperWeapon();
	return 0;
}

// Hooks for function
DEFINE_HOOK(0x4F9283, HouseClass_Update_RecheckTechTree, 0x5)
{
	HouseClass* const pHouse = HouseClass::CurrentPlayer;
	HouseExt::ExtData* const pHouseExt = HouseExt::ExtMap.Find(pHouse);
	auto& data = pHouseExt->SuperWeaponButtonData;

	for (int i = 0; i < pHouseExt->SuperWeaponButtonCount; ++i)
	{
		if (data[i] >= pHouse->Supers.Count || !pHouse->Supers.Items[data[i]]->IsPresent)
		{
			const int counts = pHouseExt->SuperWeaponButtonCount - 1;

			for (int j = i; j < counts; ++j)
			{
				data[j] = data[j + 1];
			}

			data[--pHouseExt->SuperWeaponButtonCount] = -1;
		}
	}

	return 0;
}

DEFINE_HOOK(0x6A6314, SidebarClass_AddCameo_SupportNewButtons, 0x8)
{
	enum { SkipGameCode = 0x6A65F5 };

	GET_STACK(const AbstractType, absType, STACK_OFFSET(0x14, 0x4));
	GET_STACK(int, index, STACK_OFFSET(0x14, 0x8));

	if (absType != AbstractType::Special || SuperWeaponTypeClass::Array->Count <= index)
		return 0;

	SuperWeaponTypeClass* const pType = SuperWeaponTypeClass::Array->Items[index];
	SWTypeExt::ExtData* const pTypeExt = SWTypeExt::ExtMap.Find(pType);

	HouseClass* const pHouse = HouseClass::CurrentPlayer;
	HouseExt::ExtData* const pHouseExt = HouseExt::ExtMap.Find(pHouse);
	const unsigned int ownerBits = 1u << pHouse->Type->ArrayIndex;

	if (pTypeExt->SW_ShowInButtons && (pTypeExt->SW_ButtonsRequiredHouses & ownerBits))
	{
		auto& data = pHouseExt->SuperWeaponButtonData;
		bool overflow = true;
		bool move = false;

		for (int i = 0; i < 8; ++i) // 8 buttons at max
		{
			if (move)
			{
				if (data[i] != -1)
				{
					int buffer = data[i];
					data[i] = index;
					index = buffer;
				}
				else
				{
					data[i] = index;
					overflow = false;
					++pHouseExt->SuperWeaponButtonCount;
					break;
				}
			}
			else if (data[i] != -1)
			{
				if (MousePressHelper::MoveButtonForSuperWeapon(SuperWeaponTypeClass::Array->Items[data[i]], pType, pTypeExt, ownerBits))
				{
					move = true;
					int buffer = data[i];
					data[i] = index;
					index = buffer;
				}
			}
			else
			{
				data[i] = index;
				overflow = false;
				++pHouseExt->SuperWeaponButtonCount;
				break;
			}
		}

		if (overflow)
			R->Stack(STACK_OFFSET(0x14, 0x8), index); // Draw it on the sidebar instead of the button
		else
			return SkipGameCode; // No longer need to draw on the sidebar
	}

	return 0;
}
