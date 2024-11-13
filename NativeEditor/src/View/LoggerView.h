#pragma once
#include "../src/Utils/Logger.h"

namespace LoggerView {
	class LoggerView {
	public:
		static LoggerView& Get() {
			static LoggerView instance;
			return instance;
		}

		void Draw();
		bool& GetShowState() { return m_show; }

	private:
		LoggerView() = default;
		~LoggerView() = default;

		bool m_show = true;
		bool m_showInfo = true;
		bool m_showWarnings = true;
		bool m_showErrors = true;
		bool m_autoScroll = true;

	};
}
