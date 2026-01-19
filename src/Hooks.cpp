#include "PCH.h"
#include "Hooks.h"

// Address Library IDs for FuzRoBork hooks
// Research sources:
// - powerof3/FloatingSubtitles: QueueDialogSubtitles = RELOCATION_ID(51916, 52854)
// - CommonLibSSE-NG SubtitleManager.h: KillSubtitles = RELOCATION_ID(51755, 52628)
// - Original Fuz Ro D-oh 64 RVAs (for 1.6.1170):
//   - CachedResponseData_Ctor: 0x5DE460
//   - UIUtils_QueueDialogSubtitles: 0x976E60
//   - ASCM_DisplayQueuedNPCChatterData: 0x96D1B0
//   - ASCM_QueueNPCChatterData: 0x96CB00
//
// NOTE: IDs need verification with Address Library Manager tool (meh321)
// Use FindIdByOffset() to convert RVA offsets to IDs
namespace Addresses
{
	// CachedResponseData constructor hook point
	// Old RVA: 0x1405DE460 (offset: 0x5DE460) + 0xEC
	// Related to: DialogueMenu::ProcessMessage, voice file path assignment
	// STATUS: NEEDS VERIFICATION - ID estimated based on nearby subtitle functions
	inline REL::Relocation<std::uintptr_t> CachedResponseData_Ctor{ RELOCATION_ID(50763, 51658) };
	constexpr std::ptrdiff_t CachedResponseData_Ctor_Offset = 0xEC;

	// UIUtils::QueueDialogSubtitles hook point
	// Old RVA: 0x140976E60 (offset: 0x976E60) + 0x4D
	// Related to: Subtitle display system
	// SOURCE: powerof3/FloatingSubtitles RE.cpp - VERIFIED
	inline REL::Relocation<std::uintptr_t> UIUtils_QueueDialogSubtitles{ RELOCATION_ID(51916, 52854) };
	constexpr std::ptrdiff_t UIUtils_QueueDialogSubtitles_Hook_Offset = 0x4D;
	constexpr std::ptrdiff_t UIUtils_QueueDialogSubtitles_Show_Offset = 0x5A;
	constexpr std::ptrdiff_t UIUtils_QueueDialogSubtitles_Exit_Offset = 0x103;

	// AudioSubtitleControllerManager::DisplayQueuedNPCChatterData
	// Old RVA: 0x14096D1B0 (offset: 0x96D1B0)
	// Related to: NPC chatter subtitle display
	// STATUS: NEEDS VERIFICATION - ID estimated based on nearby functions
	inline REL::Relocation<std::uintptr_t> ASCM_DisplayQueuedNPCChatterData{ RELOCATION_ID(51971, 52851) };
	constexpr std::ptrdiff_t ASCM_DisplayQueuedNPCChatterData_DialogSubs_Hook_Offset = 0x1CA;
	constexpr std::ptrdiff_t ASCM_DisplayQueuedNPCChatterData_DialogSubs_Show_Offset = 0x1D3;
	constexpr std::ptrdiff_t ASCM_DisplayQueuedNPCChatterData_DialogSubs_Exit_Offset = 0x1FD;
	constexpr std::ptrdiff_t ASCM_DisplayQueuedNPCChatterData_GeneralSubs_Hook_Offset = 0x99;
	constexpr std::ptrdiff_t ASCM_DisplayQueuedNPCChatterData_GeneralSubs_Show_Offset = 0xA6;
	constexpr std::ptrdiff_t ASCM_DisplayQueuedNPCChatterData_GeneralSubs_Exit_Offset = 0x1CA;

	// AudioSubtitleControllerManager::QueueNPCChatterData
	// Old RVA: 0x14096CB00 (offset: 0x96CB00) + 0x85
	// Related to: NPC chatter queueing
	// STATUS: NEEDS VERIFICATION - ID estimated based on nearby functions
	inline REL::Relocation<std::uintptr_t> ASCM_QueueNPCChatterData{ RELOCATION_ID(51966, 52846) };
	constexpr std::ptrdiff_t ASCM_QueueNPCChatterData_Hook_Offset = 0x85;
	constexpr std::ptrdiff_t ASCM_QueueNPCChatterData_Show_Offset = 0x92;
	constexpr std::ptrdiff_t ASCM_QueueNPCChatterData_Exit_Offset = 0xCA;
}

