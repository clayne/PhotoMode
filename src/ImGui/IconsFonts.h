#pragma once

namespace IconFont
{
	struct ImageData
	{
		ImageData() = default;
		ImageData(std::wstring_view a_iconName);
		~ImageData();

		bool Init();

		// members
		std::wstring              path{ LR"(Data\Interface\PhotoMode\Icons\)" };
		ID3D11ShaderResourceView* srView{ nullptr };
		ImVec2                    size{};
	};

	class Manager final : public ISingleton<Manager>
	{
	public:
		struct GamepadIcon
		{
			ImageData xbox;
			ImageData ps4;
		};

	    void LoadSettings(CSimpleIniA& a_ini);
		void LoadMCMSettings(const CSimpleIniA& a_ini);

		void LoadIcons();
		void LoadFonts();

		ImFont* GetLargeFont() const;

		const ImageData* GetStepperLeft() const;
		const ImageData* GetStepperRight() const;

		const ImageData*           GetIcon(std::uint32_t key);
		std::set<const ImageData*> GetIcons(const std::set<std::uint32_t>& keys);

		const ImageData* GetGamePadIcon(const GamepadIcon& a_icons) const;

	private:
		enum class BUTTON_SCHEME
		{
		    kAutoDetect,
			kXbox,
			kPS4
		};

	    ImFont* LoadFontIconPair(float a_fontSize, float a_iconSize, const ImVector<ImWchar>& a_ranges) const;

		// members
		bool loadedFonts{ false };

		std::string   fontName{ "Jost-Medium.ttf" };
		std::uint32_t fontSize{ 24 };
		std::uint32_t iconSize{ 20 };
		std::uint32_t largeFontSize{ 28 };
		std::uint32_t largeIconSize{ 24 };

		ImFont* largeFont{ nullptr };

		ImageData stepperLeft{ L"StepperLeft"sv };
		ImageData stepperRight{ L"StepperRight"sv };

		ImageData unknownKey{ L"UnknownKey"sv };

		ImageData leftKey{ L"Left"sv };
		ImageData rightKey{ L"Right"sv };
		ImageData upKey{ L"Up"sv };
		ImageData downKey{ L"Down"sv };

