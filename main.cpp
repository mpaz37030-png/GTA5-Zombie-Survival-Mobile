#include <windows.h>
#include <stdio.h>
#include "main.h"
#include "natives.h"
#include "types.h"
#include "enums.h"

bool modZumbiAtivo = false;
bool zumbisCorredores = false;
bool semPolicia = true;
bool menuAberto = false;
int opcaoSelecionada = 0;

float fome = 100.0f;
float sede = 100.0f;
float estamina = 100.0f;

Object caixaAirdrop = 0;
Blip blipAirdrop = 0;
bool airdropAtivo = false;
DWORD ultimoTickSobrevivencia = 0;

void MostrarNotificacao(const char* msg) {
    UI::_SET_NOTIFICATION_TEXT_ENTRY((char*)"STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char*)msg);
    UI::_DRAW_NOTIFICATION(false, true);
}

void DesenharRetangulo(float x, float y, float w, float h, int r, int g, int b, int a) {
    GRAPHICS::DRAW_RECT(x, y, w, h, r, g, b, a);
}

void DesenharTexto(const char* texto, float x, float y, float escala, int r, int g, int b, bool centro) {
    UI::SET_TEXT_FONT(0);
    UI::SET_TEXT_SCALE(0.0f, escala);
    UI::SET_TEXT_COLOUR(r, g, b, 255);
    UI::SET_TEXT_CENTRE(centro);
    UI::SET_TEXT_OUTLINE();
    UI::_SET_TEXT_ENTRY((char*)"STRING");
    UI::_ADD_TEXT_COMPONENT_STRING((char*)texto);
    UI::_DRAW_TEXT(x, y);
}

void TocarAnimacaoPegarChao() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    char* dict = (char*)"pickup_object";
    char* anim = (char*)"pickup_low";

    STREAMING::REQUEST_ANIM_DICT(dict);
    int tentativas = 0;
    while (!STREAMING::HAS_ANIM_DICT_LOADED(dict) && tentativas < 40) {
        WAIT(10);
        tentativas++;
    }

    if (STREAMING::HAS_ANIM_DICT_LOADED(dict)) {
        AI::TASK_PLAY_ANIM(playerPed, dict, anim, 8.0f, -8.0f, 1200, 0, 0.0f, false, false, false);
        MostrarNotificacao("~y~Vasculhando corpo...");
    }
}

void DesenharBarrasSobrevivencia() {
    if (!modZumbiAtivo) return;

    float baseX = 0.085f;
    float baseY = 0.775f;
    float larguraMax = 0.14f;
    float altura = 0.011f;

    DesenharRetangulo(baseX, baseY, larguraMax + 0.004f, altura + 0.004f, 0, 0, 0, 180);
    DesenharRetangulo(baseX, baseY + 0.016f, larguraMax + 0.004f, altura + 0.004f, 0, 0, 0, 180);
    DesenharRetangulo(baseX, baseY + 0.032f, larguraMax + 0.004f, altura + 0.004f, 0, 0, 0, 180);

    float wFome = larguraMax * (fome / 100.0f);
    DesenharRetangulo(baseX - (larguraMax - wFome) * 0.5f, baseY, wFome, altura, 230, 140, 30, 230);

    float wSede = larguraMax * (sede / 100.0f);
    DesenharRetangulo(baseX - (larguraMax - wSede) * 0.5f, baseY + 0.016f, wSede, altura, 40, 160, 240, 230);

    float wEst = larguraMax * (estamina / 100.0f);
    DesenharRetangulo(baseX - (larguraMax - wEst) * 0.5f, baseY + 0.032f, wEst, altura, 60, 210, 90, 230);
}

void SpawnarAliado() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, true);
    Hash modelo = GAMEPLAY::GET_HASH_KEY((char*)"s_m_y_marine_01");

    STREAMING::REQUEST_MODEL(modelo);
    int tentativas = 0;
    while (!STREAMING::HAS_MODEL_LOADED(modelo) && tentativas < 50) {
        WAIT(10);
        tentativas++;
    }

    if (STREAMING::HAS_MODEL_LOADED(modelo)) {
        Ped aliado = PED::CREATE_PED(26, modelo, pos.x + 2.0f, pos.y + 2.0f, pos.z, 0.0f, true, false);
        int grupoPlayer = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());
        PED::SET_PED_AS_GROUP_MEMBER(aliado, grupoPlayer);
        PED::SET_PED_CAN_SWITCH_WEAPON(aliado, true);
        WEAPON::GIVE_DELAYED_WEAPON_TO_PED(aliado, GAMEPLAY::GET_HASH_KEY((char*)"WEAPON_CARBINERIFLE"), 999, true);
        PED::SET_PED_COMBAT_ATTRIBUTES(aliado, 46, true);
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(modelo);
        MostrarNotificacao("~g~Sobrevivente Aliado recrutado!");
    }
}

