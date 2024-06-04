#include <windows.h>
#include <algorithm>
#include <exedit.hpp>

static char name[] = "音量";

constexpr int track_n = 4;
static char* track_name[track_n] = { const_cast<char*>("音量"), const_cast<char*>("左右"), const_cast<char*>("音量-左"), const_cast<char*>("音量-右") };
static int track_default[track_n] = { 1000, 0, 1000, 1000 };
static int track_s[track_n] = { 0, -1000, 0, 0 };
static int track_e[track_n] = { 5000, 1000, 5000, 5000 };
static int track_scale[track_n] = { 10, 10, 10, 10 };

constexpr int check_n = 0;

BOOL func_proc(ExEdit::Filter* efp, ExEdit::FilterProcInfo* efpip) {

    int volume = (max(0, efp->track[0]) << 9) / 125;
    int left = max(0, efp->track[2]) * volume / 1000;
    int right = max(0, efp->track[3]) * volume / 1000;

    if (2 <= efpip->audio_ch) {
        int lr = std::clamp(efp->track[1], efp->track_s[1], efp->track_e[1]);
        if (lr < 0) {
            right = right * (lr + 1000) / 1000;
        } else if (0 < lr) {
            left = left * (1000 - lr) / 1000;
        }
    }
    short* audiop;
    if ((byte)efp->flag & (byte)(ExEdit::Filter::Flag::Effect)) {
        audiop = efpip->audio_data;
    } else {
        audiop = efpip->audio_p;
    }

    switch (efpip->audio_ch) {
    case 1: {
        for (int i = efpip->audio_n; 0 < i; i--) {
            *audiop = std::clamp(*audiop * volume >> 12, SHRT_MIN, SHRT_MAX);
            audiop++;
        }
    }break;
    case 2: {
        for (int i = efpip->audio_n; 0 < i; i--) {
            *audiop = std::clamp(*audiop * left >> 12, SHRT_MIN, SHRT_MAX);
            audiop++;
            *audiop = std::clamp(*audiop * right >> 12, SHRT_MIN, SHRT_MAX);
            audiop++;
        }
    }break;
    default: {
        for (int i = efpip->audio_n; 0 < i; i--) {
            *audiop = std::clamp(*audiop * left >> 12, SHRT_MIN, SHRT_MAX);
            audiop++;
            *audiop = std::clamp(*audiop * right >> 12, SHRT_MIN, SHRT_MAX);
            audiop++;
            for (int j = efpip->audio_ch - 2; 0 < j; j--) {
                *audiop = std::clamp(*audiop * volume >> 12, SHRT_MIN, SHRT_MAX);
                audiop++;
            }
        }
    }break;
    }

    return TRUE;
}

ExEdit::Filter effect_ef = {
    .flag = ExEdit::Filter::Flag::Audio | ExEdit::Filter::Flag::Effect,
    .name = name,
    .track_n = track_n,
    .track_name = track_name,
    .track_default = track_default,
    .track_s = track_s,
    .track_e = track_e,
    .check_n = check_n,
    .func_proc = &func_proc,
    .track_scale = track_scale,
};
ExEdit::Filter filter_ef = {
    .flag = ExEdit::Filter::Flag::Audio,
    .name = name,
    .track_n = track_n,
    .track_name = track_name,
    .track_default = track_default,
    .track_s = track_s,
    .track_e = track_e,
    .check_n = check_n,
    .func_proc = &func_proc,
    .track_scale = track_scale,
};
ExEdit::Filter* filter_list[] = {
    &effect_ef, &filter_ef,
    NULL
};
EXTERN_C __declspec(dllexport)ExEdit::Filter** __stdcall GetFilterTableList() {
    return filter_list;
}
