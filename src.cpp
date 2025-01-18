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
    int next_track_values[track_n];
    int* next_track = next_track_values;

    int tl_nextframe, tl_nextsubframe;
    if (efpip->audio_speed == 0) {
        tl_nextframe = min(efpip->frame_num + 1, efp->frame_end_chain);
        tl_nextsubframe = 0;
    } else {
        int tl_nextmilli = min(efpip->audio_milliframe + efpip->audio_speed / 1000, efp->frame_end_chain * 1000);
        tl_nextframe = tl_nextmilli / 1000;
        tl_nextsubframe = (tl_nextmilli % 1000) / 10;
    }
    efp->exfunc->calc_trackbar(efp->processing, tl_nextframe, tl_nextsubframe, next_track_values, nullptr);
    
    double d_volume = (double)max(0, efp->track[0]) * 4.096;
    double d_right = (double)max(0, efp->track[2]) * d_volume * 0.001;
    double d_left = (double)max(0, efp->track[3]) * d_volume * 0.001;
    if (efpip->audio_ch == 2) {
        int lr = std::clamp(efp->track[1], -1000, 1000);
        if (0 < lr) {
            d_right *= (double)(1000 - lr) * 0.001;
        } else if (lr < 0) {
            d_left *= (double)(lr + 1000) * 0.001;
        }
    }
    double next_volume = (double)max(0, next_track[0]) * 4.096;
    double next_right = (double)max(0, next_track[2]) * next_volume * 0.001;
    double next_left = (double)max(0, next_track[3]) * next_volume * 0.001;
    if (efpip->audio_ch == 2) {
        int lr = std::clamp(next_track[1], -1000, 1000);
        if (0 < lr) {
            next_right *= (double)(1000 - lr) * 0.001;
        } else if (lr < 0) {
            next_left *= (double)(lr + 1000) * 0.001;
        }
    }

    double step = 65536.0 / (double)efpip->audio_n;
    int step_o = (int)((next_volume - d_volume) * step);
    int step_r = (int)((next_right - d_right) * step);
    int step_l = (int)((next_left - d_left) * step);

    int other = (int)d_volume;
    int right = (int)d_right;
    int left = (int)d_left;

    int volume_fo = (int)((d_volume - (double)other) * 65536.0);
    int volume_fr = (int)((d_right - (double)right) * 65536.0);
    int volume_fl = (int)((d_left - (double)left) * 65536.0);

    short* volume_fho = (short*)&volume_fo + 1;
    short* volume_fhr = (short*)&volume_fr + 1;
    short* volume_fhl = (short*)&volume_fl + 1;

    short* audiop;
    if ((byte)efp->flag & (byte)(ExEdit::Filter::Flag::Effect)) {
        audiop = efpip->audio_data;
    } else {
        audiop = efpip->audio_p;
    }

    switch (efpip->audio_ch) {
    case 1: {
        for (int i = efpip->audio_n; 0 < i; i--) {
            *audiop = (short)std::clamp((int)*audiop * (other + *volume_fho) >> 12, SHRT_MIN, SHRT_MAX);
            audiop++;
            volume_fo += step_o;
        }
    }break;
    case 2: {
        for (int i = efpip->audio_n; 0 < i; i--) {
            *audiop = (short)std::clamp((int)*audiop * (right + *volume_fhr) >> 12, SHRT_MIN, SHRT_MAX);
            audiop++;
            volume_fr += step_r;

            *audiop = (short)std::clamp((int)*audiop * (left + *volume_fhl) >> 12, SHRT_MIN, SHRT_MAX);
            audiop++;
            volume_fl += step_l;
        }
    }break;
    default: {
        for (int i = efpip->audio_n; 0 < i; i--) {
            *audiop = (short)std::clamp((int)*audiop * (right + *volume_fhr) >> 12, SHRT_MIN, SHRT_MAX);
            audiop++;
            volume_fr += step_r;

            *audiop = (short)std::clamp((int)*audiop * (left + *volume_fhl) >> 12, SHRT_MIN, SHRT_MAX);
            audiop++;
            volume_fl += step_l;
            for (int j = efpip->audio_ch - 2; 0 < j; j--) {
                *audiop = (short)std::clamp((int)*audiop * (other + *volume_fho) >> 12, SHRT_MIN, SHRT_MAX);
                audiop++;
            }
            volume_fo += step_o;
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