void SneakAtackVoicePath(CachedResponseData* Data, char* VoicePathBuffer)
{
	SKSE::log::info("Sneak attack voice path {}", VoicePathBuffer);

	// overwritten code
	CALL_MEMBER_FN(&Data->voiceFilePath, Set)(VoicePathBuffer);

	if (strlen(VoicePathBuffer) < 17)
		return;

	std::string FUZPath(VoicePathBuffer), WAVPath(VoicePathBuffer), XWMPath(VoicePathBuffer);
	WAVPath.erase(0, 5);

	FUZPath.erase(0, 5);
	FUZPath.erase(FUZPath.length() - 3, 3);
	FUZPath.append("fuz");

	XWMPath.erase(0, 5);
	XWMPath.erase(XWMPath.length() - 3, 3);
	XWMPath.append("xwm");

	BSIStream* WAVStream = BSIStream::CreateInstance(WAVPath.c_str());
	BSIStream* FUZStream = BSIStream::CreateInstance(FUZPath.c_str());
	BSIStream* XWMStream = BSIStream::CreateInstance(XWMPath.c_str());

	if (WAVStream->valid == 0 && FUZStream->valid == 0 && XWMStream->valid == 0)
	{
		SKSE::log::info("No valid voice file, processing");
		static const int kWordsPerSecond = kWordsPerSecondSilence.GetData().i;
		static const int kMaxSeconds = 10;

		int SecondsOfSilence = 2;
		char ShimAssetFilePath[0x104] = { 0 };
		std::string ResponseText(Data->responseText.Get());

		if (ResponseText.length() > 4 && strncmp(ResponseText.c_str(), "<ID=", 4))
		{
			SKSE::log::info("Got response subtitle");
			SME::StringHelpers::Tokenizer TextParser(ResponseText.c_str(), " ");
			int WordCount = 0;

			while (TextParser.NextToken(ResponseText) != -1)
				WordCount++;

			SecondsOfSilence = WordCount / ((kWordsPerSecond > 0) ? kWordsPerSecond : 2) + 1;

			if (SecondsOfSilence <= 0)
				SecondsOfSilence = 2;
			else if (SecondsOfSilence > kMaxSeconds)
				SecondsOfSilence = kMaxSeconds;

			// calculate the response text's hash and stash it for later lookups
			SubtitleHasher::Instance.Add(Data->responseText.Get());
		}

		if (ResponseText.length() > 1 || (ResponseText.length() == 1 && ResponseText[0] == ' ' && kSkipEmptyResponses.GetData().i == 0))
		{
			SKSE::log::info("Missing Asset");
			FORMAT_STR(ShimAssetFilePath, "Data\\Sound\\Voice\\Fuz Ro Bork\\Stock_%d.xwm", SecondsOfSilence);
			CALL_MEMBER_FN(&Data->voiceFilePath, Set)(ShimAssetFilePath);
			SKSE::log::info("Missing Asset - Switching to '{}'", ShimAssetFilePath);
		}
	}

	WAVStream->Dtor();
	FUZStream->Dtor();
	XWMStream->Dtor();
}

