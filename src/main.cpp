namespace util
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

		static void asm_jump(std::uintptr_t a_from, [[maybe_unused]] std::size_t a_size, std::uintptr_t a_to)
		{
			asm_patch p{ a_to };
			p.ready();
			assert(p.getSize() <= a_size);
			REL::safe_write(
				a_from,
				std::span{ p.getCode<const std::byte*>(), p.getSize() });
		}
	}

	static void asm_replace(std::uintptr_t a_from, std::size_t a_size, std::uintptr_t a_to)
	{
		REL::safe_fill(a_from, REL::INT3, a_size);
		detail::asm_jump(a_from, a_size, a_to);
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
			static REL::Relocation<std::uintptr_t> target{ REL::ID(110562) };
			util::asm_replace(target.address(), 0x29, reinterpret_cast<std::uintptr_t>(QuitGame));
		}

	private:
		static bool QuitGame()
		{
			static REL::Relocation<void**> Console{ REL::ID(879277) };
			static REL::Relocation<void (*)(void*, const char*, ...)> ConsolePrint{ REL::ID(166359) };
			ConsolePrint(*Console.get(), "Bye.");

			std::thread(
				[]()
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					static REL::Relocation<std::byte**> Main{ REL::ID(881027) };
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
			static REL::Relocation<std::uintptr_t> target{ REL::ID(149084), 0xAB };
			auto& trampoline = SFSE::GetTrampoline();
			trampoline.write_call<5>(target.address(), Shutdown);
		}

	private:
		static void Shutdown()
		{
			REX::W32::TerminateProcess(REX::W32::GetCurrentProcess(), EXIT_SUCCESS);
		}
	};
};

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
