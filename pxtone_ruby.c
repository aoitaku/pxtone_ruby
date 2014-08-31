/*
###################################
#
# PXtone/Ruby 0.0.1
#
###################################
*/

#include <ruby.h>
#include "ruby/encoding.h"
#include <windows.h>
#include "pxtoneWin32.h"

#define PXTONE_RUBY_VERSION "0.0.1"
#define PXTONE_DLL_VERSION "0.9.2.5"

static VALUE mPxtone;
static VALUE ePxtoneError; // 例外.

static HWND g_hWnd; // グローバルなウィンドウハンドル.

static VALUE Pxtone_reset(VALUE object, VALUE channel_num, VALUE sps, VALUE bps)
{
    if (!pxtone_Reset(
        g_hWnd,
        NUM2INT(channel_num),
        NUM2INT(sps),
        NUM2INT(bps),
        0.1f,
        TRUE,
        NULL
    ))
    {
        rb_raise(ePxtoneError, "pxtoneWin32.DLLの再設定に失敗しました");
    }
    else
    {
        return Qtrue;
    }
}

static VALUE Pxtone_last_error(VALUE object)
{
    const char* last_error = pxtone_GetLastError();
    return rb_enc_str_new(
        last_error,
        strlen(last_error),
        rb_enc_from_index(rb_enc_find_index("Windows-31J"))
    );
}

static VALUE Pxtone_quality(VALUE object)
{
    int channel_num, sps, bps, smp_per_buf;
    pxtone_GetQuality(&channel_num, &sps, &bps, &smp_per_buf);
    return rb_ary_new3(
        3,
        INT2NUM(channel_num),
        INT2NUM(sps),
        INT2NUM(bps)
    );
}

static VALUE Pxtone_load_tune(VALUE object, VALUE filename)
{
    Check_Type(filename, T_STRING);
    if (!pxtone_Tune_Load(NULL, NULL, RSTRING_PTR(filename)))
    {
        // 例外投げるのがいいのか戻り値で読み込み失敗を返すのがいいのか.
        // rb_raise(ePxtoneError, "ファイルの読み込みに失敗しました");
        return Qfalse;
    }
    else
    {
        return Qtrue;
    }
}

static VALUE Pxtone_release_tune(VALUE object)
{
    if (pxtone_Tune_Release())
    {
        return Qtrue;
    }
    else
    {
        return Qfalse;
    }
}

static VALUE Pxtone_play(int argc, VALUE *argv, VALUE object)
{
    VALUE start_sample, volume;
    rb_scan_args(argc, argv, "02", &start_sample, &volume);

    if (pxtone_Tune_Start(
        start_sample == Qnil ? 0 : NUM2INT(start_sample),
        0,
        volume == Qnil ? 1.0f : (float)NUM2DBL(volume)))
    {
        return Qtrue;
    }
    else
    {
        return Qfalse;
    }
}

static VALUE Pxtone_fadein(int argc, VALUE *argv, VALUE object)
{
    VALUE fadein_msec, start_sample, volume;
    rb_scan_args(argc, argv, "12", &fadein_msec, &start_sample, &volume);

    if (pxtone_Tune_Start(
        start_sample == Qnil ? 0 : NUM2INT(start_sample),
        NUM2INT(fadein_msec),
        volume == Qnil ? 1.0f : (float)NUM2DBL(volume)))
    {
        return Qtrue;
    }
    else
    {
        return Qfalse;
    }
}

static VALUE Pxtone_fadeout(VALUE object, VALUE fadeout_msec)
{
    return INT2NUM(pxtone_Tune_Fadeout(NUM2INT(fadeout_msec)));
}

static VALUE Pxtone_set_volume(VALUE object, VALUE volume)
{
    pxtone_Tune_SetVolume((float)NUM2DBL(volume));
    return Qnil;
}

static VALUE Pxtone_stop(VALUE object)
{
    return INT2NUM(pxtone_Tune_Stop());
}

static VALUE Pxtone_loop_on(VALUE object)
{
    pxtone_Tune_SetLoop(TRUE);
    return Qnil;
}

static VALUE Pxtone_loop_off(VALUE object)
{
    pxtone_Tune_SetLoop(FALSE);
    return Qnil;
}

static VALUE Pxtone_IsPlaying(VALUE object)
{
    if (pxtone_Tune_IsStreaming())
    {
        return Qtrue;
    }
    else
    {
        return Qfalse;
    }
}

static VALUE Pxtone_repeat_measure(VALUE object)
{
    return INT2NUM(pxtone_Tune_GetRepeatMeas());
}

