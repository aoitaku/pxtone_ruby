/*
 * やってないこと:
 * - pxtone_Tune_Read
 *     リソースパッキングしたときくらいしか使わないと思うので後で考える
 * - pxtone_GetDirectSound
 *     ポインタもらってもRuby側で何に使うんだろう
 *     とりあえず実装しない
 * - pxtone_Tune_Vomit
 *     書き出すのはいいんだけどRubyでポインタもらってもどうしようもないので
 *     抽象化を考えないといけない
 *     ポインタからStringオブジェクトを作ればAyame.load_from_memoryとかできる
 *     このとき書き出したメモリはRubyが管理するってことでいいのかな、とか
 *     そのへんも調べる
 * - pause / resume
 *     stopの戻り値使って再生開始位置を指定できるのでとりあえずそれで……
 * - ピストンノイズの読み込み全般
 *     ピスコラ解放時に読み込んだノイズも解放しないとリークするので
 *     読み込んだノイズの管理をしないとダメ
 */

#include <ruby.h>
#include <windows.h>
#include "pxtoneWin32.h"

static VALUE cPxtone;
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
    return rb_str_new2(pxtone_GetLastError());
}

static VALUE Pxtone_quolity(VALUE object)
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

static VALUE Pxtone_play(VALUE argc, VALUE *argv, VALUE object)
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

static VALUE Pxtone_fadein(VALUE argc, VALUE *argv, VALUE object)
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
    return rb_str_new2(pxtone_Tune_GetName());
}

static VALUE Pxtone_tune_comment(VALUE object)
{
    return rb_str_new2(pxtone_Tune_GetComment());
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
    cPxtone = rb_define_module("Pxtone");

    rb_define_singleton_method(cPxtone, "reset", Pxtone_reset, 3);
    rb_define_singleton_method(cPxtone, "last_error", Pxtone_last_error, 0);
    rb_define_singleton_method(cPxtone, "quolity", Pxtone_quolity, 4);
    rb_define_singleton_method(cPxtone, "load_tune", Pxtone_load_tune, 1);
    rb_define_singleton_method(cPxtone, "release_tune", Pxtone_release_tune, 0);
    rb_define_singleton_method(cPxtone, "play", Pxtone_play, -1);
    rb_define_singleton_method(cPxtone, "fadein", Pxtone_fadein, -1);
    rb_define_singleton_method(cPxtone, "fadeout", Pxtone_fadeout, 1);
    rb_define_singleton_method(cPxtone, "set_volume", Pxtone_set_volume, 1);
    rb_define_singleton_method(cPxtone, "stop", Pxtone_stop, 0);
    rb_define_singleton_method(cPxtone, "loop_on", Pxtone_loop_on, 0);
    rb_define_singleton_method(cPxtone, "loop_off", Pxtone_loop_off, 0);
    rb_define_singleton_method(cPxtone, "playing?", Pxtone_IsPlaying, 0);
    rb_define_singleton_method(cPxtone, "repeat_measure", Pxtone_repeat_measure, 0);
    rb_define_singleton_method(cPxtone, "play_measure", Pxtone_play_measure, 0);
    rb_define_singleton_method(cPxtone, "tune_information", Pxtone_tune_information, 0);
    rb_define_singleton_method(cPxtone, "tune_name", Pxtone_tune_name, 0);
    rb_define_singleton_method(cPxtone, "tune_comment", Pxtone_tune_comment, 0);

    rb_set_end_proc((void(*)(VALUE))Pxtone_shutdown, Qnil);
}