bool ShouldForceSubs(NPCChatterData* ChatterData, UInt32 ForceRegardless, const char* Subtitle)
{
	bool Result = false;

	if (Subtitle && SubtitleHasher::Instance.HasMatch(Subtitle))		// force if the subtitle is for a voiceless response
	{
		SKSE::log::info("Found a match for {} - Forcing subs", Subtitle);
		Result = true;
	}
	else if (ForceRegardless || (ChatterData && ChatterData->forceSubtitles))
		Result = true;
	else
	{
		TESTopicInfo* CurrentTopicInfo = nullptr;
		PlayerDialogData* Selection = nullptr;

		if (override::MenuTopicManager::GetSingleton()->selectedResponseNode)
			Selection = override::MenuTopicManager::GetSingleton()->selectedResponseNode->Head.Data;
		else
			Selection = override::MenuTopicManager::GetSingleton()->lastSelectedResponse;

		if (Selection)
			CurrentTopicInfo = Selection->parentTopicInfo;
		else if (override::MenuTopicManager::GetSingleton()->rootTopicInfo)
			CurrentTopicInfo = override::MenuTopicManager::GetSingleton()->rootTopicInfo;
		else
			CurrentTopicInfo = override::MenuTopicManager::GetSingleton()->unk14;

		if (CurrentTopicInfo)
		{
			if ((CurrentTopicInfo->dialogFlags >> 9) & 1)		// force subs flag's set
				Result = true;
		}
	}

	return Result;
}

bool ShouldForceSubs1(NPCChatterData* ChatterData, UInt32 ForceRegardless, const char* Subtitle)
{
	SKSE::log::info("ShouldForceSubs1");
	return ShouldForceSubs(ChatterData, ForceRegardless, Subtitle);
}

bool ShouldForceSubs2(NPCChatterData* ChatterData, UInt32 ForceRegardless, const char* Subtitle)
{
	SKSE::log::info("ShouldForceSubs2");
	return ShouldForceSubs(ChatterData, ForceRegardless, Subtitle);
}

bool ShouldForceSubs3(NPCChatterData* ChatterData, UInt32 ForceRegardless, const char* Subtitle)
{
	SKSE::log::info("ShouldForceSubs3");
	if (ChatterData->speaker != (*g_invalidRefHandle) && ChatterData->speaker != 0) {
		NiPointer<TESObjectREFR> refr;
		LookupREFRByHandle(ChatterData->speaker, refr);

		if (Subtitle && SubtitleHasher::Instance.HasMatch(Subtitle))		// force if the subtitle is for a voiceless response
		{
			SKSE::log::info("Voiceless NPC speech {}", Subtitle);

			if (!refr || !refr->baseForm) {
				SKSE::log::info("Couldn't get speaker, aborting");
			}
			else {
				TESNPC* npc = DYNAMIC_CAST(refr->baseForm, TESForm, TESNPC);
				if (npc) {
					SKSE::log::info("Speaker: {} ({})", npc->fullName.GetName(), CALL_MEMBER_FN(npc, GetSex)() == 0 ? "male" : "female");
					FuzRoBorkNamespace::AddSpeechToQueue(npc, Subtitle);
				}
			}

			return true;
		}
	}
	return ShouldForceSubs(ChatterData, ForceRegardless, Subtitle);
}

bool ShouldForceSubs4(NPCChatterData* ChatterData, UInt32 ForceRegardless, const char* Subtitle)
{
	SKSE::log::info("ShouldForceSubs4");
	return ShouldForceSubs(ChatterData, ForceRegardless, Subtitle);
}

#define PUSH_VOLATILE		push(rcx); push(rdx); push(r8); sub(rsp, 0x20);
#define POP_VOLATILE		add(rsp, 0x20); pop(r8); pop(rdx); pop(rcx);

