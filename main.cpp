#include <windows.h>
#include "main.h"
#include "natives.h"
#include "types.h"
#include "enums.h"

bool modZumbi = false, corredores = false, semPolicia = true, menuAberto = false, airdrop = false;
int sel = 0;
float fome = 100.0f, sede = 100.0f, estamina = 100.0f;
Object caixa = 0;
Blip blipCaixa = 0;
DWORD tickSobrev = 0;

void Aviso(const char* m) {
    UI::_SET_NOTIFICATION_TEXT_ENTRY((char*)"STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char*)m);
    UI::_DRAW_NOTIFICATION(false, true);
}

void Ret(float x, float y, float w, float h, int r, int g, int b, int a) {
    GRAPHICS::DRAW_RECT(x, y, w, h, r, g, b, a);
}

void Txt(const char* t, float x, float y, float e, int r, int g, int b, bool c) {
    UI::SET_TEXT_FONT(0);
    UI::SET_TEXT_SCALE(0.0f, e);
    UI::SET_TEXT_COLOUR(r, g, b, 255);
    UI::SET_TEXT_CENTRE(c);
    UI::SET_TEXT_OUTLINE();
    UI::_SET_TEXT_ENTRY((char*)"STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char*)t);
    UI::_DRAW_TEXT(x, y);
}

void AnimSaquear() {
    Ped p = PLAYER::PLAYER_PED_ID();
    STREAMING::REQUEST_ANIM_DICT((char*)"pickup_object");
    for (int t = 0; !STREAMING::HAS_ANIM_DICT_LOADED((char*)"pickup_object") && t < 40; t++) WAIT(10);
    if (STREAMING::HAS_ANIM_DICT_LOADED((char*)"pickup_object")) {
        AI::TASK_PLAY_ANIM(p, (char*)"pickup_object", (char*)"pickup_low", 8.0f, -8.0f, 1200, 0, 0.0f, false, false, false);
        Aviso("~y~Vasculhando...");
    }
}

void CriarNPC(const char* mod, const char* arma, bool aliado) {
    Ped p = PLAYER::PLAYER_PED_ID();
    Vector3 pos = ENTITY::GET_ENTITY_COORDS(p, true);
    Hash h = GAMEPLAY::GET_HASH_KEY((char*)mod);
    STREAMING::REQUEST_MODEL(h);
    for (int t = 0; !STREAMING::HAS_MODEL_LOADED(h) && t < 50; t++) WAIT(10);
    if (STREAMING::HAS_MODEL_LOADED(h)) {
        for (int i = 0; i < (aliado ? 1 : 3); i++) {
            float off = aliado ? 2.0f : (20.0f + i * 2.0f);
            Ped n = PED::CREATE_PED(26, h, pos.x + off, pos.y + off, pos.z, 0.0f, true, false);
            WEAPON::GIVE_DELAYED_WEAPON_TO_PED(n, GAMEPLAY::GET_HASH_KEY((char*)arma), 999, true);
            PED::SET_PED_COMBAT_ATTRIBUTES(n, 46, true);
            if (aliado) PED::SET_PED_AS_GROUP_MEMBER(n, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()));
            else { DECORATOR::DECOR_SET_BOOL(n, (char*)"SobreviventeNPC", true); AI::TASK_COMBAT_PED(n, p, 0, 16); }
        }
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(h);
        Aviso(aliado ? "~g~Aliado recrutado!" : "~r~Bandidos nas proximidades!");
    }
}

void CriarAirdrop() {
    Ped p = PLAYER::PLAYER_PED_ID();
    Vector3 pos = ENTITY::GET_ENTITY_COORDS(p, true);
    Hash h = GAMEPLAY::GET_HASH_KEY((char*)"prop_box_wood02a_pu");
    STREAMING::REQUEST_MODEL(h);
    for (int t = 0; !STREAMING::HAS_MODEL_LOADED(h) && t < 50; t++) WAIT(10);
    if (STREAMING::HAS_MODEL_LOADED(h)) {
        if (airdrop && ENTITY::DOES_ENTITY_EXIST(caixa)) { OBJECT::DELETE_OBJECT(&caixa); if (UI::DOES_BLIP_EXIST(blipCaixa)) UI::REMOVE_BLIP(&blipCaixa); }
        caixa = OBJECT::CREATE_OBJECT(h, pos.x + 15.0f, pos.y + 15.0f, pos.z + 25.0f, true, true, true);
        OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(caixa);
        ENTITY::SET_ENTITY_DYNAMIC(caixa, true);
        blipCaixa = UI::ADD_BLIP_FOR_ENTITY(caixa);
        UI::SET_BLIP_SPRITE(blipCaixa, 478);
        UI::SET_BLIP_COLOUR(blipCaixa, 5);
        airdrop = true;
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(h);
        Aviso("~y~AIRDROP marcado no minimapa!");
    }
}

bool BotaoInteragir() {
    return (GetAsyncKeyState(0x45) & 1) || CONTROLS::IS_CONTROL_JUST_PRESSED(0, 51) || CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 51);
}