		Map<KEY, ImageData> keyboard{
			{ KEY::kTab, ImageData(L"Tab"sv) },
			{ KEY::kPageUp, ImageData(L"PgUp"sv) },
			{ KEY::kPageDown, ImageData(L"PgDown"sv) },
			{ KEY::kHome, ImageData(L"Home"sv) },
			{ KEY::kEnd, ImageData(L"End"sv) },
			{ KEY::kInsert, ImageData(L"Insert"sv) },
			{ KEY::kDelete, ImageData(L"Delete"sv) },
			{ KEY::kBackspace, ImageData(L"Backspace"sv) },
			{ KEY::kSpacebar, ImageData(L"Space"sv) },
			{ KEY::kEnter, ImageData(L"Enter"sv) },
			{ KEY::kEscape, ImageData(L"Esc"sv) },
			{ KEY::kLeftControl, ImageData(L"L-Ctrl"sv) },
			{ KEY::kLeftShift, ImageData(L"L-Shift"sv) },
			{ KEY::kLeftAlt, ImageData(L"L-Alt"sv) },
			//{ KEY::kLeftWin, ImageData(L"LeftWin"sv) },
			{ KEY::kRightControl, ImageData(L"R-Ctrl"sv) },
			{ KEY::kRightShift, ImageData(L"R-Shift"sv) },
			{ KEY::kRightAlt, ImageData(L"R-Alt"sv) },
			//{ KEY::kRightWin, ImageData(L"RightWin"sv) },
			{ KEY::kNum0, ImageData(L"0"sv) },
			{ KEY::kNum1, ImageData(L"1"sv) },
			{ KEY::kNum2, ImageData(L"2"sv) },
			{ KEY::kNum3, ImageData(L"3"sv) },
			{ KEY::kNum4, ImageData(L"4"sv) },
			{ KEY::kNum5, ImageData(L"5"sv) },
			{ KEY::kNum6, ImageData(L"6"sv) },
			{ KEY::kNum7, ImageData(L"7"sv) },
			{ KEY::kNum8, ImageData(L"8"sv) },
			{ KEY::kNum9, ImageData(L"9"sv) },
			{ KEY::kA, ImageData(L"A"sv) },
			{ KEY::kB, ImageData(L"B"sv) },
			{ KEY::kC, ImageData(L"C"sv) },
			{ KEY::kD, ImageData(L"D"sv) },
			{ KEY::kE, ImageData(L"E"sv) },
			{ KEY::kF, ImageData(L"F"sv) },
			{ KEY::kG, ImageData(L"G"sv) },
			{ KEY::kH, ImageData(L"H"sv) },
			{ KEY::kI, ImageData(L"I"sv) },
			{ KEY::kJ, ImageData(L"J"sv) },
			{ KEY::kK, ImageData(L"K"sv) },
			{ KEY::kL, ImageData(L"L"sv) },
			{ KEY::kM, ImageData(L"M"sv) },
			{ KEY::kN, ImageData(L"N"sv) },
			{ KEY::kO, ImageData(L"O"sv) },
			{ KEY::kP, ImageData(L"P"sv) },
			{ KEY::kQ, ImageData(L"Q"sv) },
			{ KEY::kR, ImageData(L"R"sv) },
			{ KEY::kS, ImageData(L"S"sv) },
			{ KEY::kT, ImageData(L"T"sv) },
			{ KEY::kU, ImageData(L"U"sv) },
			{ KEY::kV, ImageData(L"V"sv) },
			{ KEY::kW, ImageData(L"W"sv) },
			{ KEY::kX, ImageData(L"X"sv) },
			{ KEY::kY, ImageData(L"Y"sv) },
			{ KEY::kZ, ImageData(L"Z"sv) },
			{ KEY::kF1, ImageData(L"F1"sv) },
			{ KEY::kF2, ImageData(L"F2"sv) },
			{ KEY::kF3, ImageData(L"F3"sv) },
			{ KEY::kF4, ImageData(L"F4"sv) },
			{ KEY::kF5, ImageData(L"F5"sv) },
			{ KEY::kF6, ImageData(L"F6"sv) },
			{ KEY::kF7, ImageData(L"F7"sv) },
			{ KEY::kF8, ImageData(L"F8"sv) },
			{ KEY::kF9, ImageData(L"F9"sv) },
			{ KEY::kF10, ImageData(L"F10"sv) },
			{ KEY::kF11, ImageData(L"F11"sv) },
			{ KEY::kF12, ImageData(L"F12"sv) },
			{ KEY::kApostrophe, ImageData(L"Quotesingle"sv) },
			{ KEY::kComma, ImageData(L"Comma"sv) },
			{ KEY::kMinus, ImageData(L"Hyphen"sv) },
			{ KEY::kPeriod, ImageData(L"Period"sv) },
			{ KEY::kSlash, ImageData(L"Slash"sv) },
			{ KEY::kSemicolon, ImageData(L"Semicolon"sv) },
			{ KEY::kEquals, ImageData(L"Equal"sv) },
			{ KEY::kBracketLeft, ImageData(L"Bracketleft"sv) },
			{ KEY::kBackslash, ImageData(L"Backslash"sv) },
			{ KEY::kBracketRight, ImageData(L"Bracketright"sv) },
			{ KEY::kTilde, ImageData(L"Tilde"sv) },
			{ KEY::kCapsLock, ImageData(L"CapsLock"sv) },
			{ KEY::kScrollLock, ImageData(L"ScrollLock"sv) },
			//{ KEY::kNumLock, ImageData(L"NumLock"sv) },
			{ KEY::kPrintScreen, ImageData(L"PrintScreen"sv) },
			{ KEY::kPause, ImageData(L"Pause"sv) },
			{ KEY::kKP_0, ImageData(L"NumPad0"sv) },
			//{ KEY::kKP_1, ImageData(L"Keypad1"sv) },
			//{ KEY::kKP_2, ImageData(L"Keypad2"sv) },
			//{ KEY::kKP_3, ImageData(L"Keypad3"sv) },
			//{ KEY::kKP_4, ImageData(L"Keypad4"sv) },
			//{ KEY::kKP_5, ImageData(L"Keypad5"sv) },
			//{ KEY::kKP_6, ImageData(L"Keypad6"sv) },
			//{ KEY::kKP_7, ImageData(L"Keypad7"sv) },
			//{ KEY::kKP_8, ImageData(L"Keypad8"sv) },
			{ KEY::kKP_9, ImageData(L"NumPad9"sv) },
			{ KEY::kKP_Decimal, ImageData(L"NumPadDec"sv) },
			{ KEY::kKP_Divide, ImageData(L"NumPadDivide"sv) },
			{ KEY::kKP_Multiply, ImageData(L"NumPadMult"sv) },
			{ KEY::kKP_Subtract, ImageData(L"NumPadMinus"sv) },
			{ KEY::kKP_Plus, ImageData(L"NumPadPlus"sv) },
			//{ KEY::kKP_Enter, ImageData(L"KeypadEnter"sv) }
		};

		Map<std::uint32_t, GamepadIcon> gamePad{
			{ SKSE::InputMap::kGamepadButtonOffset_START, { ImageData(L"360_Start"sv), ImageData(L"PS3_Start"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_BACK, { ImageData(L"360_Back"sv), ImageData(L"PS3_Back"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_LEFT_THUMB, { ImageData(L"360_LS"sv), ImageData(L"PS3_L3"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_RIGHT_THUMB, { ImageData(L"360_RS"sv), ImageData(L"PS3_R3"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_LEFT_SHOULDER, { ImageData(L"360_LB"sv), ImageData(L"PS3_LB"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_RIGHT_SHOULDER, { ImageData(L"360_RB"sv), ImageData(L"PS3_RB"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_A, { ImageData(L"360_A"sv), ImageData(L"PS3_A"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_B, { ImageData(L"PS3_B"sv), ImageData(L"PS3_B"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_X, { ImageData(L"360_X"sv), ImageData(L"PS3_X"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_Y, { ImageData(L"360_Y"sv), ImageData(L"PS3_Y"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_LT, { ImageData(L"360_LT"sv), ImageData(L"PS3_LT"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_LT, { ImageData(L"360_RT"sv), ImageData(L"PS3_RT"sv) } },
		};

		BUTTON_SCHEME buttonScheme{ BUTTON_SCHEME::kAutoDetect };
	};
}

namespace ImGui
{
	ImVec2 ButtonIcon(std::uint32_t a_key);

	ImVec2 ButtonIcon(const IconFont::ImageData* a_imageData, bool a_centerIcon);
	void   ButtonIcon(const std::set<const IconFont::ImageData*>& a_imageData, bool a_centerIcon);

	void ButtonIconWithLabel(const char* a_text, const IconFont::ImageData* a_imageData, bool a_centerIcon);
	void ButtonIconWithLabel(const char* a_text, const std::set<const IconFont::ImageData*>& a_imageData, bool a_centerIcon);
}