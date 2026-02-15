#include "papyrus.h"

namespace Papyrus
{
	std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
	}

	//0 is regular item transfer, 1 is stealing from a container or corpse, 2 is pickpocketing, 3 is transfer as teammate
	static bool OpenInventoryEx(STATIC_ARGS, RE::Actor* target, int type)
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

	static RE::TESForm* GetFormOwner(RE::TESObjectREFR* contRef, RE::TESForm* item)
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

		return nullptr;
	}

	static RE::TESNPC* GetFormActorOwner(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item)
	{
		if (!contRef || !item) {
			a_vm->TraceStack("GetFormFactionOwner passed with at least 1 NONE argument.",
				a_stackID, RE::BSScript::IVirtualMachine::Severity::kWarning);
			return nullptr;
		}
		auto* ownerForm = GetFormOwner(contRef, item);
		return ownerForm->As<RE::TESNPC>();
	}

	
	static RE::TESFaction* GetFormFactionOwner(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item)
	{
		if (!contRef || !item) {
			a_vm->TraceStack("GetFormFactionOwner passed with at least 1 NONE argument.",
				a_stackID, RE::BSScript::IVirtualMachine::Severity::kWarning);
			return nullptr;
		}
		auto* ownerForm = GetFormOwner(contRef, item);
		return ownerForm->As<RE::TESFaction>();
	}

	static bool IsFormStolen(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item)
	{
		if (!contRef || !item) {
			a_vm->TraceStack("IsFormStolen passed with at least 1 NONE argument.",
				a_stackID, RE::BSScript::IVirtualMachine::Severity::kWarning);
			return false;
		}

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

	static bool SetFormOwner(RE::TESObjectREFR* contRef, RE::TESForm* item, RE::TESForm* owner)
	{
		if (!contRef || !item || !owner) return false;
		auto* base = skyrim_cast<RE::TESBoundObject*>(item);
		if (!base) {
			return false;
		}

		auto* invChanges = contRef->GetInventoryChanges(true);
		if (!invChanges) {
			return false;
		}

		auto* invLists = invChanges->entryList;
		if (!invLists || invLists->empty()) {
			return false;
		}
		bool success = false;

		for (auto& entry : *invChanges->entryList) {
			auto* obj = entry ? entry->GetObject() : nullptr;
			if (!obj || obj != base) {
				continue;
			}
			auto* xLists = entry->extraLists;
			if (!xLists) {
				continue;
			}

			for (auto* xList : *xLists) {
				auto extraOwnership = xList->GetByType<RE::ExtraOwnership>();
				if (extraOwnership) {
					xList->SetOwner(owner);
					success = true;
				}
			}
		}

		return success; 
	}

	static bool SetFormActorOwner(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item, RE::TESNPC* owner)
	{
		if (!contRef || !item || !owner) {
			a_vm->TraceStack("SetFormActorOwner passed with at least 1 NONE argument.",
				a_stackID, RE::BSScript::IVirtualMachine::Severity::kWarning);
			return false;
		}
		return SetFormOwner(contRef, item, owner);
	}

	static bool SetFormFactionOwner(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item, RE::TESFaction* owner)
	{
		if (!contRef || !item || !owner) {
			a_vm->TraceStack("SetFormFactionOwner passed with at least 1 NONE argument.",
				a_stackID, RE::BSScript::IVirtualMachine::Severity::kWarning);
			return false;
		}
		return SetFormOwner(contRef, item, owner);
	}

	static int GetTotalGoldValue(STATIC_ARGS, RE::TESForm* item)
	{
		if (!item) {
			a_vm->TraceStack("GetTotalGoldValue passed with NONE item.",
				a_stackID, RE::BSScript::IVirtualMachine::Severity::kWarning);
		}
		auto* magicItem = item->As<RE::MagicItem>();
		if (magicItem) {
			float val = floor(magicItem->CalculateTotalGoldValue(RE::PlayerCharacter::GetSingleton()));
			return val < std::numeric_limits<int>::max() ?
				val > std::numeric_limits<int>::lowest() ? static_cast<int>(val) : std::numeric_limits<int>::lowest() :
				std::numeric_limits<int>::max();

		}
		return item ? item->GetGoldValue() : 0;
	}

	static RE::EnchantmentItem* GetFormEnchantment(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::TESForm* item)
	{
		if (!contRef || !item) return nullptr;
		auto* base = skyrim_cast<RE::TESBoundObject*>(item);
		if (!base) {
			return nullptr;
		}

		if (!contRef->GetInventoryCounts().contains(base)) {
			return nullptr;
		}

		//Non-player made enchantment
		auto ench = base->As<RE::TESEnchantableForm>();
		if (ench && ench->formEnchanting) {
			return ench->formEnchanting;
		}

		auto* invChanges = contRef->GetInventoryChanges(true);
		if (!invChanges) {
			return nullptr;
		}

		auto* invLists = invChanges->entryList;
		if (!invLists || invLists->empty()) {
			return nullptr;
		}

		for (auto& entry : *invChanges->entryList) {
			auto* obj = entry ? entry->GetObject() : nullptr;
			if (!obj || obj != base) {
				continue;
			}
			auto* xLists = entry->extraLists;
			if (!xLists) {
				continue;
			}

			//Player-made enchantment
			for (auto* xList : *xLists) {
				auto xEnch = xList->GetByType<RE::ExtraEnchantment>();
				if (xEnch && xEnch->enchantment) {
					return xEnch->enchantment;
				}
			}
		}

		return nullptr;
	}

	static int GetNumEnchantedFormsWithKeyword(STATIC_ARGS, RE::TESObjectREFR* contRef, RE::BGSKeyword* a_keyword)
	{
		if (!contRef || !a_keyword) return 0;
		int count = 0;
		//logger::info("  >GetNumEnchantedFormsWithKeyword() - keyword EDID = {}"sv, keywordEDID);

		auto* invChanges = contRef->GetInventoryChanges(true);
		if (!invChanges) {
			return 0;
		}

		auto* invLists = invChanges->entryList;
		if (!invLists || invLists->empty()) {
			return 0;
		}

		for (auto& entry : *invChanges->entryList) {
			auto* obj = entry ? entry->GetObject() : nullptr;
			if (!obj) {
				continue;
			}

			//Non-player made enchantment
			auto ench = obj->As<RE::TESEnchantableForm>();
			if (ench && ench->formEnchanting) {
				for (auto& effect : ench->formEnchanting->effects) {
					if (effect->baseEffect->HasKeyword(a_keyword)) {
						count += 1;
						break;
					}
				}
				continue;
			}

			auto* xLists = entry->extraLists;
			if (!xLists) {
				continue;
			}

			//Player-made enchantment
			for (auto* xList : *xLists) {
				auto xEnch = xList->GetByType<RE::ExtraEnchantment>();
				if (xEnch && xEnch->enchantment) {
					for (auto& effect : xEnch->enchantment->effects) {
						if (effect->baseEffect->HasKeyword(a_keyword)) {
							count += 1;
							break;
						}
					}
				}
			}
		}

		return count;
	}

	static void Bind(VM& a_vm) {
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
		logger::info("  >Binding GetFormEnchantment..."sv);
		BIND(GetFormEnchantment);
		logger::info("  >Binding GetNumEnchantedFormsWithKeyword..."sv);
		BIND(GetNumEnchantedFormsWithKeyword);
	}

	bool RegisterFunctions(VM* a_vm) {
		SECTION_SEPARATOR;
		logger::info("Binding papyrus functions in SteelfeathersPapyrusExtensions..."sv, script);
		Bind(*a_vm);
		logger::info("Finished binding functions."sv);
		return true;
	}
}
