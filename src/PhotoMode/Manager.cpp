#include "Manager.h"

#include "Hotkeys.h"
#include "ImGui/IconsFonts.h"
#include "ImGui/Styles.h"
#include "ImGui/Widgets.h"
#include "Screenshots/Manager.h"

#include "Input.h"

namespace PhotoMode
{
	void Manager::Register()
	{
		tweenMenuInstalled = GetModuleHandle(L"TweenMenuOverhaul") != nullptr;
		improvedCameraInstalled = GetModuleHandle(L"ImprovedCameraSE.dll") != nullptr;
		skyrimSoulsInstalled = GetModuleHandle(L"SkyrimSoulsRE.dll") != nullptr;

		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(this);
		logger::info("Registered for menu open/close event");

		if (tweenMenuInstalled) {
			SKSE::GetModCallbackEventSource()->AddEventSink(this);
			logger::info("Registered for mod callback event");
		}
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		freeCameraSpeed = static_cast<float>(a_ini.GetDoubleValue("Settings", "fFreeCameraTranslationSpeed", freeCameraSpeed));
		freezeTimeOnStart = a_ini.GetBoolValue("Settings", "bFreezeTimeOnStart", freezeTimeOnStart);
		openFromPauseMenu = a_ini.GetBoolValue("Settings", "bOpenFromPauseMenu", openFromPauseMenu);
	}

	bool Manager::IsValid()
	{
		static constexpr std::array badMenus{
			RE::MainMenu::MENU_NAME,
			RE::MistMenu::MENU_NAME,
			RE::LoadingMenu::MENU_NAME,
			RE::FaderMenu::MENU_NAME,
			"LootMenu"sv,
			"CustomMenu"sv
		};

		const auto UI = RE::UI::GetSingleton();
		if (!UI || std::ranges::any_of(badMenus, [&](const auto& menuName) { return UI->IsMenuOpen(menuName); })) {
			return false;
		} 

		if (!GetValidControlMapContext() || RE::MenuControls::GetSingleton()->InBeastForm() || RE::VATS::GetSingleton()->mode == RE::VATS::VATS_MODE::kKillCam) {
			return false;
		}

		return true;
	}

	bool Manager::GetValidControlMapContext()
	{
		const auto* controlMap = RE::ControlMap::GetSingleton();
		if (!controlMap) {
			return false;
		}

		switch (controlMap->contextPriorityStack.back()) {
		case RE::UserEvents::INPUT_CONTEXT_ID::kGameplay:
		case RE::UserEvents::INPUT_CONTEXT_ID::kTFCMode:
		case RE::UserEvents::INPUT_CONTEXT_ID::kConsole:
			return true;
		case RE::UserEvents::INPUT_CONTEXT_ID::kCursor:
			return RE::UI::GetSingleton()->IsMenuOpen(RE::TweenMenu::MENU_NAME);
		default:
			return false;
		}
	}

	bool Manager::ShouldBlockInput() const
	{
		return blockInput;
	}

	bool Manager::IsActive() const
	{
		return activated;
	}

	bool Manager::IsHidden() const
	{
		return hiddenUI;
	}

	void Manager::ToggleUI()
	{
		hiddenUI = !hiddenUI;

		const auto UI = RE::UI::GetSingleton();
		UI->ShowMenus(!UI->IsShowingMenus());
		RE::PlaySound("UIMenuFocus");

		if (!hiddenUI) {
			restoreLastFocusID = true;
		}
	}

