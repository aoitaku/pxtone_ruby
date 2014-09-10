/*
###################################
#
# PXtone/Ruby 0.0.3
#
###################################
*/

#include <ruby.h>
#include "ruby/intern.h"
#include "ruby/encoding.h"
#include <windows.h>
#include "pxtoneWin32.h"

#define PXTONE_RUBY_VERSION "0.0.3"
#define PXTONE_DLL_VERSION "0.9.2.5"

static VALUE mPxtone;
static VALUE cPxtoneNoise;
static VALUE ePxtoneError; // 例外.

static HWND  g_hWnd;   // グローバルなウィンドウハンドル.
static float g_volume; // 現在ボリュームの保持.
static int   g_sample; // 前回再生位置の保持.
static bool  g_pause;  // ポーズ中かどうか.

struct PxtoneNoiseData {
    PXTONENOISEBUFFER* buffer;
    int channel_num;
    int sps;
    int bps;
};

struct PxtoneWaveHeader {
    char riff[4]; // = "RIFF"
    unsigned long total_size; // 全体サイズ
    char fmt[8]; // "WAVEfmt "
    unsigned long fmt_size; // fmt チャンクサイズ
    unsigned short format; // フォーマットの種類
    unsigned short channel; // チャンネル
    unsigned long rate; // サンプリングレート
    unsigned long avgbyte; // rate * block
    unsigned short block; // channels * bit / 8
    unsigned short bit; // ビット数
    char data[4]; // = "data"
    unsigned long data_size; // データサイズ
};

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

    g_volume = NIL_P(volume) ? 1.0f : (float)NUM2DBL(volume);
    g_pause = false;

    if (pxtone_Tune_Start(
        NIL_P(start_sample) ? 0 : NUM2INT(start_sample),
        0,
        g_volume))
    {
        return Qtrue;
    }
    else
    {
        return Qfalse;
    }
}

static VALUE Pxtone_resume(int argc, VALUE *argv, VALUE object)
{
    VALUE volume;

    if (!g_pause)
    {
        return Qfalse;
    }
    g_pause = false;

    rb_scan_args(argc, argv, "01", &volume);
    if (!NIL_P(volume))
    {
        g_volume = (float)NUM2DBL(volume);
    }

    if (pxtone_Tune_Start(g_sample, 0, g_volume))
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

    g_volume = NIL_P(volume) ? 1.0f : (float)NUM2DBL(volume);
    g_pause = false;

    if (pxtone_Tune_Start(
        NIL_P(start_sample) ? 0 : NUM2INT(start_sample),
        NUM2INT(fadein_msec),
        g_volume))
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
    g_sample = pxtone_Tune_Fadeout(NUM2INT(fadeout_msec));
    g_pause  = false;
    return INT2NUM(g_sample);
}

static VALUE Pxtone_volume(VALUE object)
{
    return rb_float_new(g_volume);
}

static VALUE Pxtone_set_volume(VALUE object, VALUE volume)
{
    g_volume = (float)NUM2DBL(volume);
    pxtone_Tune_SetVolume(g_volume);
    return Qnil;
}

static VALUE Pxtone_stop(VALUE object)
{
    // 停止したときは 0 に戻す.
    g_sample = 0;
    g_pause = false;
    return INT2NUM(pxtone_Tune_Stop());
}

