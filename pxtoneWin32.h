#ifdef PXTONEDLL_EXPORTS
#define DLLAPI __declspec(dllexport) // DLL を作る側は export として
#else
#define DLLAPI __declspec(dllimport) // DLL を使う側は import として
#endif

// C++ でも C でも使えるようにする
#ifdef __cplusplus
extern "C"{
#endif

#include <stdbool.h>

typedef bool (* PXTONEPLAY_CALLBACK)( int clock );

// 以下 pxtone関数群 ==========================================

// pxtone を生成します。
DLLAPI bool pxtone_Ready(
	HWND hWnd,          // ウインドウハンドルを渡してください
	int channel_num,   // のチャンネル数を指定してください。( 1:モノラル / 2:ステレオ )
	int sps,           // 秒間サンプリングレートです。      ( 11025 / 22050 / 44100 )
	int bps,           // １サンプルを表現するビット数です。( 8 / 16 )
	float buffer_sec,   // 曲を再生するのに使用するバッファサイズを秒で指定します。( 推奨 0.1 )
	bool bDirectSound,  // true: DirectSound を使用します / false: WAVEMAPPER を使用します。
	PXTONEPLAY_CALLBACK pProc // サンプリング毎に呼ばれる関数です。NULL でかまいません。
); // pxtone の準備

// pxtone を再設定します。
DLLAPI bool pxtone_Reset(
	HWND hWnd,
	int channel_num,
	int sps,
	int bps,
	float buffer_sec,
	bool bDirectSound,
	PXTONEPLAY_CALLBACK pProc
);

// pxtone で生成されたDirectSoundのポインタ(LPDIRECTSOUND)を取得する。
// 取得したDirectSoundは自分でリリースしないように注意してください。
DLLAPI void *pxtone_GetDirectSound( void );

// ラストエラー文字列取得
DLLAPI const char *pxtone_GetLastError( void );

// pxtone の音質を取得します
DLLAPI void pxtone_GetQuality( int *p_channel_num, int *p_sps, int *p_bps, int *p_smp_per_buf ); // pxtone

// pxtone を開放します
DLLAPI bool pxtone_Release( void );

// 曲を読み込みます(ファイル・リソースから)
DLLAPI bool pxtone_Tune_Load(
	HMODULE hModule,       // リソースから読む場合はモジュールハンドルを指定します。NULL でも問題ないかも。
	const char *type_name, // リソースから読む場合はリソースの種類名。外部ファイルを読む場合は NULL。
	const char *file_name  // ファイルパスもしくはリソース名。
	);

// 曲を読み込む(メモリから)
DLLAPI bool pxtone_Tune_Read( void* p, int size );

// 曲を解放します
DLLAPI bool pxtone_Tune_Release( void );

// 曲を再生します
DLLAPI bool pxtone_Tune_Start(
	int start_sample,     // 開始位置です。主に Stop や Fadeout で取得した値を設定します。0 で最初から。
	int fadein_msec ,       // フェードインする場合はここに時間（ミリ秒）を指定します。
	float volume
	);

// フェードアウトスイッチを入れて現在再生サンプルを取得します
DLLAPI int pxtone_Tune_Fadeout( int msec );

// 曲のボリュームを設定します。1.0 が最大で、0.5 が半分です。
DLLAPI void pxtone_Tune_SetVolume( float v );

// 曲を停止して現在再生サンプルを取得
DLLAPI int pxtone_Tune_Stop( void );

// 再生中かどうかを調べます
DLLAPI bool pxtone_Tune_IsStreaming( void );

// ループ再生の ON/OFF を切り替えます
DLLAPI void pxtone_Tune_SetLoop( bool bLoop );

// 曲の情報を取得します
DLLAPI void pxtone_Tune_GetInformation( int *p_beat_num, float *p_beat_tempo, int *p_beat_clock, int *p_meas_num );

// リピート小節を取得します
DLLAPI int pxtone_Tune_GetRepeatMeas( void );

// 有効演奏小節を取得します(LASTイベント小節。無ければ最終小節)
DLLAPI int pxtone_Tune_GetPlayMeas(   void );

// 曲の名称を取得します
DLLAPI const char *pxtone_Tune_GetName(    void );

// 曲のコメントを取得します
DLLAPI const char *pxtone_Tune_GetComment( void );

// ■指定のアドレスに再生バッファを書き込みます
DLLAPI bool pxtone_Tune_Vomit(
	void* p,         // 再生バッファを吐き出すアドレスです
	int sample_num  // 書き込むサンプル数です
	);
// 戻り値：まだ続きがある場合は true それ以外は false
// １、この関数を使用する場合は pxtone_Ready() の引数 buffer_sec には 0 を設定してください。
//     pxtone のストリーミング機能が無効になります。
// ２、曲のロードと pxtone_Tune_Start() を終えてからこの関数を呼び出してください。
// ３、sample_num はサイズではなくてサンプル数です。
//     例) 11025hz 2ch 8bit を１秒吐き出す場合、sample_num は 11025 を指定する。
//         p には 22050バイト分の再生バッファが書き込まれる。
// ４、ストリーミング機能が有効な時と同様に pxtone_Tune_Fadeout() 等の関数が使えます。




// ピストンノイズを生成します
typedef struct
{
	unsigned char* p_buf;
	int            size ;
}
PXTONENOISEBUFFER;

DLLAPI void pxtone_Noise_Initialize( void ); // pxtone_Ready() を既に呼んでいる場合は必要ありません。
DLLAPI void pxtone_Noise_Release( PXTONENOISEBUFFER* p_noise );
DLLAPI PXTONENOISEBUFFER *pxtone_Noise_Create(
	const char* name   ,     // リソース名      を設定。外部ファイルの場合はファイルパス。
	const char* type   ,     // リソースタイプ名を設定。外部ファイルの場合はNULL。
	int        channel_num, int sps, int bps );
DLLAPI PXTONENOISEBUFFER *pxtone_Noise_Create_FromMemory(
	const char* p_designdata   , // ノイズデザインが入ったバッファへのポインタ
	int         designdata_size, // ノイズデザインが入ったバッファのサイズ
	int        channel_num , int sps, int bps );

#ifdef __cplusplus
}
#endif