void SpawnarBandidos() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, true);
    Hash modelo = GAMEPLAY::GET_HASH_KEY((char*)"g_m_y_lost_01");

    STREAMING::REQUEST_MODEL(modelo);
    int tentativas = 0;
    while (!STREAMING::HAS_MODEL_LOADED(modelo) && tentativas < 50) {
        WAIT(10);
        tentativas++;
    }

    if (STREAMING::HAS_MODEL_LOADED(modelo)) {
        for (int i = 0; i < 3; i++) {
            Ped bandido = PED::CREATE_PED(26, modelo, pos.x + 22.0f + (i * 2.0f), pos.y + 22.0f, pos.z, 0.0f, true, false);
            DECORATOR::DECOR_SET_BOOL(bandido, (char*)"SobreviventeNPC", true);
            WEAPON::GIVE_DELAYED_WEAPON_TO_PED(bandido, GAMEPLAY::GET_HASH_KEY((char*)"WEAPON_ASSAULTRIFLE"), 999, true);
            PED::SET_PED_COMBAT_ATTRIBUTES(bandido, 46, true);
            AI::TASK_COMBAT_PED(bandido, playerPed, 0, 16);
        }
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(modelo);
        MostrarNotificacao("~r~Alerta: Bandidos hostis nas proximidades!");
    }
}

void ChamarAirdrop() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, true);
    Hash modeloCaixa = GAMEPLAY::GET_HASH_KEY((char*)"prop_box_wood02a_pu");

    STREAMING::REQUEST_MODEL(modeloCaixa);
    int tentativas = 0;
    while (!STREAMING::HAS_MODEL_LOADED(modeloCaixa) && tentativas < 50) {
        WAIT(10);
        tentativas++;
    }

    if (STREAMING::HAS_MODEL_LOADED(modeloCaixa)) {
        if (airdropAtivo && ENTITY::DOES_ENTITY_EXIST(caixaAirdrop)) {
            OBJECT::DELETE_OBJECT(&caixaAirdrop);
            if (UI::DOES_BLIP_EXIST(blipAirdrop)) UI::REMOVE_BLIP(&blipAirdrop);
        }

        caixaAirdrop = OBJECT::CREATE_OBJECT(modeloCaixa, pos.x + 15.0f, pos.y + 15.0f, pos.z + 25.0f, true, true, true);
        OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(caixaAirdrop);
        ENTITY::SET_ENTITY_DYNAMIC(caixaAirdrop, true);

        blipAirdrop = UI::ADD_BLIP_FOR_ENTITY(caixaAirdrop);
        UI::SET_BLIP_SPRITE(blipAirdrop, 478);
        UI::SET_BLIP_COLOUR(blipAirdrop, 5);
        airdropAtivo = true;

        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(modeloCaixa);
        MostrarNotificacao("~y~AIRDROP! Caixa marcada no minimapa.");
    }
}

bool BotaoInteragirPressionado() {
    return (GetAsyncKeyState(0x45) & 1) != 0 ||
           CONTROLS::IS_CONTROL_JUST_PRESSED(0, 51) != 0 ||
           CONTROLS::IS_DISABLED_CONTROL_JUST_PRESSED(0, 51) != 0;
}