	void Manager::Activate()
	{
		RE::PlaySound("UIMenuOK");

		cameraTab.GetOriginalState();
		timeTab.GetOriginalState();

		const auto player = RE::PlayerCharacter::GetSingleton();
		characterTab.emplace(player->GetFormID(), Character(player));
		cachedCharacter = player;

		filterTab.GetOriginalState();

		const auto pcCamera = RE::PlayerCamera::GetSingleton();
		originalcameraState = pcCamera->currentState ? pcCamera->currentState->id : RE::CameraState::kThirdPerson;

		menusAlreadyHidden = !RE::UI::GetSingleton()->IsShowingMenus();
		if (menusAlreadyHidden) {
			hiddenUI = true;
		}

		// disable saving
		RE::PlayerCharacter::GetSingleton()->byCharGenFlag.set(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);

		// toggle freecam
		if (originalcameraState != RE::CameraState::kFree) {
			pcCamera->ToggleFreeCameraMode(false);
			//RE::ControlMap::GetSingleton()->PushInputContext(RE::ControlMap::InputContextID::kTFCMode);
		}

		// disable controls
		TogglePlayerControls(false);

		// apply mcm settings
		FreeCamera::translateSpeed = freeCameraSpeed;
		if (freezeTimeOnStart) {
			RE::Main::GetSingleton()->freezeTime = true;
		}

		// load default screenshot keys
		// keybindings can change?
		MANAGER(Input)->LoadDefaultKeys();

		// refresh style
		ImGui::Styles::GetSingleton()->RefreshStyle();

		activated = true;
		if (activeGlobal) {
			activeGlobal->value = 1.0f;
		}
	}

	void Manager::TogglePlayerControls(bool a_enable)
	{
		RE::ControlMap::GetSingleton()->ToggleControls(controlFlags, a_enable);

		if (const auto pcControls = RE::PlayerControls::GetSingleton()) {
			pcControls->readyWeaponHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->sneakHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->autoMoveHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->shoutHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->attackBlockHandler->SetInputEventHandlingEnabled(a_enable);
		}
	}

	bool Manager::OnFrameUpdate()
	{
		if (!IsValid()) {
			Deactivate();
			return false;
		}

		// disable controls
		if (ImGui::GetIO().WantTextInput) {
			if (!allowTextInput) {
				allowTextInput = true;
				RE::ControlMap::GetSingleton()->AllowTextInput(true);
			}
		} else if (allowTextInput) {
			allowTextInput = false;
			RE::ControlMap::GetSingleton()->AllowTextInput(false);
		}
		TogglePlayerControls(false);

		timeTab.OnFrameUpdate();

		return true;
	}

	void Manager::Deactivate()
	{
		Revert(true);

		//reset characters
		characterTab.clear();
		cachedCharacter = nullptr;

		// reset camera
		if (originalcameraState != RE::CameraState::kFree) {
			RE::PlayerCamera::GetSingleton()->ToggleFreeCameraMode(false);
			//RE::ControlMap::GetSingleton()->PopInputContext(RE::ControlMap::InputContextID::kTFCMode);
		}

		// reset controls
		allowTextInput = false;
		RE::ControlMap::GetSingleton()->AllowTextInput(false);
		TogglePlayerControls(true);

		// allow saving
		RE::PlayerCharacter::GetSingleton()->byCharGenFlag.reset(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);

		// reset variables
		hiddenUI = false;

		noItemsFocused = false;
		restoreLastFocusID = false;
		lastFocusedID = 0;

		updateKeyboardFocus = false;

		activated = false;
		if (activeGlobal) {
			activeGlobal->value = 0.0f;
		}

		RE::PlaySound("UIMenuCancel");
	}

	void Manager::ToggleActive()
	{
		if (!IsActive()) {
			if (IsValid() && !ShouldBlockInput()) {
				Activate();
			}
		} else {
			if (!ImGui::GetIO().WantTextInput && !ShouldBlockInput()) {
				Deactivate();
			}
		}
	}

