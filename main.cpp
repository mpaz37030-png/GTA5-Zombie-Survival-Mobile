#include <windows.h>
#include "main.h"
#include "natives.h"
#include "types.h"
#include "enums.h"

bool modoZumbiAtivo = false;

void MostrarAviso(const char* texto) {
    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
    UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME((char*)texto);
    UI::_DRAW_NOTIFICATION(false, true);
}

void AtualizarZumbis() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, true);

    const int MAX_PEDS = 64;
    Ped peds[MAX_PEDS];
    int count = worldGetAllPeds(peds, MAX_PEDS);

    STREAMING::REQUEST_ANIM_SET("move_m@drunk@verydrunk");

    for (int i = 0; i < count; i++) {
        Ped ped = peds[i];
        if (!ENTITY::DOES_ENTITY_EXIST(ped) || ped == playerPed || ENTITY::IS_ENTITY_DEAD(ped)) {
            continue;
        }
        if (!PED::IS_PED_HUMAN(ped)) {
            continue;
        }

        Vector3 pedPos = ENTITY::GET_ENTITY_COORDS(ped, true);
        float dist = SYSTEM::VDIST(playerPos.x, playerPos.y, playerPos.z, pedPos.x, pedPos.y, pedPos.z);

        if (dist < 120.0f) {
            if (!DECORATOR::DECOR_EXIST_ON(ped, "ZumbiConvertido")) {
                DECORATOR::DECOR_SET_BOOL(ped, "ZumbiConvertido", true);

                WEAPON::REMOVE_ALL_PED_WEAPONS(ped, true);
                PED::SET_PED_COMBAT_ATTRIBUTES(ped, 46, true);
                PED::SET_PED_COMBAT_ATTRIBUTES(ped, 5, true);
                PED::SET_PED_FLEE_ATTRIBUTES(ped, 0, false);
                PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped, true);
                PED::APPLY_PED_DAMAGE_PACK(ped, "BigHitByVehicle", 0.0f, 1.0f);
                AI::TASK_COMBAT_PED(ped, playerPed, 0, 16);
            }

            if (STREAMING::HAS_ANIM_SET_LOADED("move_m@drunk@verydrunk")) {
                PED::SET_PED_MOVEMENT_CLIPSET(ped, "move_m@drunk@verydrunk", 1.0f);
            }

            if (!PED::IS_PED_IN_COMBAT(ped, playerPed)) {
                AI::TASK_COMBAT_PED(ped, playerPed, 0, 16);
            }
        }
    }
}

void ScriptMain() {
    while (true) {
        if (GetAsyncKeyState(0x5A) & 1) {
            modoZumbiAtivo = !modoZumbiAtivo;
            if (modoZumbiAtivo) {
                MostrarAviso("~r~MODO ZUMBI: ATIVADO");
            } else {
                MostrarAviso("~g~MODO ZUMBI: DESATIVADO");
            }
        }

        if (modoZumbiAtivo) {
            AtualizarZumbis();
        }

        WAIT(100);
    }
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        scriptRegister(hInstance, ScriptMain);
        break;
    case DLL_PROCESS_DETACH:
        scriptUnregister(hInstance);
        break;
    }
    return TRUE;
}