void AtualizarZumbis() {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, true);
    int grupoPlayer = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());

    const char* animZumbi = zumbisCorredores ? "move_m@brave" : "move_m@drunk@verydrunk";
    STREAMING::REQUEST_ANIM_SET((char*)animZumbi);

    const int MAX_PEDS = 64;
    Ped peds[MAX_PEDS];
    int count = worldGetAllPeds(peds, MAX_PEDS);

    for (int i = 0; i < count; i++) {
        Ped ped = peds[i];
        if (!ENTITY::DOES_ENTITY_EXIST(ped) || ped == playerPed) continue;
        if (!PED::IS_PED_HUMAN(ped)) continue;
        if (PED::IS_PED_GROUP_MEMBER(ped, grupoPlayer)) continue;
        if (DECORATOR::DECOR_EXIST_ON(ped, (char*)"SobreviventeNPC")) continue;

        Vector3 pedPos = ENTITY::GET_ENTITY_COORDS(ped, true);
        float dist = SYSTEM::VDIST(playerPos.x, playerPos.y, playerPos.z, pedPos.x, pedPos.y, pedPos.z);

        if (ENTITY::IS_ENTITY_DEAD(ped)) {
            if (dist < 2.2f) {
                DesenharTexto("Toque em ~y~INTERAGIR~w~ para vasculhar", 0.5f, 0.82f, 0.42f, 255, 255, 255, true);
                if (BotaoInteragirPressionado()) {
                    TocarAnimacaoPegarChao();
                }
            }
            continue;
        }

        if (dist < 110.0f) {
            if (!DECORATOR::DECOR_EXIST_ON(ped, (char*)"ZumbiConvertido")) {
                DECORATOR::DECOR_SET_BOOL(ped, (char*)"ZumbiConvertido", true);
                WEAPON::REMOVE_ALL_PED_WEAPONS(ped, true);
                PED::SET_PED_COMBAT_ATTRIBUTES(ped, 46, true);
                PED::SET_PED_COMBAT_ATTRIBUTES(ped, 5, true);
                PED::SET_PED_FLEE_ATTRIBUTES(ped, 0, false);
                PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped, true);
                PED::APPLY_PED_DAMAGE_PACK(ped, (char*)"BigHitByVehicle", 0.0f, 1.0f);
            }

            if (STREAMING::HAS_ANIM_SET_LOADED((char*)animZumbi)) {
                PED::SET_PED_MOVEMENT_CLIPSET(ped, (char*)animZumbi, 1.0f);
            }

            if (!PED::IS_PED_IN_COMBAT(ped, playerPed)) {
                AI::TASK_COMBAT_PED(ped, playerPed, 0, 16);
            }
        }
    }

    if (airdropAtivo && ENTITY::DOES_ENTITY_EXIST(caixaAirdrop)) {
        Vector3 boxPos = ENTITY::GET_ENTITY_COORDS(caixaAirdrop, true);
        float distBox = SYSTEM::VDIST(playerPos.x, playerPos.y, playerPos.z, boxPos.x, boxPos.y, boxPos.z);
        if (distBox < 3.0f) {
            DesenharTexto("Toque em ~y~INTERAGIR~w~ para abrir o Airdrop", 0.5f, 0.82f, 0.42f, 255, 255, 255, true);
            if (BotaoInteragirPressionado()) {
                TocarAnimacaoPegarChao();
                if (UI::DOES_BLIP_EXIST(blipAirdrop)) UI::REMOVE_BLIP(&blipAirdrop);
                OBJECT::DELETE_OBJECT(&caixaAirdrop);
                airdropAtivo = false;
            }
        }
    }
}

void AtualizarSobrevivencia() {
    if (!modZumbiAtivo) return;

    Ped playerPed = PLAYER::PLAYER_PED_ID();
    if (semPolicia) {
        PLAYER::CLEAR_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID());
    }

    if (AI::IS_PED_SPRINTING(playerPed) || AI::IS_PED_RUNNING(playerPed)) {
        estamina -= 0.08f;
        if (estamina < 0.0f) {
            estamina = 0.0f;
            CONTROLS::DISABLE_CONTROL_ACTION(0, 21, true);
        }
    } else {
        estamina += 0.05f;
        if (estamina > 100.0f) estamina = 100.0f;
    }

    DWORD agora = GetTickCount();
    if (agora - ultimoTickSobrevivencia > 2000) {
        ultimoTickSobrevivencia = agora;
        fome -= 0.25f;
        sede -= 0.35f;
        if (fome < 0.0f) fome = 0.0f;
        if (sede < 0.0f) sede = 0.0f;

        if (fome == 0.0f || sede == 0.0f) {
            int vida = ENTITY::GET_ENTITY_HEALTH(playerPed);
.25f;
        sede -= 0.35f;