	void Manager::Revert(bool a_deactivate)
	{
		const std::int32_t tabIndex = a_deactivate ? -1 : currentTab;

		// Camera
		if (tabIndex == -1 || tabIndex == kCamera) {
			cameraTab.RevertState(a_deactivate);
			if (!a_deactivate) {
				FreeCamera::translateSpeed = freeCameraSpeed;
			}
			revertENB = true;
		}
		// Time/Weather
		if (tabIndex == -1 || tabIndex == kTime) {
			timeTab.RevertState();
		}

		// Character
		if (tabIndex == kCharacter) {
			if (cachedCharacter) {
				characterTab[cachedCharacter->GetFormID()].RevertState();
			}
		} else if (tabIndex == -1) {
			std::ranges::for_each(characterTab, [](auto& data) {
				data.second.RevertState();
			});
		}

		// Filters
		if (tabIndex == -1 || tabIndex == kFilters) {
			filterTab.RevertState(tabIndex == -1);
		}
		// Overlays
		if (tabIndex == -1 || tabIndex == kOverlays) {
			overlaysTab.RevertOverlays();
		}

		if (a_deactivate) {
			// reset UI
			if ((!menusAlreadyHidden || hiddenUI) && !RE::UI::GetSingleton()->IsShowingMenus()) {
				RE::UI::GetSingleton()->ShowMenus(true);
			}
			resetWindow = true;
			resetPlayerTabs = true;
		} else {
			RE::PlaySound("UIMenuOK");

			const auto notification = std::format("{}", resetAll ? "$PM_ResetNotifAll"_T : TRANSLATE(tabResetNotifs[currentTab]));
			RE::DebugNotification(notification.c_str());

			if (resetAll) {
				resetAll = false;
			}
		}
	}

	bool Manager::GetResetAll() const
	{
		return resetAll;
	}

	void Manager::DoResetAll()
	{
		resetAll = true;
	}

	void Manager::NavigateTab(bool a_left)
	{
		const auto tabsSizeInt32 = tabs.size() > std::numeric_limits<uint32_t>::max() ?
		                               std::numeric_limits<uint32_t>::max() :
		                               static_cast<uint32_t>(tabs.size());
		if (a_left) {
			currentTab = (currentTab - static_cast<uint32_t>(1) + tabsSizeInt32) % tabsSizeInt32;
		} else {
			currentTab = (currentTab + static_cast<uint32_t>(1)) % tabsSizeInt32;
		}
		updateKeyboardFocus = true;
	}

	float Manager::GetViewRoll(const float a_fallback) const
	{
		return IsActive() ? cameraTab.GetViewRoll() : a_fallback;
	}

	void Manager::TryOpenFromTweenMenu()
	{
		if (openFromTweenMenu) {
			if (improvedCameraInstalled) {
				// prevent camera flicker on transition
				SKSE::GetTaskInterface()->AddTask([this]() {
					this->Activate();
					this->openFromTweenMenu = false;
				});
			} else {
				Activate();
				openFromTweenMenu = false;
			}
		}
	}

	void Manager::UpdateENBParams()
	{
		if (IsActive()) {
			cameraTab.UpdateENBParams();
		}
	}

	void Manager::RevertENBParams()
	{
		if (revertENB) {
			cameraTab.RevertENBParams();
			revertENB = false;
		}
	}

	void Manager::OnDataLoad()
	{
		overlaysTab.LoadOverlays();

		activeGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>("PhotoMode_IsActive");
		resetRootIdle = RE::TESForm::LookupByEditorID<RE::TESIdleForm>("ResetRoot");
	}

	std::pair<ImGui::Texture*, float> Manager::GetOverlay() const
	{
		return overlaysTab.GetCurrentOverlay();
	}

