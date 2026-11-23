#include <windows.h>
#include "main.h"
#include "natives.h"
#include "types.h"
#include "enums.h"

bool modZumbiAtivo = false, zumbisCorredores = false, semPolicia = true, menuAberto = false, airdropAtivo = false;
int opcaoSelecionada = 0;
float fome = 100.0f, sede = 100.0f, estamina = 100.0f;
Object caixaAirdrop = 0;
Blip blipAirdrop = 0;
DWORD ultimoTick = 0;

void Notificar(const char* msg) {
    UI::_SET_NOTIFICATION_TEXT_ENTRY((char*)"STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char*)msg);
    UI::_DRAW_NOTIFICATION(false, true);
}

void Retangulo(float x, float y, float w, float h, int r, int g, int b, int a) {
    GRAPHICS::DRAW_RECT(x, y, w, h, r, g, b, a);
}

void Texto(const char* txt, float x, float y, float esc, int r, int g, int b, bool centro) {
    UI::SET_TEXT_FONT(0);
    UI::SET_TEXT_SCALE(0.0f, esc);
    UI::SET_TEXT_COLOUR(r, g, b, 255);
    UI::SET_TEXT_CENTRE(centro);
    UI::SET_TEXT_OUTLINE();
    UI::_SET_TEXT_ENTRY((char*)"STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char*)txt);
    UI::_DRAW_TEXT(x, y);
}

void AnimPegarChao() {
    Ped p = PLAYER::PLAYER_PED_ID();
    STREAMING::REQUEST_ANIM_DICT((char*)"pickup_object");
    for (int t = 0; !STREAMING::HAS_ANIM_DICT_LOADED((char*)"pickup_object") && t < 40; t++) WAIT(10);
    if (STREAMING::HAS_ANIM_DICT_LOADED((char*)"pickup_object")) {
        AI::TASK_PLAY_ANIM(p, (char*)"pickup_object", (char*)"pickup_low", 8.0f, -8.0f, 1200, 0, 0.0f, false, false, false);
        Notificar("~y~Vasculhando...");
    }
}

void SpawnNPC(const char* mod, const char* arma, bool aliado) {
    Ped p = PLAYER::PLAYER_PED_ID();
    Vector3 pos = ENTITY::GET_ENTITY_COORDS(p, true);
    Hash h = GAMEPLAY::GET_HASH_KEY((char*)mod);
    STREAMING::REQUEST_MODEL(h);
    for (int t = 0; !STREAMING::HAS_MODEL_LOADED(h) && t < 50; t++) WAIT(10);
    if (STREAMING::HAS_MODEL_LOADED(h)) {
        int qtd = aliado ? 1 : 3;
        for (int i = 0; i < qtd; i++) {
            float off = aliado ? 2.0f : (20.0f + i * 2.0f);
            Ped npc = PED::CREATE_PED(26, h, pos.x + off, pos.y + off, pos.z, 0.0f, true, false);
            WEAPON::GIVE_DELAYED_WEAPON_TO_PED(npc, GAMEPLAY::GET_HASH_KEY((char*)arma), 999, true);
            PED::SET_PED_COMBAT_ATTRIBUTES(npc, 46, true);
            if (aliado) {
                PED::SET_PED_AS_GROUP_MEMBER(npc, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()));
            } else {
                DECORATOR::DECOR_SET_BOOL(npc, (char*)"SobreviventeNPC", true);
                AI::TASK_COMBAT_PED(npc, p, 0, 16);
            }
        }
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(h);
        Notificar(aliado ? "~g~Aliado recrutado!" : "~r~Bandidos nas proximidades!");
    }
}

void ChamarAirdrop() {
    Ped p = PLAYER::PLAYER_PED_ID();
    Vector3 pos = ENTITY::GET_ENTITY_COORDS(p, true);
    Hash h = GAMEPLAY::GET_HASH_KEY((char*)"prop_box_wood02a_pu");
    STREAMING::REQUEST_MODEL(h);
    for (int t = 0; !STREAMING::HAS_MODEL_LOADED(h) && t < 50; t++) WAIT(10);
    if (STREAMING::HAS_MODEL_LOADED(h)) {
        if (airdropAtivo && ENTITY::DOES_ENTITY_EXIST(caixaAirdrop)) {
            OBJECT::DELETE_OBJECT(&caixaAirdrop);
            if (UI::DOES_BLIP_EXIST(blipAirdrop)) UI::REMOVE_BLIP(&blipAirdrop);
        }
        caixaAirdrop = OBJECT::CREATE_OBJECT(h, pos.x + 15.0f, pos.y + 15.0f, pos.z + 25.0f, true, true, true);
        OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(caixaAirdrop);
        ENTITY::SET_ENTITY_DYNAMIC(caixaAirdrop, true);
        blipAirdrop = UI::ADD_BLIP_FOR_ENTITY(caixaAirdrop);
        UI::SET_BLIP_SPRITE(blipAirdrop, 478);
        UI::SET_BLIP_COLOUR(blipAirdrop, 5);
        airdropAtivo = true;
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(h);
        Notificar("~y~AIRDROP marcado no minimapa!");
    }
}

bool InteragirPressionado() {
    return (GetAsyncKeyState(0x45) & 1) || CONTROLS::IS_CONTROL_JUST_PRESSED(0, 51) || CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 51);
}