void LoopZumbis() {
    Ped p = PLAYER::PLAYER_PED_ID();
    Vector3 pPos = ENTITY::GET_ENTITY_COORDS(p, true);
    int grp = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());
    const char* anim = corredores ? "move_m@brave" : "move_m@drunk@verydrunk";
    STREAMING::REQUEST_ANIM_SET((char*)anim);

    Ped* peds = new Ped;
    int count = worldGetAllPeds(peds, 64);
    for (int i = 0; i < count; i++) {
        Ped ped = *(peds + i);
        if (!ENTITY::DOES_ENTITY_EXIST(ped) || ped == p || !PED::IS_PED_HUMAN(ped) || PED::IS_PED_GROUP_MEMBER(ped, grp) || DECORATOR::DECOR_EXIST_ON(ped, (char*)"SobreviventeNPC")) continue;
        Vector3 pos = ENTITY::GET_ENTITY_COORDS(ped, true);
        float d = SYSTEM::VDIST(pPos.x, pPos.y, pPos.z, pos.x, pos.y, pos.z);
        if (ENTITY::IS_ENTITY_DEAD(ped)) {
            if (d < 2.2f) { Txt("Toque em ~y~INTERAGIR~w~ para vasculhar", 0.5f, 0.82f, 0.42f, 255, 255, 255, true); if (BotaoInteragir()) AnimSaquear(); }
            continue;
        }
        if (d < 110.0f) {
            if (!DECORATOR::DECOR_EXIST_ON(ped, (char*)"ZumbiConvertido")) {
                DECORATOR::DECOR_SET_BOOL(ped, (char*)"ZumbiConvertido", true);
                WEAPON::REMOVE_ALL_PED_WEAPONS(ped, true);
                PED::SET_PED_COMBAT_ATTRIBUTES(ped, 46, true);
                PED::SET_PED_COMBAT_ATTRIBUTES(ped, 5, true);
                PED::SET_PED_FLEE_ATTRIBUTES(ped, 0, false);
                PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped, true);
                PED::APPLY_PED_DAMAGE_PACK(ped, (char*)"BigHitByVehicle", 0.0f, 1.0f);
            }
            if (STREAMING::HAS_ANIM_SET_LOADED((char*)anim)) PED::SET_PED_MOVEMENT_CLIPSET(ped, (char*)anim, 1.0f);
            if (!PED::IS_PED_IN_COMBAT(ped, p)) AI::TASK_COMBAT_PED(ped, p, 0, 16);
        }
    }
    delete peds;

    if (airdrop && ENTITY::DOES_ENTITY_EXIST(caixa)) {
        Vector3 bPos = ENTITY::GET_ENTITY_COORDS(caixa, true);
        if (SYSTEM::VDIST(pPos.x, pPos.y, pPos.z, bPos.x, bPos.y, bPos.z) < 3.0f) {
            Txt("Toque em ~y~INTERAGIR~w~ para abrir Airdrop", 0.5f, 0.82f, 0.42f, 255, 255, 255, true);
            if (BotaoInteragir()) { AnimSaquear(); if (UI::DOES_BLIP_EXIST(blipCaixa)) UI::REMOVE_BLIP(&blipCaixa); OBJECT::DELETE_OBJECT(&caixa); airdrop = false; }
        }
    }
}

void LoopSobrev() {
    if (!modZumbi) return;
    Ped p = PLAYER::PLAYER_PED_ID();
    if (semPolicia) PLAYER::CLEAR_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID());
    if (AI::IS_PED_SPRINTING(p) || AI::IS_PED_RUNNING(p)) {
        estamina -= 0.08f;
        if (estamina < 0.0f) { estamina = 0.0f; CONTROLS::DISABLE_CONTROL_ACTION(0, 21, true); }
    } else if (estamina < 100.0f) estamina += 0.05f;
    if (GetTickCount() - tickSobrev > 2000) {
        tickSobrev = GetTickCount();
        if (fome > 0.0f) fome -= 0.25f;
        if (sede > 0.0f) sede -= 0.35f;
        if (fome <= 0.0f || sede <= 0.0f) { int v = ENTITY::GET_ENTITY_HEALTH(p); if (v > 105) ENTITY::SET_ENTITY_HEALTH(p, v - 2); }
    }
    float bx = 0.085f, by = 0.775f, lm = 0.14f, al = 0.011f;
    Ret(bx, by, lm + 0.004f, al + 0.004f, 0, 0, 0, 180); Ret(bx, by + 0.016f, lm + 0.004f, al + 0.004f, 0, 0, 0, 180); Ret(bx, by + 0.032f, lm + 0.004f, al + 0.004f, 0, 0, 0, 180);
    Ret(bx - (lm - lm * (fome / 100.0f)) * 0.5f, by, lm * (fome / 100.0f), al, 230, 140, 30, 230);
    Ret(bx - (lm - lm * (sede / 100.0f)) * 0.5f, by + 0.016f, lm * (sede / 100.0f), al, 40, 160, 240, 230);
    Ret(bx - (lm - lm * (estamina / 100.0f)) * 0.5f, by + 0.032f, lm * (estamina / 100.0f), al, 60, 210, 90, 230);
}

