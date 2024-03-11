#pragma once
#ifndef IMGUI_UTILS_H
#define IMGUI_UTILS_H

#include <imgui.h>
#include <cstdint>

namespace ImGuiUtils
{
	static inline uint32_t ConvertVecToColor32(const ImVec4& color)
	{
		return IM_COL32(color.x * 255, color.y * 255, color.z * 255, color.w * 255);
	}

	static inline void DrawRect(const ImVec2& pos, const ImVec2& size, uint32_t color, const uint32_t thickness = 14)
	{
		ImGui::GetWindowDrawList()->AddRect({pos.x - size.x / 2, pos.y - size.y / 2},
											{pos.x + size.x / 2, pos.y + size.y / 2}, color, 0, 1);
	}

	static inline void DrawRectFilled(const ImVec2& pos, const ImVec2& size, uint32_t color, uint32_t fillColor,
									  const uint32_t thickness = 14)
	{
		ImGui::GetWindowDrawList()->AddRect({pos.x - size.x / 2, pos.y - size.y / 2},
											{pos.x + size.x / 2, pos.y + size.y / 2}, color, 0, 1);
		ImGui::GetWindowDrawList()->AddRectFilled(
			{pos.x - size.x / 2 + thickness / 2, pos.y - size.y / 2 + thickness / 2},
			{pos.x + size.x / 2 - thickness / 2, pos.y + size.y / 2 - thickness / 2}, fillColor, 0, 1);
	}

	static inline void DrawRect(const ImVec2& pos, const ImVec2& size, const ImVec4& color,
								const uint32_t thickness = 14)
	{
		DrawRect(pos, size, ConvertVecToColor32(color), thickness);
	}

	static inline void DrawRectFilled(const ImVec2& pos, const ImVec2& size, const ImVec4& color,
									  const ImVec4& fillColor, const uint32_t thickness = 14)
	{
		DrawRectFilled(pos, size, ConvertVecToColor32(color), ConvertVecToColor32(fillColor), thickness);
	}

	static inline void TextCentered(const char* msg, ...)
	{
		va_list va;
		va_start(va, msg);
		char* buff = (char*)alloca(vsnprintf(nullptr, 0, msg, va) + 1);
		vsprintf(buff, msg, va);
		va_end(va);

		const ImVec2 textSize = ImGui::CalcTextSize(buff);
		ImGui::SetCursorPosX((ImGui::GetCursorPosX() - textSize.x * 0.5f));
		ImGui::SetCursorPosY((ImGui::GetCursorPosY() - textSize.y * 0.5f));
		ImGui::Text(buff);
	}

	static inline std::string _labelPrefix(const char* const label)
	{
		float width = ImGui::CalcItemWidth();

		float x = ImGui::GetCursorPosX();
		ImGui::Text(label);
		ImGui::SameLine();
		ImGui::SetCursorPosX(x + width * 0.5f + ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::SetNextItemWidth(-1);

		std::string labelID = "##";
		labelID += label;

		return labelID;
	}
} // namespace ImGuiUtils

#endif
