namespace Hooks
{
	namespace hkQuitGame
	{
		static bool QuitGame()
		{
			static REL::Relocation<void**> Console{ REL::ID(879277) };
			static REL::Relocation<void (*)(void*, const char*, ...)> ConsolePrint{ REL::ID(166359) };
			ConsolePrint(*Console.get(), "Bye.");

			std::thread(
				[]()
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					static REL::Relocation<void**> Main{ REL::ID(881027) };
					auto quitGame = SFSE::stl::adjust_pointer<bool>(*Main.get(), 0x28);
					*quitGame = true;
				})
				.detach();

			return true;
		}

		static void Install()
		{
			static REL::Relocation target{ REL::ID(110562) };
			target.replace_func(0x29, QuitGame);
		}
	}

	namespace hkShutdown
	{
		static void Shutdown()
		{
			REX::W32::TerminateProcess(REX::W32::GetCurrentProcess(), EXIT_SUCCESS);
		}

		static void Install()
		{
			static REL::Relocation target{ REL::ID(149084), 0xAB };
			target.write_call<5>(Shutdown);
		}
	}

	static void Install()
	{
		hkQuitGame::Install();
		hkShutdown::Install();
	}
}

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type)
		{
		case SFSE::MessagingInterface::kPostLoad:
		{
			Hooks::Install();
			break;
		}
		default:
			break;
		}
	}
}

SFSEPluginLoad(const SFSE::LoadInterface* a_sfse)
{
	SFSE::Init(a_sfse);

	SFSE::AllocTrampoline(16);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}
