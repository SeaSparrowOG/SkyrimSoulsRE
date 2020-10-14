#include "Menus/Hooks_SleepWaitMenu.h"
#include "SlowMotionHandler.h"

namespace SkyrimSoulsRE
{
	RE::UI_MESSAGE_RESULTS SleepWaitMenuEx::ProcessMessage_Hook(RE::UIMessage& a_message)
	{
		if (a_message.type == RE::UI_MESSAGE_TYPE::kScaleformEvent)
		{
			return this->RE::IMenu::ProcessMessage(a_message);
		}
		return _ProcessMessage(this, a_message);
	}

	void SleepWaitMenuEx::AdvanceMovie_Hook(float a_interval, std::uint32_t a_currentTime)
	{
		if (!(RE::UI::GetSingleton()->GameIsPaused()))
		{
			this->UpdateClock();
		}
		return _AdvanceMovie(this, a_interval, a_currentTime);
	}

	void SleepWaitMenuEx::UpdateClock()
	{
		void(*GetTimeString)(RE::Calendar * a_this, char* a_str, std::uint64_t a_bufferSize, bool a_showYear) = reinterpret_cast<void(*)(RE::Calendar*, char*, std::uint64_t, bool)>(Offsets::Calendar::GetTimeString.address());

		char buf[200];
		GetTimeString(RE::Calendar::GetSingleton(), buf, 200, false);

		RE::GFxValue dateText;
		this->uiMovie->GetVariable(&dateText, "_root.SleepWaitMenu_mc.CurrentTime");
		RE::GFxValue newDate(buf);
		dateText.SetMember("htmlText", newDate);
	}

	void SleepWaitMenuEx::StartSleepWait_Hook(const RE::FxDelegateArgs& a_args)
	{
		class StartSleepWaitTask : public UnpausedTask
		{
			double sleepWaitTime;

		public:
			StartSleepWaitTask(double a_time)
			{
				this->sleepWaitTime = a_time;
				this->beginTime = std::chrono::high_resolution_clock::now();
				this->delayTimeMS = std::chrono::milliseconds(0);
			}

			void Run() override
			{
				RE::UI* ui = RE::UI::GetSingleton();

				if (ui->IsMenuOpen(RE::SleepWaitMenu::MENU_NAME))
				{
					RE::SleepWaitMenu* menu = static_cast<RE::SleepWaitMenu*>(ui->GetMenu(RE::SleepWaitMenu::MENU_NAME).get());
					RE::GFxValue time = this->sleepWaitTime;
					RE::FxDelegateArgs args(0, menu, menu->uiMovie.get(), &time, 1);
					_StartSleepWait(args);
				}
			}
		};

		RE::UI* ui = RE::UI::GetSingleton();
		Settings* settings = Settings::GetSingleton();

		static_cast<RE::SleepWaitMenu*>(a_args.GetHandler())->menuFlags |= RE::IMenu::Flag::kPausesGame;
		ui->numPausesGame++;

		if (SkyrimSoulsRE::SlowMotionHandler::isInSlowMotion)
		{
			SlowMotionHandler::DisableSlowMotion();
		}

		StartSleepWaitTask* task = new StartSleepWaitTask(a_args[0].GetNumber());
		UnpausedTaskQueue* queue = UnpausedTaskQueue::GetSingleton();
		queue->AddTask(task);
	}

	RE::IMenu* SleepWaitMenuEx::Creator()
	{
		RE::SleepWaitMenu* menu = static_cast<RE::SleepWaitMenu*>(CreateMenu(RE::SleepWaitMenu::MENU_NAME));

		RE::FxDelegate* dlg = menu->fxDelegate.get();
		_StartSleepWait = dlg->callbacks.GetAlt("OK")->callback;
		dlg->callbacks.GetAlt("OK")->callback = StartSleepWait_Hook;

		return menu;
	}
	void SleepWaitMenuEx::InstallHook()
	{
		//Hook AdvanceMovie
		REL::Relocation<std::uintptr_t> vTable(Offsets::Menus::SleepWaitMenu::Vtbl);
		_AdvanceMovie = vTable.write_vfunc(0x5, &SleepWaitMenuEx::AdvanceMovie_Hook);

		//Hook ProcessMessage
		_ProcessMessage = vTable.write_vfunc(0x4, &SleepWaitMenuEx::ProcessMessage_Hook);
	}
}