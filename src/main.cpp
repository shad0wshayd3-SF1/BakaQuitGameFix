namespace Hooks
{
	class hkQuitGame :
		public REX::Singleton<hkQuitGame>
	{
	private:
		static bool QuitGame()
		{
			if (auto log = RE::ConsoleLog::GetSingleton())
			{
				log->PrintLine("Bye.");
			}

			std::thread(
				[]()
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(2000));
					if (auto main = RE::Main::GetSingleton())
					{
						main->quitGame = true;
					}
				})
				.detach();

			return true;
		}

	public:
		static void Install()
		{
			static REL::Relocation target{ REL::ID(66878) };
			target.replace_func(0x28, QuitGame);
		}
	};

	class hkShutdown :
		public REX::Singleton<hkShutdown>
	{
	private:
		static void Shutdown()
		{
			REX::W32::TerminateProcess(REX::W32::GetCurrentProcess(), EXIT_SUCCESS);
		}

		inline static REL::Hook Hook{ REL::ID(99375), 0xA9, Shutdown };
	};

	static void Install()
	{
		hkQuitGame::Install();
	}
}

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type)
		{
		case SFSE::MessagingInterface::kPostLoad:
			Hooks::Install();
			break;
		default:
			break;
		}
	}
}

SFSEPluginLoad(const SFSE::LoadInterface* a_sfse)
{
	SFSE::Init(a_sfse, { .trampoline = true });
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);
	return true;
}