static VALUE Pxtone_play_measure(VALUE object)
{
    return INT2NUM(pxtone_Tune_GetPlayMeas());
}

static VALUE Pxtone_tune_information(VALUE object)
{
    int beat_num, beat_clock, meas_num;
    float beat_tempo;
    pxtone_Tune_GetInformation(&beat_num, &beat_tempo, &beat_clock, &meas_num);
    return rb_ary_new3(
        4,
        INT2NUM(beat_num),
        rb_Float(beat_tempo),
        INT2NUM(beat_clock),
        INT2NUM(meas_num)
    );
}

static VALUE Pxtone_tune_name(VALUE object)
{
    const char *tune_name = pxtone_Tune_GetName();
    return rb_enc_str_new(
        tune_name,
        strlen(tune_name),
        rb_enc_from_index(rb_enc_find_index("Windows-31J"))
    );
}

static VALUE Pxtone_tune_comment(VALUE object)
{
    const char *tune_comment = pxtone_Tune_GetComment();
    return rb_enc_str_new(
        tune_comment,
        strlen(tune_comment),
        rb_enc_from_index(rb_enc_find_index("Windows-31J"))
    );
}

static void Pxtone_shutdown(VALUE object)
{
    pxtone_Release();
}

void Init_Pxtone(void)
{
    WNDCLASSEX wcex;
    HINSTANCE hInstance;

    // 例外を定義.
    ePxtoneError = rb_define_class("PxtoneError", rb_eRuntimeError);

    /* インスタンスハンドル取得 */
    hInstance = (HINSTANCE)GetModuleHandle(NULL);

    /* ウインドウ・クラスの登録 */
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = 0;
    wcex.lpfnWndProc   = DefWindowProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = 0;
    wcex.hIconSm       = 0;
    wcex.hCursor       = 0;
    wcex.hbrBackground = 0;
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = "Pxtone";

    if (!RegisterClassEx(&wcex))
    {
        rb_raise(ePxtoneError, "ウィンドウの初期化に失敗しました - RegusterClassEx" );
    }

    g_hWnd = CreateWindow("Pxtone", "", WS_POPUP, 0, 0, 0, 0, 0, NULL, hInstance, NULL);

    if (!pxtone_Ready(
        g_hWnd,
        2,
        44100,
        16,
        0.1f,
        TRUE,
        NULL
    ))
    {
        rb_raise( ePxtoneError, "pxtoneWin32.dllがロードできませんでした" );
    }

    // Pxtone モジュールを定義.
    mPxtone = rb_define_module("Pxtone");

    rb_define_singleton_method(mPxtone, "reset", Pxtone_reset, 3);
    rb_define_singleton_method(mPxtone, "last_error", Pxtone_last_error, 0);
    rb_define_singleton_method(mPxtone, "quality", Pxtone_quality, 4);
    rb_define_singleton_method(mPxtone, "load_tune", Pxtone_load_tune, 1);
    rb_define_singleton_method(mPxtone, "release_tune", Pxtone_release_tune, 0);
    rb_define_singleton_method(mPxtone, "play", Pxtone_play, -1);
    rb_define_singleton_method(mPxtone, "fadein", Pxtone_fadein, -1);
    rb_define_singleton_method(mPxtone, "fadeout", Pxtone_fadeout, 1);
    rb_define_singleton_method(mPxtone, "set_volume", Pxtone_set_volume, 1);
    rb_define_singleton_method(mPxtone, "stop", Pxtone_stop, 0);
    rb_define_singleton_method(mPxtone, "loop_on", Pxtone_loop_on, 0);
    rb_define_singleton_method(mPxtone, "loop_off", Pxtone_loop_off, 0);
    rb_define_singleton_method(mPxtone, "playing?", Pxtone_IsPlaying, 0);
    rb_define_singleton_method(mPxtone, "repeat_measure", Pxtone_repeat_measure, 0);
    rb_define_singleton_method(mPxtone, "play_measure", Pxtone_play_measure, 0);
    rb_define_singleton_method(mPxtone, "tune_information", Pxtone_tune_information, 0);
    rb_define_singleton_method(mPxtone, "tune_name", Pxtone_tune_name, 0);
    rb_define_singleton_method(mPxtone, "tune_comment", Pxtone_tune_comment, 0);

    rb_define_const(mPxtone, "VERSION", rb_str_new2(PXTONE_RUBY_VERSION));
    rb_define_const(mPxtone, "DLL_VERSION", rb_str_new2(PXTONE_DLL_VERSION));

    rb_set_end_proc((void(*)(VALUE))Pxtone_shutdown, Qnil);
}
