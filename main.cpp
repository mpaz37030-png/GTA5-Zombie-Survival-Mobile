#include <windows.h>
#include "main.h"
#include "types.h"

bool modoZumbiAtivo = false;
int bannerTimer = 0;

void DesenharTextoCentro(const char* texto, float y) {
    nativeInit(0x66E0276CC5F6B9DA);
    nativePush64(4);
    nativeCall();

    nativeInit(0x07C837F9A01C34C9);
    nativePush64(0);
    float scale = 0.65f;
    nativePush64(*(UINT64*)&scale);
    nativeCall();

    nativeInit(0xBE6B23FFA53FB442);
    nativePush64(255);
    nativePush64(60);
    nativePush64(60);
    nativePush64(255);
    nativeCall();

    nativeInit(0xC02F4DBFB51D988B);
    nativePush64(1);
    nativeCall();

    nativeInit(0x2513DFB0FB8400FE);
    nativeCall();

    nativeInit(0x25FBB336DF1804CB);
    nativePush64((UINT64)"STRING");
    nativeCall();

    nativeInit(0x6C188BE134E074AA);
    nativePush64((UINT64)texto);
    nativeCall();

    nativeInit(0xCD015E5BB0D96A57);
    float x = 0.5f;
    nativePush64(*(UINT64*)&x);
    nativePush64(*(UINT64*)&y);
    nativeCall();
}

bool ChecarControle(int controle) {
    nativeInit(0x580417101DDB492F);
    nativePush64(0);
    nativePush64(controle);
    if (*nativeCall() != 0) return true;

    nativeInit(0x91AEF906BCA88877);
    nativePush64(0);
    nativePush64(controle);
    if (*nativeCall() != 0) return true;

    return false;
}

void AtualizarZumbis() {
    nativeInit(0xD80958FC74E988A6);
    Ped playerPed = (Ped)*nativeCall();

    nativeInit(0x3FEF770D40960D5A);
    nativePush64(playerPed);
    nativePush64(1);
    Vector3 playerPos = *(Vector3*)nativeCall();

    nativeInit(0x6EA47DAE7FAD0EED);
    nativePush64((UINT64)"move_m@drunk@verydrunk");
    nativeCall();

    const int MAX_PEDS = 64;
    Ped peds[MAX_PEDS];
    int count = worldGetAllPeds(peds, MAX_PEDS);

    for (int i = 0; i < count; i++) {
        Ped ped = peds[i];
        if (ped == playerPed) continue;

        nativeInit(0x7239B21A38F536BA);
        nativePush64(ped);
        if (*nativeCall() == 0) continue;

        nativeInit(0x5F9532F3B5CC2551);
        nativePush64(ped);
        if (*nativeCall() != 0) continue;

        nativeInit(0xB980061DA992779D);
        nativePush64(ped);
        if (*nativeCall() == 0) continue;

        nativeInit(0x3FEF770D40960D5A);
        nativePush64(ped);
        nativePush64(1);
        Vector3 pedPos = *(Vector3*)nativeCall();

        float dx = playerPos.x - pedPos.x;
        float dy = playerPos.y - pedPos.y;
        float dz = playerPos.z - pedPos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < 10000.0f) {
            nativeInit(0xF25DF915FA38C5F3);
            nativePush64(ped);
            nativePush64(1);
            nativeCall();

            nativeInit(0x9F8AA94D6D97DBF4);
            nativePush64(ped);
            nativePush64(1);
            nativeCall();

            nativeInit(0xC4EA073D86FB29B0);
            nativePush64((UINT64)"move_m@drunk@verydrunk");
            if (*nativeCall() != 0) {
                nativeInit(0xAF8A94EDE7712BEF);
                nativePush64(ped);
                nativePush64((UINT64)"move_m@drunk@verydrunk");
                float speed = 1.0f;
                nativePush64(*(UINT64*)&speed);
                nativeCall();
            }

            nativeInit(0x4859F1FC66A6278E);
            nativePush64(ped);
            nativePush64(playerPed);
            if (*nativeCall() == 0) {
                nativeInit(0xF166E48407BAC484);
                nativePush64(ped);
                nativePush64(playerPed);
                nativePush64(0);
                nativePush64(16);
                nativeCall();
            }
        }
    }
}

void ScriptMain() {
    bannerTimer = 240;

    while (true) {
        nativeInit(0x4F8644AF03D0E0D6);
        Player player = (Player)*nativeCall();

        nativeInit(0x9CB5CE07A3968D5A);
        nativePush64(player);
        nativeCall();

        if (ChecarControle(171) || ChecarControle(28)) {
            modoZumbiAtivo = !modoZumbiAtivo;
            bannerTimer = 150;
        }

        if (bannerTimer > 0) {
            bannerTimer--;
            if (modoZumbiAtivo) {
                DesenharTextoCentro("MODO ZUMBI: ATIVADO", 0.12f);
            } else {
                DesenharTextoCentro("ZUMBI MOBILE CARREGADO - TOQUE NO RAIO", 0.12f);
            }
        }

        if (modoZumbiAtivo) {
            AtualizarZumbis();
        }

        WAIT(0);
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
