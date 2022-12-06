#include "memory.h"
#include "offsets.h"
#include "skinchanger.h"
#include "vector.h"
#include "gui.h"
#include <thread>
#include <array>
#include <iostream>
#include <ctime>
#include "color.h"

using namespace std;

time_t updateTimer = 0;

constexpr Vector3 CalculateAngle(
	const Vector3& localPosition,
	const Vector3& enemyPosition,
	const Vector3& viewAngles) noexcept
{
	return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

struct DoubleBool {
	constexpr DoubleBool(bool a, bool b) noexcept :
		a(a), b(b) {}

	bool a, b;
};

struct WriteColor {
	constexpr WriteColor(float r, float g, float b, float a) noexcept :
		r(r), g(g), b(b), a(a) {}

	float r, g, b, a;
};

int __stdcall wWinMain(
	HINSTANCE instance,
	HINSTANCE previousInstance,
	PWSTR arguments,
	int commandShow)
{
	gui::CreateHWindow("CSGO External Hack");
	gui::CreateDevice();
	gui::CreateImGui();

	const auto memory = Memory{ "csgo.exe" };

	const auto client = memory.GetModuleAddress("client.dll");
	const auto engine = memory.GetModuleAddress("engine.dll");

	const auto TrueTrue = DoubleBool(true, true);
	auto oldPunch = Vector2{};

	while (gui::isRunning)
	{
		gui::BeginRender();
		gui::Render();
		gui::EndRender();
		const auto& localPlayer = memory.Read<std::uintptr_t>(client + offsets::dwLocalPlayer);

		if (!localPlayer)
			continue;

		const auto& localHealth = memory.Read<std::int32_t>(localPlayer + offsets::m_iHealth);
		const auto& localTeam = memory.Read<std::uintptr_t>(localPlayer + offsets::m_iTeamNum);
		const auto& glowObjectManager = memory.Read<std::uintptr_t>(client + offsets::dwGlowObjectManager);
		const auto& shotsFired = memory.Read<std::int32_t>(localPlayer + offsets::m_iShotsFired);
		const auto& weapons = memory.Read<std::array<unsigned long, 8>>(localPlayer + offsets::m_hMyWeapons);

		bool shouldSleep = true;
		time_t curtime = time(NULL);

		if (g_Options.bhop[0]) {
			const auto& flags = memory.Read<bool>(localPlayer + offsets::m_fFlags);

			if (!GetAsyncKeyState(VK_SPACE))
				goto skip_bhop;

			updateTimer = curtime + 1;

			if (flags & (1 << 0) && rand() % 100 + 1 <= g_Options.bhop_chance[0])
				memory.Write<BYTE>(client + offsets::dwForceJump, 6);
		}

		skip_bhop:

		if (g_Options.trigger[0] && GetAsyncKeyState(5)) {
			updateTimer = curtime + 1; 
			memory.Write<std::uintptr_t>(client + offsets::dwForceAttack, 4);
			const auto& crosshairID = memory.Read<int32_t>(localPlayer + offsets::m_iCrosshairId);
			const auto& entity = memory.Read<std::uintptr_t>(client + offsets::dwEntityList + (crosshairID - 1) * 0x10);

			if (!localHealth)
				goto skip_trigger;

			if (!crosshairID || crosshairID > 64)
				goto skip_trigger;

			if (localTeam == memory.Read<std::uintptr_t>(entity + offsets::m_iTeamNum))
				goto skip_trigger;

			Sleep(g_Options.trigger_delay[0]);

			memory.Write<std::uintptr_t>(client + offsets::dwForceAttack, 6);
		};

		skip_trigger:

		if (shotsFired > 1 && g_Options.recoil[0]) {
			const auto& clientState = memory.Read<std::uintptr_t>(engine + offsets::dwClientState);
			const auto& viewAngles = memory.Read<Vector2>(clientState + offsets::dwClientState_ViewAngles);

			const auto& aimPunch = memory.Read<Vector2>(localPlayer + offsets::m_aimPunchAngle);

			const float x = 2 - g_Options.recoil_smooth_x[0] * 0.01f;
			const float y = 2 - g_Options.recoil_smooth_y[0] * 0.01f;

			auto newAngles = Vector2{
				viewAngles.x + oldPunch.x - aimPunch.x * x,
				viewAngles.y + oldPunch.y - aimPunch.y * y,
			};

			if (newAngles.x > 89.f)
				newAngles.x = 89.f;

			if (newAngles.x < -89.f)
				newAngles.x = -89.f;

			while (newAngles.y > 180.f)
				newAngles.y -= 360.f;

			while (newAngles.y < -180.f)
				newAngles.y += 360.f;

			memory.Write<Vector2>(clientState + offsets::dwClientState_ViewAngles, newAngles);

			oldPunch.x = aimPunch.x * x;
			oldPunch.y = aimPunch.y * y;
		}
		else {
			oldPunch.x = oldPunch.y = 0.f;
		}

		if (GetAsyncKeyState(6) && g_Options.aim[0]) {
			const auto& clientState = memory.Read<std::uintptr_t>(engine + offsets::dwClientState);
			const auto& localPlayerId = memory.Read<std::int32_t>(clientState + offsets::dwClientState_GetLocalPlayer);
			const auto& localEyePosition = memory.Read<Vector3>(localPlayer + offsets::m_vecOrigin) + memory.Read<Vector3>(localPlayer + offsets::m_vecViewOffset);
			const auto& viewAngles = memory.Read<Vector3>(clientState + offsets::dwClientState_ViewAngles);
			const auto& aimPunch = memory.Read<Vector3>(localPlayer + offsets::m_aimPunchAngle) * 2;

			auto bestFov = g_Options.aim_fov[0] * 3.6;
			auto bestAngle = Vector3();
			auto smooth = 0.1f * g_Options.aim_smooth[0] * 5 + 1;
			auto smoothAftershot = 0.1f * g_Options.aim_smooth_after_shot[0] * 5 + 1;

			for (auto i = 1; i <= 32; ++i)
			{
				const auto& player = memory.Read<std::uintptr_t>(client + offsets::dwEntityList + i * 0x10);

				if (memory.Read<std::int32_t>(player + offsets::m_iTeamNum) == localTeam)
					continue;

				if (memory.Read<bool>(player + offsets::m_bDormant))
					continue;

				if (memory.Read<std::int32_t>(player + offsets::m_lifeState))
					continue;

				if (memory.Read<std::int32_t>(player + offsets::m_bSpottedByMask) & (1 << localPlayerId))
				{
					const auto& boneMatrix = memory.Read<std::uintptr_t>(player + offsets::m_dwBoneMatrix);
					const auto& cachedMemory = boneMatrix + 0x30 * 8;

					const auto playerHeadPosition = Vector3{
						memory.Read<float>(cachedMemory + 0x0C),
						memory.Read<float>(cachedMemory + 0x1C),
						memory.Read<float>(cachedMemory + 0x2C)
					};

					const auto& angle = CalculateAngle(
						localEyePosition,
						playerHeadPosition,
						viewAngles + aimPunch
					);

					const auto& fov = std::hypot(angle.x, angle.y);

					if (fov < bestFov)
					{
						if (bestAngle.x > 89.f)
							bestAngle.x = 89.f;

						if (bestAngle.x < -89.f)
							bestAngle.x = -89.f;

						while (bestAngle.y > 180.f)
							bestAngle.y -= 360.f;

						while (bestAngle.y < -180.f)
							bestAngle.y += 360.f;

						bestFov = fov;
						bestAngle = angle;
					}
				}
			}

			if (!bestAngle.IsZero())
				if (shotsFired > 1)
					memory.Write<Vector3>(clientState + offsets::dwClientState_ViewAngles, viewAngles + bestAngle / smoothAftershot);
				else
					memory.Write<Vector3>(clientState + offsets::dwClientState_ViewAngles, viewAngles + bestAngle / smooth);
		}

		auto enemyMenuCol = g_Options.glow_color[0];
		const auto& enemyColor = WriteColor(enemyMenuCol.r() / 255.f, enemyMenuCol.g() / 255.f, enemyMenuCol.b() / 255.f, enemyMenuCol.a() / 255.f);

		if (g_Options.glow[0]) {
			const auto radar = g_Options.radar[0];

			for (auto i = 0; i < 64; ++i) {
				const auto& entity = memory.Read<std::uintptr_t>(client + offsets::dwEntityList + i * 0x10);

				if (localTeam == memory.Read<std::uintptr_t>(entity + offsets::m_iTeamNum))
					continue;

				if (radar)
					memory.Write<bool>(entity + offsets::m_bSpotted, true);

				const auto& glowIndex = memory.Read<std::int32_t>(entity + offsets::m_iGlowIndex);
				const auto& cachedMemory = glowObjectManager + (glowIndex * 0x38);

				memory.Write<WriteColor>(cachedMemory + 0x8, enemyColor);
				memory.Write<DoubleBool>(cachedMemory + 0x27, TrueTrue);
			}
		}

		for (const auto& handle : weapons)
		{
			const auto& weapon = memory.Read<std::uintptr_t>((client + offsets::dwEntityList + (handle & 0xFFF) * 0x10) - 0x10);

			if (!weapon)
				continue;

			if (const auto& paint = GetWeaponPaint(memory.Read<short>(weapon + offsets::m_iItemDefinitionIndex)))
			{
				const bool shouldUpdate = memory.Read<std::int32_t>(weapon + offsets::m_nFallbackPaintKit) != paint;

				memory.Write<std::int32_t>(weapon + offsets::m_iItemIDHigh, -1);
				memory.Write<std::int32_t>(weapon + offsets::m_nFallbackPaintKit, paint);
				memory.Write<float>(weapon + offsets::m_flFallbackWear, 0.f);
				memory.Write<std::int32_t>(weapon + offsets::m_nFallbackStatTrak, 1337);
				memory.Write<std::int32_t>(weapon + offsets::m_iAccountID, memory.Read<std::int32_t>(weapon + offsets::m_OriginalOwnerXuidLow));

				if (shouldUpdate)
					shouldSleep = false;
			}
		}

		if (!shouldSleep) {
			memory.Write<std::int32_t>(memory.Read<std::uintptr_t>(engine + offsets::dwClientState) + 0x174, -1);
			updateTimer = curtime + 1;
		}

		if (updateTimer < curtime) {
			Sleep(5);
		}
	}

	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();

	return EXIT_SUCCESS;
}