void AtualizarZumbis() {
    Ped p = PLAYER::PLAYER_PED_ID();
    Vector3 pPos = ENTITY::GET_ENTITY_COORDS(p, true);
    int grp = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());
    const char* anim = zumbisCorredores ? "move_m@brave" : "move_m@drunk@verydrunk";
    STREAMING::REQUEST_ANIM_SET((char*)anim);

    Ped peds[64];
    int count = worldGetAllPeds(peds, 64);
    for (int i = 0; i < count; i++) {
        Ped ped = peds[i];
        if (!ENTITY::DOES_ENTITY_EXIST(ped) || ped == p || !PED::IS_PED_HUMAN(ped) || PED::IS_PED_GROUP_MEMBER(ped, grp) || DECORATOR::DECOR_EXIST_ON(ped, (char*)"SobreviventeNPC")) continue;
        Vector3 pos = ENTITY::GET_ENTITY_COORDS(ped, true);
        float d = SYSTEM::VDIST(pPos.x, pPos.y, pPos.z, pos.x, pos.y, pos.z);

        if (ENTITY::IS_ENTITY_DEAD(ped)) {
            if (d < 2.2f) {
                Texto("Toque em ~y~INTERAGIR~w~ para vasculhar", 0.5f, 0.82f, 0.42f, 255, 255, 255, true);
                if (InteragirPressionado()) AnimPegarChao();
            }
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

    if (airdropAtivo && ENTITY::DOES_ENTITY_EXIST(caixaAirdrop)) {
        Vector3 bPos = ENTITY::GET_ENTITY_COORDS(caixaAirdrop, true);
        if (SYSTEM::VDIST(pPos.x, pPos.y, pPos.z, bPos.x, bPos.y, bPos.z) < 3.0f) {
            Texto("Toque em ~y~INTERAGIR~w~ para abrir o Airdrop", 0.5f, 0.82f, 0.42f, 255, 255, 255, true);
            if (InteragirPressionado()) {
                AnimPegarChao();
                if (UI::DOES_BLIP_EXIST(blipAirdrop)) UI::REMOVE_BLIP(&blipAirdrop);
                OBJECT::DELETE_OBJECT(&caixaAirdrop);
                airdropAtivo = false;
            }
        }
    }
}

void AtualizarSobrevivencia() {
    if (!modZumbiAtivo) return;
    Ped p = PLAYER::PLAYER_PED_ID();
    if (semPolicia) PLAYER::CLEAR_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID());

    if (AI::IS_PED_SPRINTING(p) || AI::IS_PED_RUNNING(p)) {
        estamina -= 0.08f;
        if (estamina < 0.0f) { estamina = 0.0f; CONTROLS::DISABLE_CONTROL_ACTION(0, 21, true); }
    } else if (estamina < 100.0f) estamina += 0.05f;

    if (GetTickCount() - ultimoTick > 2000) {
        ultimoTick = GetTickCount();
        if (fome > 0.0f) fome -= 0.25f;
        if (sede > 0.0f) sede -= 0.35f;
        if (fome <= 0.0f || sede <= 0.0f) {
            int v = ENTITY::GET_ENTITY_HEALTH(p);
            if (v > 105) ENTITY::SET_ENTITY_HEALTH(p, v - 2);
        }
    }

    float bx = 0.085f, by = 0.775f, lm = 0.14f, al = 0.011f;
    Retangulo(bx, by, lm + 0.004f, al + 0.004f, 0, 0, 0, 180);
    Retangulo(bx, by + 0.016f, lm + 0.004f, al + 0.004f, 0, 0, 0, 180);
    Retangulo(bx, by + 0.032f, lm + 0.004f, al + 0.004f, 0, 0, 0, 180);
    Retangulo(bx - (lm - lm * (fome / 100.0f)) * 0.5f, by, lm * (fome / 100.0f), al, 230, 140, 30, 230);
    Retangulo(bx - (lm - lm * (sede / 100.0f)) * 0.5f, by + 0.016f, lm * (sede / 100.0f), al, 40, 160, 240, 230);
    Retangulo(bx - (lm - lm * (estamina / 100.0f)) * 0.5f, by + 0.032f, lm * (estamina / 100.0f), al, 60, 210, 90, 230);
}

