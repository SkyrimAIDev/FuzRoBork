#include "PCH.h"
#include "FuzRoBorkInternals.h"
#include "Hooks.h"
#include "VersionInfo.h"

// Plugin version information
SKSEPluginVersion(
	constexpr SKSE::PluginVersionData{
		.pluginVersion = MAKE_VERSION(3, 2, 5),
		.pluginName = "FuzRoBork",
		.author = "aedenthorn",
		.addressIndependence = SKSE::AddressIndependence::kCompatible,
		.structureIndependence = SKSE::StructureIndependence::kCompatible,
		.compatibleVersions = { SKSE::RUNTIME_SSE_LATEST_AE }
	}
);

// Setup logging
void SetupLog() {
	auto logsFolder = SKSE::log::log_directory();
	if (!logsFolder) {
		SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
		return;
	}

	auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
	auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
	auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
	auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));

	spdlog::set_default_logger(std::move(loggerPtr));
	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_on(spdlog::level::trace);
}

// Scaleform function handler
class SKSEScaleform_BorkFunction : public RE::GFxFunctionHandler
{
public:
	virtual void Call(Params& params) override {
		auto args = params.args;

		std::string speech = "";
		if (args[0].IsString()) {
			speech = args[0].GetString();
		}

		std::string type = "";
		if (args[1].IsString()) {
			type = args[1].GetString();
		}

		if (type != "CHECK_DONE")
			SKSE::log::info("SKSEScaleform_BorkFunction received {}", type);

		if (type == "DIALOGUE_CLICK") {
			FuzRoBorkNamespace::stopSpeaking();
			FuzRoBorkNamespace::startPlayerSpeech(speech.c_str());
		}
		else if (type == "DIALOGUE" && kPlayPlayerDialogue.GetData().i == 1)
			FuzRoBorkNamespace::startPlayerSpeech(speech.c_str());
		else if (type == "BOOK_READ" && kPlayBookPages.GetData().i == 1) {
			FuzRoBorkNamespace::stopSpeaking();
			FuzRoBorkNamespace::startBookSpeech(speech.c_str());
		}
		else if (type == "BOOK_BOOK") {
			FuzRoBorkNamespace::storeBookSpeech(speech.c_str());
		}
		else if (type == "BOOK_PAGES_FIRST") {
			FuzRoBorkNamespace::storeFirstPagesSpeech(speech.c_str());
		}
		else if (type == "BOOK_PAGES") {
			FuzRoBorkNamespace::storePagesSpeech(speech.c_str());
		}
		else if (type == "LOADING_SCREEN" && kPlayLoadingScreenText.GetData().i == 1) {
			FuzRoBorkNamespace::speakLoadingScreen(speech.c_str());
		}
		else if (type == "CHECK_DONE") {
			bool isDone = !FuzRoBorkNamespace::isSpeaking() && !FuzRoBorkNamespace::isXVASpeaking();
			params.result->SetString(isDone ? "YES" : "NO");
			if (isDone) {
				SKSE::log::info("Done speaking");
			}
			else {
				SKSE::log::info("Still speaking");
			}
		}
		else if (type == "STOP") {
			FuzRoBorkNamespace::stopSpeaking();
		}
	}
};

bool RegisterScaleform(RE::GFxMovieView* view, RE::GFxValue* root)
{
	RE::GFxValue obj;
	view->CreateObject(&obj);

	auto borkFunc = new SKSEScaleform_BorkFunction();
	RE::GFxValue fn;
	view->CreateFunction(&fn, borkFunc);
	obj.SetMember("BorkFunction", fn);

	root->SetMember("FuzRoBork", obj);
	return true;
}

void MessageHandler(SKSE::MessagingInterface::Message* msg)
{
	switch (msg->type)
	{
		case SKSE::MessagingInterface::kDataLoaded:
		{
			// Schedule a cleanup thread for the subtitle hasher
			std::thread CleanupThread([]() {
				while (true)
				{
					std::this_thread::sleep_for(std::chrono::seconds(2));
					SubtitleHasher::Instance.Tick();
				}
			});
			CleanupThread.detach();

			SKSE::log::info("Scheduled cleanup thread");

			std::thread CheckSpeechThread([]() {
				SpeakObj obj;
				while (true)
				{
					if (!FuzRoBorkNamespace::isSpeaking() && !FuzRoBorkNamespace::isXVASpeaking() && FuzRoBorkNamespace::GetSpeechFromQueue(obj)) {
						SKSE::log::info("Speaking queued speech {}", obj.speech);
						actionSpeaking = true;
						std::thread(FuzRoBorkNamespace::speakTask, obj).detach();
						FuzRoBorkNamespace::EraseFromQueue();
					}
					std::this_thread::sleep_for(100ms);
				}
			});
			CheckSpeechThread.detach();

			SKSE::log::info("{} Initialized!", MakeSillyName());
			FuzRoBorkNamespace::LoadXML();
			FuzRoBorkNamespace::ImportTranslationFiles();
		}
		break;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	// Initialize SKSE
	SKSE::Init(skse);

	// Setup logging
	SetupLog();
	SKSE::log::info("{} Initializing...", MakeSillyName());

	// Initialize INI
	SKSE::log::info("Initializing INI Manager");
	FuzRoBorkINIManager::Instance.Initialize("Data\\SKSE\\Plugins\\FuzRoBork.ini", nullptr);

	// Get messaging interface
	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging) {
		SKSE::log::critical("Couldn't get messaging interface");
		return false;
	}

	// Register message listener
	if (!messaging->RegisterListener(MessageHandler)) {
		SKSE::log::critical("Couldn't register message listener");
		return false;
	}

	// Install hooks
	if (!InstallHooks()) {
		SKSE::log::critical("Failed to install hooks");
		return false;
	}

	// Get Scaleform interface
	auto scaleform = SKSE::GetScaleformInterface();
	if (!scaleform) {
		SKSE::log::critical("Couldn't get scaleform interface");
		return false;
	}

	// Register Scaleform functions
	if (!scaleform->Register("FuzRoBork", RegisterScaleform)) {
		SKSE::log::critical("Register scaleform methods failed");
		return false;
	}
	SKSE::log::info("Register scaleform method Succeeded");

	// Get Papyrus interface
	auto papyrus = SKSE::GetPapyrusInterface();
	if (!papyrus) {
		SKSE::log::critical("Couldn't get papyrus interface");
		return false;
	}

	// Register Papyrus functions
	if (!papyrus->Register(FuzRoBorkNamespace::RegisterFuncs)) {
		SKSE::log::critical("Register papyrus methods failed");
		return false;
	}
	SKSE::log::info("Register papyrus methods Succeeded");

	return true;
}