	void Manager::Draw()
	{
		ImGui::SetNextWindowPos(ImGui::GetNativeViewportPos());
		ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize());

		ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
		{
			// render hierachy
			overlaysTab.DrawOverlays();

			if (!IsHidden()) {
				CameraGrid::Draw();
				DrawBar();
				DrawControls();
			}

			if (ImGui::IsKeyReleased(ImGuiKey_Escape) || ImGui::IsKeyReleased(ImGuiKey_GamepadFaceRight)) {
				if (IsHidden() || noItemsFocused) {
					Deactivate();
					RE::PlaySound("UIMenuCancel");
				}
			}
		}
		ImGui::End();
	}

	void Manager::DrawControls()
	{
		const static auto center = ImGui::GetNativeViewportCenter();
		const static auto size = ImGui::GetNativeViewportSize();

		const static auto third_width = size.x / 3;
		const static auto third_height = size.y / 3;

		ImGui::SetNextWindowPos(ImVec2(center.x + third_width, center.y + third_height * 0.8f), ImGuiCond_Always, ImVec2(0.5, 0.5));
		ImGui::SetNextWindowSize(ImVec2(size.x / 3.25f, size.y / 3.125f));

		constexpr auto windowFlags = ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoDecoration;

		ImGui::Begin("$PM_Title_Menu"_T, nullptr, windowFlags);
		{
			ImGui::ExtendWindowPastBorder();

			if (resetWindow) {
				currentTab = kCamera;
			}

			// Q [Tab Tab Tab Tab Tab] E
			ImGui::BeginGroup();
			{
				const auto buttonSize = ImGui::ButtonIcon(MANAGER(Hotkeys)->PreviousTabKey());
				ImGui::SameLine();

				const float tabWidth = (ImGui::GetContentRegionAvail().x - (buttonSize.x + ImGui::GetStyle().ItemSpacing.x * tabs.size())) / tabs.size();

				ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				for (std::int32_t i = 0; i < tabs.size(); ++i) {
					if (currentTab != i) {
						ImGui::BeginDisabled(true);
					} else {
						ImGui::PushFont(MANAGER(IconFont)->GetLargeFont());
					}
					ImGui::Button(tabIcons[i], ImVec2(tabWidth, ImGui::GetFrameHeightWithSpacing()));
					if (currentTab != i) {
						ImGui::EndDisabled();
					} else {
						ImGui::PopFont();
					}
					ImGui::SameLine();
				}
				ImGui::PopStyleColor();
				ImGui::PopItemFlag();

				ImGui::SameLine();
				ImGui::ButtonIcon(MANAGER(Hotkeys)->NextTabKey());
			}
			ImGui::EndGroup();

			//		CAMERA
			// ----------------
			ImGui::CenteredText(currentTab != TAB_TYPE::kCharacter ? TRANSLATE(tabs[currentTab]) : characterTab[cachedCharacter->GetFormID()].GetName());
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));

			// content
			ImGui::SetNextWindowBgAlpha(0.0f);  // child bg color is added ontop of window
			ImGui::BeginChild("##PhotoModeChild", ImVec2(0, 0), false, windowFlags);
			{
				ImGui::Spacing();

				if (restoreLastFocusID) {
					ImGui::SetFocusID(lastFocusedID, ImGui::GetCurrentWindow());

					restoreLastFocusID = false;
				} else if (updateKeyboardFocus) {
					if (currentTab == TAB_TYPE::kCharacter) {
						resetPlayerTabs = true;
					}

					ImGui::SetKeyboardFocusHere();
					RE::PlaySound("UIJournalTabsSD");

					updateKeyboardFocus = false;
				}

				switch (currentTab) {
				case TAB_TYPE::kCamera:
					{
						if (resetWindow) {
							ImGui::SetKeyboardFocusHere();
							resetWindow = false;
						}
						cameraTab.Draw();
					}
					break;
				case TAB_TYPE::kTime:
					timeTab.Draw();
					break;
				case TAB_TYPE::kCharacter:
					{
						const auto consoleRef = RE::Console::GetSelectedRef();
						if (!consoleRef || !consoleRef->Is(RE::FormType::ActorCharacter) || consoleRef->IsDisabled() || consoleRef->IsDeleted() || !consoleRef->Is3DLoaded()) {
							prevCachedCharacter = cachedCharacter;
							cachedCharacter = RE::PlayerCharacter::GetSingleton();
						} else {
							prevCachedCharacter = cachedCharacter;
							cachedCharacter = consoleRef->As<RE::Actor>();
							if (!characterTab.contains(cachedCharacter->GetFormID())) {
								characterTab.emplace(cachedCharacter->GetFormID(), Character(cachedCharacter));
							}
						}

						if (cachedCharacter != prevCachedCharacter) {
							resetPlayerTabs = true;
						}

						characterTab[cachedCharacter->GetFormID()].Draw(resetPlayerTabs);

						if (resetPlayerTabs) {
							resetPlayerTabs = false;
						}
					}
					break;
				case TAB_TYPE::kFilters:
					filterTab.Draw();
					break;
				case TAB_TYPE::kOverlays:
					overlaysTab.Draw();
					break;
				default:
					break;
				}

				noItemsFocused = !ImGui::IsAnyItemFocused() || !ImGui::IsWindowFocused();
				lastFocusedID = ImGui::GetFocusID();
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	void Manager::DrawBar() const
	{
		const static auto center = ImGui::GetNativeViewportCenter();
		const static auto size = ImGui::GetNativeViewportSize();

		const static auto offset = size.y / 20.25f;

		ImGui::SetNextWindowPos(ImVec2(center.x, size.y - offset), ImGuiCond_Always, ImVec2(0.5, 0.5));

		ImGui::Begin("##Bar", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);  // same offset as control window
		{
			ImGui::ExtendWindowPastBorder();

			const static auto takePhotoLabel = "$PM_TAKEPHOTO"_T;
			const static auto toggleMenusLabel = "$PM_TOGGLEMENUS"_T;
			const auto        resetLabel = GetResetAll() ? "$PM_RESET_ALL"_T : "$PM_RESET"_T;
			const static auto freezeTimeLabel = "$PM_FREEZETIME"_T;

			const auto& takePhotoIcon = MANAGER(Hotkeys)->TakePhotoIcon();
			const auto& toggleMenusIcon = MANAGER(Hotkeys)->ToggleMenusIcon();
			const auto& resetIcon = MANAGER(Hotkeys)->ResetIcon();
			const auto& freezeTimeIcon = MANAGER(Hotkeys)->FreezeTimeIcon();

			// const static auto togglePMLabel = "$PM_EXIT"_T;
			// const auto& togglePMIcons = MANAGER(Hotkeys)->TogglePhotoModeIcons();

			// calc total elements width
			const ImGuiStyle& style = ImGui::GetStyle();

			float width = 0.0f;

			const auto calc_width = [&](const IconFont::IconTexture* a_icon, const char* a_textLabel) {
				width += a_icon->size.x;
				width += style.ItemSpacing.x;
				width += ImGui::CalcTextSize(a_textLabel).x;
				width += style.ItemSpacing.x;
			};

			calc_width(takePhotoIcon, takePhotoLabel);
			calc_width(toggleMenusIcon, toggleMenusLabel);
			calc_width(freezeTimeIcon, freezeTimeLabel);
			calc_width(resetIcon, resetLabel);

			/*for (const auto& icon : togglePMIcons) {
				width += icon->size.x;
			}
			width += style.ItemSpacing.x;
			width += ImGui::CalcTextSize(togglePMLabel).x;*/

			// align at center
			ImGui::AlignForWidth(width);

			// draw
			constexpr auto draw_button = [](const IconFont::IconTexture* a_icon, const char* a_textLabel) {
				ImGui::ButtonIconWithLabel(a_textLabel, a_icon, true);
				ImGui::SameLine();
			};

			draw_button(takePhotoIcon, takePhotoLabel);
			draw_button(toggleMenusIcon, toggleMenusLabel);
			draw_button(freezeTimeIcon, freezeTimeLabel);
			draw_button(resetIcon, resetLabel);

			// ImGui::ButtonIconWithLabel(togglePMLabel, togglePMIcons, true);
		}
		ImGui::End();
	}

	bool Manager::SetupJournalMenu() const
	{
		const auto menu = RE::UI::GetSingleton()->GetMenu<RE::JournalMenu>(RE::JournalMenu::MENU_NAME);
		const auto view = menu ? menu->systemTab.view : nullptr;

		RE::GFxValue page;
		if (!view || !view->GetVariable(&page, "_root.QuestJournalFader.Menu_mc.SystemFader.Page_mc")) {
			return false;
		}

		// in case someone packed the files into a BSA
		static bool dearDiaryExists = RE::BSResourceNiBinaryStream(R"(interface\deardiary_dm\config.txt)").good() || RE::BSResourceNiBinaryStream(R"(interface\deardiary\config.txt)").good();

		// Dear Diary SetShowMod function is broken af, need to do it manually
		if (dearDiaryExists) {
			RE::GFxValue categoryList;
			if (page.GetMember("CategoryList", &categoryList)) {
				RE::GFxValue entryList;
				if (categoryList.GetMember("entryList", &entryList)) {
					std::vector<std::string> elements;

					entryList.VisitMembers([&](const char*, const RE::GFxValue& a_value) {
						RE::GFxValue textVal;
						a_value.GetMember("text", &textVal);
						elements.push_back(textVal.GetString());
					});

					RE::GFxValue showModMenu;
					if (page.GetMember("_showModMenu", &showModMenu) && !showModMenu.GetBool()) {
						page.SetMember("_showModMenu", true);
					} else {
						std::erase(elements, "$MOD MANAGER");
					}

					auto index = std::ranges::contains(elements, "$QUICKSAVE") ? 3 : 2;
					elements.insert(elements.begin() + index, "$PM_Title_Menu");

					entryList.ClearElements();
					for (auto& element : elements) {
						RE::GFxValue entry;
						view->CreateObject(&entry);
						entry.SetMember("text", element.c_str());
						entryList.PushBack(entry);
					}

					categoryList.Invoke("InvalidateData");

					return true;
				}
			}

		} else {
			RE::GFxValue showModMenu;
			if (page.GetMember("_showModMenu", &showModMenu) && !showModMenu.GetBool()) {
				std::array<RE::GFxValue, 1> args;
				args[0] = true;
				if (!page.Invoke("SetShowMod", nullptr, args.data(), args.size())) {
					return false;
				}
			}

			RE::GFxValue categoryList;
			if (page.GetMember("CategoryList", &categoryList)) {
				RE::GFxValue entryList;
				if (categoryList.GetMember("entryList", &entryList)) {
					std::optional<std::uint32_t> modMenuIndex = std::nullopt;

					std::uint32_t index = 0;
					std::string   text;
					entryList.VisitMembers([&](const char*, const RE::GFxValue& a_value) {
						RE::GFxValue textVal;
						a_value.GetMember("text", &textVal);
						if (text = textVal.GetString(); text == "$MOD MANAGER") {
							modMenuIndex = index;
						}
						index++;
					});

					if (modMenuIndex) {
						RE::GFxValue entry;
						view->CreateObject(&entry);
						entry.SetMember("text", "$PM_Title_Menu");

						entryList.SetElement(*modMenuIndex, entry);
						categoryList.Invoke("InvalidateData");

						return true;
					}
				}
			}
		}

		return false;
	}

	EventResult Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_evn) {
			return EventResult::kContinue;
		}

		if (a_evn->menuName == RE::Console::MENU_NAME) {
			blockInput = a_evn->opening;
			if (a_evn->opening && IsHidden()) {
				ToggleUI();
			}
		} else if (a_evn->opening) {
			if (a_evn->menuName == RE::JournalMenu::MENU_NAME) {
				if (openFromPauseMenu) {
					openFromPauseMenu = SetupJournalMenu();
				}
			} else if (a_evn->menuName == RE::ModManagerMenu::MENU_NAME) {
				if (RE::UI::GetSingleton()->IsMenuOpen(RE::JournalMenu::MENU_NAME) && openFromPauseMenu) {
					const auto msgQueue = RE::UIMessageQueue::GetSingleton();

					msgQueue->AddMessage(RE::ModManagerMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
					msgQueue->AddMessage(RE::JournalMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

					Activate();
				}
			}
		}

		return EventResult::kContinue;
	}

	EventResult Manager::ProcessEvent(const SKSE::ModCallbackEvent* a_evn, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
	{
		if (a_evn && a_evn->eventName == "OpenTween_PhotoMode" && !IsActive() && !openFromTweenMenu) {
			openFromTweenMenu = true;
			if (skyrimSoulsInstalled) {
				Activate();
				openFromTweenMenu = false;
			}
		}

		return EventResult::kContinue;
	}
}