const char* ItemMenu(int i) {
    if (i == 0) return modZumbi ? "Modo Zumbi: ~g~ATIVADO" : "Modo Zumbi: ~r~DESATIVADO";
    if (i == 1) return corredores ? "Tipo Zumbi: ~r~CORREDORES" : "Tipo Zumbi: ~y~LENTOS";
    if (i == 2) return semPolicia ? "Policia: ~g~DESATIVADA" : "Policia: ~r~NORMAL";
    if (i == 3) return "Recrutar Sobrevivente Aliado";
    if (i == 4) return "Gerar Bandidos Hostis";
    return "Chamar Airdrop de Suprimentos";
}

void Menu() {
    if (!menuAberto) return;
    float mx = 0.13f, my = 0.08f, lg = 0.23f, al = 0.032f;
    Ret(mx, my, lg, 0.042f, 140, 20, 20, 235);
    Txt("ZUMBI SURVIVAL (F1)", mx, my - 0.013f, 0.42f, 255, 255, 255, true);
    for (int i = 0; i < 6; i++) {
        float y = my + 0.037f + (i * al);
        bool s = (i == sel);
        Ret(mx, y, lg, al, s ? 240 : 20, s ? 240 : 20, s ? 240 : 25, s ? 230 : 200);
        Txt(ItemMenu(i), mx - (lg * 0.46f), y - 0.011f, 0.35f, s ? 15 : 240, s ? 15 : 240, s ? 15 : 240, false);
    }
    if ((GetAsyncKeyState(VK_NUMPAD8) & 1) || (GetAsyncKeyState(VK_UP) & 1)) sel = (sel + 5) % 6;
    if ((GetAsyncKeyState(VK_NUMPAD2) & 1) || (GetAsyncKeyState(VK_DOWN) & 1)) sel = (sel + 1) % 6;
    if ((GetAsyncKeyState(VK_NUMPAD0) & 1) || (GetAsyncKeyState(VK_BACK) & 1)) menuAberto = false;
    if ((GetAsyncKeyState(VK_NUMPAD5) & 1) || (GetAsyncKeyState(VK_RETURN) & 1)) {
        switch (sel) {
            case 0: modZumbi = !modZumbi; Aviso(modZumbi ? "~r~APOCALIPSE ZUMBI INICIADO!" : "~g~Modo Zumbi Desativado."); break;
            case 1: corredores = !corredores; break;
            case 2: semPolicia = !semPolicia; break;
            case 3: CriarNPC("s_m_y_marine_01", "WEAPON_CARBINERIFLE", true); break;
            case 4: CriarNPC("g_m_y_lost_01", "WEAPON_ASSAULTRIFLE", false); break;
            case 5: CriarAirdrop(); break;
        }
    }
}

void ScriptMain() {
    WAIT(2000);
    Aviso("~y~Zumbi Survival Carregado! Aperte F1 para abrir.");
    while (true) {
        if (GetAsyncKeyState(VK_F1) & 1) menuAberto = !menuAberto;
        Menu();
        if (modZumbi) { LoopZumbis(); LoopSobrev(); }
        WAIT(0);
    }
}

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID lp) {
    if (r == DLL_PROCESS_ATTACH) scriptRegister(h, ScriptMain);
    else if (r == DLL_PROCESS_DETACH) scriptUnregister(h);
    return TRUE;
}
    else if (opcaoSelecionada == 3) SpawnNPC("s_m_y_marine_01", "WEAPON_CARBINERIFLE", true);
        else if (opcaoSelecionada == 4) SpawnNPC("g_m_y_lost_01", "WEAPON_ASSAULTRIFLE", false);
        else if (opcaoSelecionada == 5) ChamarAirdrop();
    }
}

void ScriptMain() {
    WAIT(2000);
    Notificar("~y~Zumbi Survival Carregado! Aperte F1 para abrir o menu.");
    while (true) {
ede -= 0.35f;
