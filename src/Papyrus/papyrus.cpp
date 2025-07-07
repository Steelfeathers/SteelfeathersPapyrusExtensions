#include "papyrus.h"

namespace Papyrus
{
	std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
	}

	//0 is regular item transfer, 1 is stealing from a container or corpse, 2 is pickpocketing, 3 is transfer as teammate
	bool OpenInventoryEx(STATIC_ARGS, RE::Actor* target, int type)
	{
		if (type < 0 || type > 3)
		{
			return false;
		}
		if (type == 2 && !target->CanPickpocket())
		{
			type = 3;
		}
		target->RE::TESObjectREFR::OpenContainer(type);
		return true;
	}

	RE::TESForm* GetFormOwner(RE::TESObjectREFR* contRef, RE::TESForm* item)
	{
		if (item->IsGold() || item->IsLockpick()) { return nullptr; }

		auto* invChanges = contRef->GetInventoryChanges(true);
		if (invChanges && invChanges->entryList) {
			for (auto& entry : *invChanges->entryList) {
				if (entry && entry->object && entry->object->formID == item->formID)
				{
				//	logger::info("GetFormActorOwner() - Found invChange entry for item!"sv);
					return entry->GetOwner();
				}
			}
		}
		else
		{
		//	logger::info("GetFormActorOwner() - No inventoryChanges or entryList is empty"sv);
		}


		return nullptr;
	}

	RE::TESNPC* GetFormActorOwner(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item)
	{
		auto* ownerForm = GetFormOwner(contRef, item);
		return ownerForm->As<RE::TESNPC>();
	}

	
	RE::TESFaction* GetFormFactionOwner(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item)
	{
		auto* ownerForm = GetFormOwner(contRef, item);
		return ownerForm->As<RE::TESFaction>();
	}

	bool IsFormStolen(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item)
	{
		if (item->IsGold() || item->IsLockpick()) { return false; }

		auto* ownerForm = GetFormOwner(contRef, item);
		if (!ownerForm)
		{
			return false;
		}

		auto* ownerActor = ownerForm->As<RE::TESNPC>();
		if (ownerActor && ownerActor->formID != RE::PlayerCharacter::GetSingleton()->GetActorBase()->formID)
			return true;

		auto* ownerFact = ownerForm->As<RE::TESFaction>();
		if (ownerFact && !RE::PlayerCharacter::GetSingleton()->IsInFaction(ownerFact))
			return true;

		return false;
	}

	bool SetFormOwner(RE::TESObjectREFR* contRef, RE::TESForm* item, RE::TESForm* owner)
	{
		bool success = false;

		auto* invChanges = contRef->GetInventoryChanges(true);
		if (invChanges && invChanges->entryList) {
			for (auto& entry : *invChanges->entryList) {
				if (entry && entry->object && entry->object->formID == item->formID)
				{
					if (entry->extraLists) {
						for (const auto& xList : *entry->extraLists) {
							auto extraOwnership = xList->GetByType<RE::ExtraOwnership>();
							if (extraOwnership) {
							//	logger::info("SetFormOwner() - Found ExtraOwnership, trying to set new owner"sv);
								xList->SetOwner(owner);
								success = true;
							}
						}
					}
				}
			}
		}
		else
		{
		//	logger::info("SetFormOwner() - No inventoryChanges or entryList is empty"sv);
		}

		return success; 
	}

	bool SetFormActorOwner(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item, RE::TESNPC* owner)
	{
		return SetFormOwner(contRef, item, owner);
	}

	bool SetFormFactionOwner(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item, RE::TESFaction* owner)
	{
		return SetFormOwner(contRef, item, owner);
	}

	float GetTotalGoldValue(STATIC_ARGS, RE::TESForm* item)
	{
		if (item->IsMagicItem())
		{
			return item->As<RE::MagicItem>()->CalculateTotalGoldValue(RE::PlayerCharacter::GetSingleton());
		}
		else
		{
			return item->GetGoldValue();
		}
	}

	void ForceCloseMenu(STATIC_ARGS, std::string menuName)
	{
		using Message = RE::UI_MESSAGE_TYPE;
		Message messageID = Message::kHide;
		auto    uiStr = RE::InterfaceStrings::GetSingleton();

		RE::BSFixedString menuStr;
		if (menuName == "GiftMenu")
		{
			menuStr = uiStr->giftMenu;
		}
		else if (menuName == "BarterMenu")
		{
			menuStr = uiStr->barterMenu;
		}
		else if (menuName == "InventoryMenu")
		{
			menuStr = uiStr->inventoryMenu;
		}
		else if (menuName == "ContainerMenu")
		{
			menuStr = uiStr->containerMenu;
		}
		else if (menuName == "Dialogue Menu")
		{
			menuStr = uiStr->dialogueMenu;
		}
		else if (menuName == "Lockpicking Menu")
		{
			menuStr = uiStr->lockpickingMenu;
		}
		//else if (menuName == "Crafting Menu")
		//{
		//	menuStr = uiStr->lockpickingMenu;
		//	RE::ObjectRefHandle curFurnitureRef = RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture();
		//}

		if (menuStr.empty())
		{
			return;
		}

		auto messageQueue = RE::UIMessageQueue::GetSingleton();
		if (messageQueue) {
			messageQueue->AddMessage(menuStr, messageID, nullptr);
		}
	}

	void Bind(VM& a_vm) {
		logger::info("  >Binding OpenInventoryEx..."sv);
		BIND(OpenInventoryEx);
		logger::info("  >Binding GetFormActorOwner..."sv);
		BIND(GetFormActorOwner);
		logger::info("  >Binding GetFormFactionOwner..."sv);
		BIND(GetFormFactionOwner);
		logger::info("  >Binding SetFormActorOwner..."sv);
		BIND(SetFormActorOwner);
		logger::info("  >Binding SetFormFactionOwner..."sv);
		BIND(SetFormFactionOwner);
		logger::info("  >Binding IsFormStolen..."sv);
		BIND(IsFormStolen);
		logger::info("  >Binding GetTotalGoldValue..."sv);
		BIND(GetTotalGoldValue);
		logger::info("  >Binding ForceCloseMenu..."sv);
		BIND(ForceCloseMenu);
	}

	bool RegisterFunctions(VM* a_vm) {
		SECTION_SEPARATOR;
		logger::info("Binding papyrus functions in SteelfeathersPapyrusExtensions..."sv, script);
		Bind(*a_vm);
		logger::info("Finished binding functions."sv);
		return true;
	}
}
