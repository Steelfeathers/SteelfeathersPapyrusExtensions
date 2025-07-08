#include "papyrus.h"

namespace Papyrus
{
	std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
	}

	//0 is regular item transfer, 1 is stealing from a container or corpse, 2 is pickpocketing, 3 is transfer as teammate
	bool OpenInventoryEx(STATIC_ARGS, RE::Actor* target, int type)
	{
		if (!target || target->IsDisabled()) return false;

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
		if (!contRef || !item) return nullptr;

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
		if (!contRef || !item) return nullptr;
		auto* ownerForm = GetFormOwner(contRef, item);
		return ownerForm->As<RE::TESNPC>();
	}

	
	RE::TESFaction* GetFormFactionOwner(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item)
	{
		if (!contRef || !item) return nullptr;
		auto* ownerForm = GetFormOwner(contRef, item);
		return ownerForm->As<RE::TESFaction>();
	}

	bool IsFormStolen(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item)
	{
		if (!contRef || !item) return false;

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
		if (!contRef || !item || !owner) return false;

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

	int GetTotalGoldValue(STATIC_ARGS, RE::TESForm* item)
	{
		if (!item) return 0;
		if (item->IsMagicItem())
		{
			return item->As<RE::MagicItem>()->CalculateTotalGoldValue(RE::PlayerCharacter::GetSingleton());
		}
		else
		{
			return item->GetGoldValue();
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
	}

	bool RegisterFunctions(VM* a_vm) {
		SECTION_SEPARATOR;
		logger::info("Binding papyrus functions in SteelfeathersPapyrusExtensions..."sv, script);
		Bind(*a_vm);
		logger::info("Finished binding functions."sv);
		return true;
	}
}