static VALUE Pxtone_pause(VALUE object)
{
    g_sample = pxtone_Tune_Stop();
    g_pause = true;
    return INT2NUM(g_sample);
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

static VALUE Pxtone_IsPaused(VALUE object)
{
    if (g_pause)
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
        rb_float_new(beat_tempo),
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

static void PxtoneNoise_free(struct PxtoneNoiseData *noise)
{
    if (noise->buffer != NULL)
    {
        pxtone_Noise_Release(noise->buffer);
        noise->buffer = NULL;
    }
    free(noise);
}

static VALUE PxtoneNoise_allocate(VALUE object)
{
    struct PxtoneNoiseData *noise = ALLOC(struct PxtoneNoiseData);
    noise->buffer = NULL;
    return Data_Wrap_Struct(object, NULL, PxtoneNoise_free, noise);
}

static VALUE PxtoneNoise_initialize(int argc, VALUE *argv, VALUE self)
{
    struct PxtoneNoiseData *noise;
    VALUE filename, channel_num, sps, bps;
    rb_scan_args(argc, argv, "13", &filename, &channel_num, &sps, &bps);
    Data_Get_Struct(self, struct PxtoneNoiseData, noise);
    noise->buffer = pxtone_Noise_Create(
        RSTRING_PTR(filename),
        NULL,
        NIL_P(channel_num) ? 1 : NUM2INT(channel_num),
        NIL_P(sps) ? 44100 : NUM2INT(sps),
        NIL_P(bps) ? 8 : NUM2INT(bps)
    );
    noise->channel_num = NIL_P(channel_num) ? 1 : NUM2INT(channel_num);
    noise->sps = NIL_P(sps) ? 44100 : NUM2INT(sps);
	noise->bps = NIL_P(bps) ? 8 : NUM2INT(bps);
    return self;
}

static VALUE PxtoneNoise_dispose(VALUE self)
{
    struct PxtoneNoiseData *noise;
    Data_Get_Struct(self, struct PxtoneNoiseData, noise);
    if (noise->buffer != NULL)
    {
        pxtone_Noise_Release(noise->buffer);
        noise->buffer = NULL;
    }
    free(noise);
    return self;
}

static VALUE PxtoneNoise_to_a(VALUE self)
{
    struct PxtoneNoiseData *noise;
    int i;
    double buf;
    VALUE buffer;
    VALUE note;
    Data_Get_Struct(self, struct PxtoneNoiseData, noise);
    if (noise->buffer != NULL)
    {
        if (noise->bps == 16)
        {
            buffer = rb_ary_new2(noise->sps);
            for (i = 0; i < noise->sps; i++)
            {
                buf = ((noise->buffer->p_buf[i*2] | (0 | noise->buffer->p_buf[i*2+1]) << 8));
                buf = (buf < 246 ? 0 : buf - 246) / -32768;
                note = rb_float_new(
                    buf < - 1.0 ? 2 + buf : buf
                );
                rb_ary_store(buffer, i, note);
            }
            return buffer;
        }
        else
        {
            buffer = rb_ary_new2(noise->sps);
            for (i = 0; i < noise->sps; i++)
            {
                note = rb_float_new(((float)noise->buffer->p_buf[i] - 128) / -128);
                rb_ary_store(buffer, i, note);
            }
            return buffer;
        }
    }
    else
    {
        return Qnil;
    }
}

static VALUE PxtoneNoise_to_s(VALUE self)
{
    struct PxtoneNoiseData *noise;
    struct PxtoneWaveHeader *header;
    VALUE buffer;
    VALUE wave_buffer;
    Data_Get_Struct(self, struct PxtoneNoiseData, noise);
    if (noise->buffer != NULL)
    {
        header = rb_alloc_tmp_buffer(&buffer, sizeof(struct PxtoneWaveHeader));
        strcpy(header->riff, "RIFF");
        header->total_size = sizeof(struct PxtoneWaveHeader) - 8 + noise->buffer->size;
        strcpy(header->fmt, "WAVEfmt ");
        header->fmt_size = 16;
        header->format = 1;
        header->channel = noise->channel_num;
        header->rate = noise->sps;
        header->avgbyte = noise->sps * noise->channel_num * noise->bps / 8;
        header->block = noise->channel_num * noise->bps / 8;
        header->bit = noise->bps;
        strcpy(header->data, "data");
        header->data_size = noise->buffer->size;
        wave_buffer = rb_str_cat(rb_str_new((void*)header, sizeof(struct PxtoneWaveHeader)), (const char*)noise->buffer->p_buf, noise->buffer->size);
        rb_free_tmp_buffer(&buffer);
        return wave_buffer;
    }
    else
    {
        return Qnil;
    }
}

static VALUE PxtoneNoise_channel_num(VALUE self)
{
    struct PxtoneNoiseData *noise;
    Data_Get_Struct(self, struct PxtoneNoiseData, noise);
    return INT2NUM(noise->channel_num);
}

static VALUE PxtoneNoise_sps(VALUE self)
{
    struct PxtoneNoiseData *noise;
    Data_Get_Struct(self, struct PxtoneNoiseData, noise);
    return INT2NUM(noise->sps);
}

static VALUE PxtoneNoise_bps(VALUE self)
{
    struct PxtoneNoiseData *noise;
    Data_Get_Struct(self, struct PxtoneNoiseData, noise);
    return INT2NUM(noise->bps);
}

static void Pxtone_shutdown(VALUE object)
{
    // 再生中なら念の為に停止しておく.
    if (pxtone_Tune_IsStreaming())
    {
        pxtone_Tune_Stop();
    }
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
    rb_define_singleton_method(mPxtone, "resume", Pxtone_resume, -1);
    rb_define_singleton_method(mPxtone, "fadein", Pxtone_fadein, -1);
    rb_define_singleton_method(mPxtone, "fadeout", Pxtone_fadeout, 1);
    rb_define_singleton_method(mPxtone, "volume", Pxtone_volume, 0);
    rb_define_singleton_method(mPxtone, "set_volume", Pxtone_set_volume, 1);
    rb_define_singleton_method(mPxtone, "stop", Pxtone_stop, 0);
    rb_define_singleton_method(mPxtone, "pause", Pxtone_pause, 0);
    rb_define_singleton_method(mPxtone, "loop_on", Pxtone_loop_on, 0);
    rb_define_singleton_method(mPxtone, "loop_off", Pxtone_loop_off, 0);
    rb_define_singleton_method(mPxtone, "playing?", Pxtone_IsPlaying, 0);
    rb_define_singleton_method(mPxtone, "paused?", Pxtone_IsPaused, 0);
    rb_define_singleton_method(mPxtone, "repeat_measure", Pxtone_repeat_measure, 0);
    rb_define_singleton_method(mPxtone, "play_measure", Pxtone_play_measure, 0);
    rb_define_singleton_method(mPxtone, "tune_information", Pxtone_tune_information, 0);
    rb_define_singleton_method(mPxtone, "tune_name", Pxtone_tune_name, 0);
    rb_define_singleton_method(mPxtone, "tune_comment", Pxtone_tune_comment, 0);

    rb_define_const(mPxtone, "VERSION", rb_str_new2(PXTONE_RUBY_VERSION));
    rb_define_const(mPxtone, "DLL_VERSION", rb_str_new2(PXTONE_DLL_VERSION));

    cPxtoneNoise = rb_define_class("PxtoneNoise", rb_cObject);
    rb_define_alloc_func(cPxtoneNoise, PxtoneNoise_allocate);
    rb_define_private_method(cPxtoneNoise, "initialize", PxtoneNoise_initialize, -1);
    rb_define_method(cPxtoneNoise, "dispose", PxtoneNoise_dispose, 0);
    rb_define_method(cPxtoneNoise, "to_a", PxtoneNoise_to_a, 0);
    rb_define_method(cPxtoneNoise, "to_s", PxtoneNoise_to_s, 0);
    rb_define_method(cPxtoneNoise, "channel_num", PxtoneNoise_channel_num, 0);
    rb_define_method(cPxtoneNoise, "sps", PxtoneNoise_sps, 0);
    rb_define_method(cPxtoneNoise, "bps", PxtoneNoise_bps, 0);


    g_volume = 1.0f;
    g_sample = 0;
    g_pause = false;

    rb_set_end_proc((void(*)(VALUE))Pxtone_shutdown, Qnil);
}