const char* TextoMenu(int i) {
    if (i == 0) return modZumbiAtivo ? "Modo Zumbi: ~g~ATIVADO" : "Modo Zumbi: ~r~DESATIVADO";
    if (i == 1) return zumbisCorredores ? "Tipo Zumbi: ~r~CORREDORES" : "Tipo Zumbi: ~y~LENTOS";
    if (i == 2) return semPolicia ? "Policia: ~g~DESATIVADA" : "Policia: ~r~NORMAL";
    if (i == 3) return "Recrutar Sobrevivente Aliado";
    if (i == 4) return "Gerar Bandidos Hostis";
    return "Chamar Airdrop de Suprimentos";
}

void DesenharMenu() {
    if (!menuAberto) return;
    float mx = 0.13f, my = 0.08f, lg = 0.23f, al = 0.032f;
    Retangulo(mx, my, lg, 0.042f, 140, 20, 20, 235);
    Texto("ZUMBI SURVIVAL (F1)", mx, my - 0.013f, 0.42f, 255, 255, 255, true);

    for (int i = 0; i < 6; i++) {
        float y = my + 0.037f + (i * al);
        bool sel = (i == opcaoSelecionada);
        Retangulo(mx, y, lg, al, sel ? 240 : 20, sel ? 240 : 20, sel ? 240 : 25, sel ? 230 : 200);
        Texto(TextoMenu(i), mx - (lg * 0.46f), y - 0.011f, 0.35f, sel ? 15 : 240, sel ? 15 : 240, sel ? 15 : 240, false);
    }

    if ((GetAsyncKeyState(VK_NUMPAD8) & 1) || (GetAsyncKeyState(VK_UP) & 1)) opcaoSelecionada = (opcaoSelecionada + 5) % 6;
    if ((GetAsyncKeyState(VK_NUMPAD2) & 1) || (GetAsyncKeyState(VK_DOWN) & 1)) opcaoSelecionada = (opcaoSelecionada + 1) % 6;
    if ((GetAsyncKeyState(VK_NUMPAD0) & 1) || (GetAsyncKeyState(VK_BACK) & 1)) menuAberto = false;
    if ((GetAsyncKeyState(VK_NUMPAD5) & 1) || (GetAsyncKeyState(VK_RETURN) & 1)) {
        if (opcaoSelecionada == 0) { modZumbiAtivo = !modZumbiAtivo; Notificar(modZumbiAtivo ? "~r~APOCALIPSE ZUMBI INICIADO!" : "~g~Modo Zumbi Desativado."); }
        else if (opcaoSelecionada == 1) zumbisCorredores = !zumbisCorredores;
        else if (opcaoSelecionada == 2) semPolicia = !semPolicia;
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
