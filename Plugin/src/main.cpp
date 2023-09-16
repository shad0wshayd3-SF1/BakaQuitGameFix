namespace stl
{
	namespace detail
	{
		struct asm_patch :
			Xbyak::CodeGenerator
		{
			asm_patch(std::uintptr_t a_dst)
			{
				Xbyak::Label dst;

				jmp(ptr[rip + dst]);

				L(dst);
				dq(a_dst);
			}
		};
	}

	void asm_jump(std::uintptr_t a_from, [[maybe_unused]] std::size_t a_size, std::uintptr_t a_to)
	{
		detail::asm_patch p{ a_to };
		p.ready();
		assert(p.getSize() <= a_size);
		REL::safe_write(
			a_from,
			std::span{ p.getCode<const std::byte*>(), p.getSize() });
	}

	void asm_replace(std::uintptr_t a_from, std::size_t a_size, std::uintptr_t a_to)
	{
		REL::safe_fill(a_from, REL::INT3, a_size);
		asm_jump(a_from, a_size, a_to);
	}
}

class Hooks
{
public:
	static void Install()
	{
		hkQuitGame::Install();
		hkShutdown::Install();
	}

private:
	class hkQuitGame
	{
	public:
		static void Install()
		{
			static REL::Relocation<std::uintptr_t> target{ REL::Offset(0x01AE1440) };
			stl::asm_replace(target.address(), 0x29, reinterpret_cast<std::uintptr_t>(QuitGame));
		}

	private:
		static bool QuitGame()
		{
			static REL::Relocation<void**>                            Console{ REL::Offset(0x058F7A90) };
			static REL::Relocation<void (*)(void*, const char*, ...)> ConsolePrint{ REL::Offset(0x02883A14) };
			ConsolePrint(*Console.get(), "Bye.");

			std::thread(
				[]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					static REL::Relocation<std::byte**> Main{ REL::Offset(0x05906450) };
					auto quitGame = reinterpret_cast<bool*>((*Main.get()) + 0x28);
					*quitGame = true;
				})
				.detach();

			return true;
		}
	};

	class hkShutdown
	{
	public:
		static void Install()
		{
			static REL::Relocation<std::uintptr_t> target{ REL::Offset(0x023FE8A8), 0x77 };
			auto& trampoline = SFSE::GetTrampoline();
			trampoline.write_call<5>(target.address(), Shutdown);
		}

	private:
		static void Shutdown()
		{
			RE::WinAPI::TerminateProcess(RE::WinAPI::GetCurrentProcess(), EXIT_SUCCESS);
		}
	};
};

DLLEXPORT constinit auto SFSEPlugin_Version = []() noexcept {
	SFSE::PluginVersionData data{};

	data.PluginVersion(Plugin::Version);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);
	data.UsesSigScanning(false);
	//data.UsesAddressLibrary(true);
	data.HasNoStructUse(true);
	//data.IsLayoutDependent(true);
	data.CompatibleVersions({ SFSE::RUNTIME_LATEST });

	return data;
}();

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
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

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {
		Sleep(100);
	}
#endif

	SFSE::Init(a_sfse);

	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	SFSE::AllocTrampoline(1 << 6);

	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}