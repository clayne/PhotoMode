#pragma once

#include "Styles.h"
#include "Util.h"

namespace ImGui
{
	bool ComboWithFilter(const char* label, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items = -1);

	bool FramelessImageButton(const char* str_id, ImTextureID user_texture_id, const ImVec2& image_size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));

	bool AlignedImage(ID3D11ShaderResourceView* texID, const ImVec2& texture_size, const ImVec2& min, const ImVec2& max, const ImVec2& align, ImU32 colour);

	std::tuple<bool, bool> CenteredTextWithArrows(const char* label, std::string_view centerText);

	bool CheckBox(const char* label, bool* a_toggle);

	template <class E>
	bool EnumSlider(const char* label, E* index, const std::ranges::common_range auto& a_enum, bool a_translate = true)
	{
		bool value_changed = false;

		std::size_t uIndex;
		if constexpr (std::is_enum_v<E>) {
			uIndex = std::to_underlying(*index);
		} else {
			uIndex = *index;
		}

		LeftAlignedTextImpl(label);
		auto [clickedLeft, clickedRight] = CenteredTextWithArrows(label, a_translate ? TRANSLATE(a_enum[uIndex]) : a_enum[uIndex]);

		if (IsWidgetFocused(label)) {
			const bool pressedLeft = clickedLeft || IsKeyPressed(ImGuiKey_LeftArrow) || IsKeyPressed(ImGuiKey_GamepadDpadLeft);
			const bool pressedRight = clickedRight || IsKeyPressed(ImGuiKey_RightArrow) || IsKeyPressed(ImGuiKey_GamepadDpadRight);
			if (pressedLeft) {
				uIndex = (uIndex - 1 + a_enum.size()) % a_enum.size();
			}
			if (pressedRight) {
				uIndex = (uIndex + 1) % a_enum.size();
			}
			if (pressedLeft || pressedRight) {
				value_changed = true;
				*index = static_cast<E>(uIndex);
				RE::PlaySound("UIMenuPrevNext");
			}
		}

		return value_changed;
	}

	bool BeginTabItemEx(const char* a_label, ImGuiTabItemFlags flags = 0);

	bool OutlineButton(const char* label, bool* wasFocused = nullptr);

	bool DragFloatEx(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags);
	bool DragIntEx(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags);

	template <class T>
	bool DragOnHover(const char* label, T* v, float v_speed = 1.0f, T v_min = 0, T v_max = 100, const char* format = nullptr, ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp)
	{
		const auto newLabel = LeftAlignedText(label);

		PushStyleColor(ImGuiCol_FrameBg, GetUserStyleColorU32(USER_STYLE::kFrameBG_Widget));
		PushStyleColor(ImGuiCol_FrameBgHovered, GetUserStyleColorU32(USER_STYLE::kFrameBG_WidgetActive));
		PushStyleColor(ImGuiCol_FrameBgActive, GetUserStyleColorU32(USER_STYLE::kFrameBG_WidgetActive));

		bool result;
		if constexpr (std::is_floating_point_v<T>) {
			result = ImGui::DragFloatEx(newLabel.c_str(), v, v_speed, v_min, v_max, format ? format : "%.2f", flags);
		} else {
			result = ImGui::DragIntEx(newLabel.c_str(), v, v_speed, v_min, v_max, format ? format : "%d", flags);
		}
		if (result) {
			RE::PlaySound("UIMenuPrevNext");
		}

		PopStyleColor(3);

		return result;
	}

	bool ThinSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags);
	bool ThinSliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags);

	template <class T>
	bool Slider(const char* label, T* v, T v_min, T v_max, const char* format = nullptr, ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp)
	{
		const auto newLabel = LeftAlignedText(label);

		PushStyleColor(ImGuiCol_FrameBg, GetUserStyleColorU32(USER_STYLE::kFrameBG_Widget));
		PushStyleColor(ImGuiCol_FrameBgHovered, GetUserStyleColorU32(USER_STYLE::kFrameBG_WidgetActive));
		PushStyleColor(ImGuiCol_FrameBgActive, GetUserStyleColorU32(USER_STYLE::kFrameBG_WidgetActive));

		bool result;
		if constexpr (std::is_floating_point_v<T>) {
			result = ImGui::ThinSliderFloat(newLabel.c_str(), v, v_min, v_max, format ? format : "%.2f", flags);
		} else {
			result = ImGui::ThinSliderInt(newLabel.c_str(), v, v_min, v_max, format ? format : "%d", flags);
		}
		if (result) {
			RE::PlaySound("UIMenuPrevNext");
		}

		PopStyleColor(3);

		return result;
	}
}