bool InstallHooks()
{
	SKSE::log::info("Installing hooks...");
	SKSE::log::info("Using Address Library IDs - some IDs may need verification");
	SKSE::log::info("See Hooks.cpp comments for ID sources and verification status");

	auto& trampoline = SKSE::GetTrampoline();
	trampoline.create(1024 * 2);

	try {
		// CachedResponseData constructor hook
		{
			auto hook_addr = Addresses::CachedResponseData_Ctor.address() + Addresses::CachedResponseData_Ctor_Offset;

			struct Patch : Xbyak::CodeGenerator
			{
				Patch(void* buf, uintptr_t ret_addr) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label RetnLabel;

					push(rcx);
					push(rdx);		// asset path
					mov(rcx, rbx);	// cached response
					mov(rax, (uintptr_t)SneakAtackVoicePath);
					call(rax);
					pop(rdx);
					pop(rcx);
					jmp(ptr[rip + RetnLabel]);

					L(RetnLabel);
					dq(ret_addr);
				}
			};

			void* CodeBuf = trampoline.allocate(Patch(nullptr, 0));
			Patch code(CodeBuf, hook_addr + 0x5);
			trampoline.write_branch<5>(hook_addr, uintptr_t(code.getCode()));

			SKSE::log::info("CachedResponseData hook installed at {:X}", hook_addr);
		}

		// UIUtils::QueueDialogSubtitles hook
		{
			auto hook_addr = Addresses::UIUtils_QueueDialogSubtitles.address() + Addresses::UIUtils_QueueDialogSubtitles_Hook_Offset;
			auto show_addr = Addresses::UIUtils_QueueDialogSubtitles.address() + Addresses::UIUtils_QueueDialogSubtitles_Show_Offset;
			auto exit_addr = Addresses::UIUtils_QueueDialogSubtitles.address() + Addresses::UIUtils_QueueDialogSubtitles_Exit_Offset;

			struct Patch : Xbyak::CodeGenerator
			{
				Patch(void* buf, uintptr_t show, uintptr_t exit) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label ShowLabel;

					mov(rax, (uintptr_t)CanShowDialogSubtitles);
					PUSH_VOLATILE;
					call(rax);
					POP_VOLATILE;
					test(al, al);
					jnz(ShowLabel);

					PUSH_VOLATILE;
					xor(rcx, rcx);
					xor(rdx, rdx);
					mov(r8, r14);	// subtitle
					mov(rax, (uintptr_t)ShouldForceSubs1);
					call(rax);
					POP_VOLATILE;
					test(al, al);
					jnz(ShowLabel);

					jmp(ptr[rip]);
					dq(exit);

					L(ShowLabel);
					jmp(ptr[rip]);
					dq(show);
				}
			};

			void* CodeBuf = trampoline.allocate(Patch(nullptr, 0, 0));
			Patch code(CodeBuf, show_addr, exit_addr);
			trampoline.write_branch<5>(hook_addr, uintptr_t(code.getCode()));

			SKSE::log::info("UIUtils::QueueDialogSubtitles hook installed at {:X}", hook_addr);
		}

		// ASCM::DisplayQueuedNPCChatterData (Dialog subtitles) hook
		{
			auto hook_addr = Addresses::ASCM_DisplayQueuedNPCChatterData.address() + Addresses::ASCM_DisplayQueuedNPCChatterData_DialogSubs_Hook_Offset;
			auto show_addr = Addresses::ASCM_DisplayQueuedNPCChatterData.address() + Addresses::ASCM_DisplayQueuedNPCChatterData_DialogSubs_Show_Offset;
			auto exit_addr = Addresses::ASCM_DisplayQueuedNPCChatterData.address() + Addresses::ASCM_DisplayQueuedNPCChatterData_DialogSubs_Exit_Offset;

			struct Patch : Xbyak::CodeGenerator
			{
				Patch(void* buf, uintptr_t show, uintptr_t exit) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label ShowLabel;

					mov(rax, (uintptr_t)CanShowDialogSubtitles);
					PUSH_VOLATILE;
					call(rax);
					POP_VOLATILE;
					test(al, al);
					jnz(ShowLabel);

					PUSH_VOLATILE;
					mov(rcx, rsi);
					xor(rdx, rdx);
					mov(r8, ptr[rsi + 0x8]);	// subtitle
					mov(rax, (uintptr_t)ShouldForceSubs2);
					call(rax);
					POP_VOLATILE;
					test(al, al);
					jnz(ShowLabel);

					jmp(ptr[rip]);
					dq(exit);

					L(ShowLabel);
					jmp(ptr[rip]);
					dq(show);
				}
			};

			void* CodeBuf = trampoline.allocate(Patch(nullptr, 0, 0));
			Patch code(CodeBuf, show_addr, exit_addr);
			trampoline.write_branch<5>(hook_addr, uintptr_t(code.getCode()));

			SKSE::log::info("ASCM::DisplayQueuedNPCChatterData (Dialog) hook installed at {:X}", hook_addr);
		}

		// ASCM::DisplayQueuedNPCChatterData (General subtitles) hook
		{
			auto hook_addr = Addresses::ASCM_DisplayQueuedNPCChatterData.address() + Addresses::ASCM_DisplayQueuedNPCChatterData_GeneralSubs_Hook_Offset;
			auto show_addr = Addresses::ASCM_DisplayQueuedNPCChatterData.address() + Addresses::ASCM_DisplayQueuedNPCChatterData_GeneralSubs_Show_Offset;
			auto exit_addr = Addresses::ASCM_DisplayQueuedNPCChatterData.address() + Addresses::ASCM_DisplayQueuedNPCChatterData_GeneralSubs_Exit_Offset;

			struct Patch : Xbyak::CodeGenerator
			{
				Patch(void* buf, uintptr_t show, uintptr_t exit) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label ShowLabel;

					mov(rax, (uintptr_t)CanShowGeneralSubtitles);
					PUSH_VOLATILE;
					call(rax);
					POP_VOLATILE;
					test(al, al);
					jnz(ShowLabel);

					PUSH_VOLATILE;
					mov(rcx, rsi);
					xor(rdx, rdx);
					mov(r8, ptr[rsi + 0x8]);	// subtitle
					mov(rax, (uintptr_t)ShouldForceSubs3);
					call(rax);
					POP_VOLATILE;
					test(al, al);
					jnz(ShowLabel);

					jmp(ptr[rip]);
					dq(exit);

					L(ShowLabel);
					jmp(ptr[rip]);
					dq(show);
				}
			};

			void* CodeBuf = trampoline.allocate(Patch(nullptr, 0, 0));
			Patch code(CodeBuf, show_addr, exit_addr);
			trampoline.write_branch<5>(hook_addr, uintptr_t(code.getCode()));

			SKSE::log::info("ASCM::DisplayQueuedNPCChatterData (General) hook installed at {:X}", hook_addr);
		}

		// ASCM::QueueNPCChatterData hook
		{
			auto hook_addr = Addresses::ASCM_QueueNPCChatterData.address() + Addresses::ASCM_QueueNPCChatterData_Hook_Offset;
			auto show_addr = Addresses::ASCM_QueueNPCChatterData.address() + Addresses::ASCM_QueueNPCChatterData_Show_Offset;
			auto exit_addr = Addresses::ASCM_QueueNPCChatterData.address() + Addresses::ASCM_QueueNPCChatterData_Exit_Offset;

			struct Patch : Xbyak::CodeGenerator
			{
				Patch(void* buf, uintptr_t show, uintptr_t exit) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label ShowLabel;

					mov(rax, (uintptr_t)CanShowDialogSubtitles);
					PUSH_VOLATILE;
					call(rax);
					POP_VOLATILE;
					test(al, al);
					jnz(ShowLabel);

					PUSH_VOLATILE;
					xor(rcx, rcx);
					mov(rdx, r15d);
					mov(r8, rbp);	// subtitle
					mov(rax, (uintptr_t)ShouldForceSubs4);
					call(rax);
					POP_VOLATILE;
					test(al, al);
					jnz(ShowLabel);

					jmp(ptr[rip]);
					dq(exit);

					L(ShowLabel);
					jmp(ptr[rip]);
					dq(show);
				}
			};

			void* CodeBuf = trampoline.allocate(Patch(nullptr, 0, 0));
			Patch code(CodeBuf, show_addr, exit_addr);
			trampoline.write_branch<5>(hook_addr, uintptr_t(code.getCode()));

			SKSE::log::info("ASCM::QueueNPCChatterData hook installed at {:X}", hook_addr);
		}

		SKSE::log::info("All hooks installed successfully");
		return true;
	}
	catch (const std::exception& e) {
		SKSE::log::critical("Failed to install hooks: {}", e.what());
		return false;
	}
}
