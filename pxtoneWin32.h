#ifdef PXTONEDLL_EXPORTS
#define DLLAPI __declspec(dllexport) // DLL ����鑤�� export �Ƃ���
#else
#define DLLAPI __declspec(dllimport) // DLL ���g������ import �Ƃ���
#endif

// C++ �ł� C �ł��g����悤�ɂ���
#ifdef __cplusplus
extern "C"{
#endif

#include <stdbool.h>

typedef bool (* PXTONEPLAY_CALLBACK)( int clock );

// �ȉ� pxtone�֐��Q ==========================================

// pxtone �𐶐����܂��B
DLLAPI bool pxtone_Ready(
	HWND hWnd,          // �E�C���h�E�n���h����n���Ă�������
	int channel_num,   // �̃`�����l�������w�肵�Ă��������B( 1:���m���� / 2:�X�e���I )
	int sps,           // �b�ԃT���v�����O���[�g�ł��B      ( 11025 / 22050 / 44100 )
	int bps,           // �P�T���v����\������r�b�g���ł��B( 8 / 16 )
	float buffer_sec,   // �Ȃ��Đ�����̂Ɏg�p����o�b�t�@�T�C�Y��b�Ŏw�肵�܂��B( ���� 0.1 )
	bool bDirectSound,  // true: DirectSound ���g�p���܂� / false: WAVEMAPPER ���g�p���܂��B
	PXTONEPLAY_CALLBACK pProc // �T���v�����O���ɌĂ΂��֐��ł��BNULL �ł��܂��܂���B
); // pxtone �̏���

// pxtone ���Đݒ肵�܂��B
DLLAPI bool pxtone_Reset(
	HWND hWnd,
	int channel_num,
	int sps,
	int bps,
	float buffer_sec,
	bool bDirectSound,
	PXTONEPLAY_CALLBACK pProc
);

// pxtone �Ő������ꂽDirectSound�̃|�C���^(LPDIRECTSOUND)���擾����B
// �擾����DirectSound�͎����Ń����[�X���Ȃ��悤�ɒ��ӂ��Ă��������B
DLLAPI void *pxtone_GetDirectSound( void );

// ���X�g�G���[������擾
DLLAPI const char *pxtone_GetLastError( void );

// pxtone �̉������擾���܂�
DLLAPI void pxtone_GetQuality( int *p_channel_num, int *p_sps, int *p_bps, int *p_smp_per_buf ); // pxtone

// pxtone ���J�����܂�
DLLAPI bool pxtone_Release( void );

// �Ȃ�ǂݍ��݂܂�(�t�@�C���E���\�[�X����)
DLLAPI bool pxtone_Tune_Load(
	HMODULE hModule,       // ���\�[�X����ǂޏꍇ�̓��W���[���n���h�����w�肵�܂��BNULL �ł����Ȃ������B
	const char *type_name, // ���\�[�X����ǂޏꍇ�̓��\�[�X�̎�ޖ��B�O���t�@�C����ǂޏꍇ�� NULL�B
	const char *file_name  // �t�@�C���p�X�������̓��\�[�X���B
	);

// �Ȃ�ǂݍ���(����������)
DLLAPI bool pxtone_Tune_Read( void* p, int size );

// �Ȃ�������܂�
DLLAPI bool pxtone_Tune_Release( void );

// �Ȃ��Đ����܂�
DLLAPI bool pxtone_Tune_Start(
	int start_sample,     // �J�n�ʒu�ł��B��� Stop �� Fadeout �Ŏ擾�����l��ݒ肵�܂��B0 �ōŏ�����B
	int fadein_msec ,       // �t�F�[�h�C������ꍇ�͂����Ɏ��ԁi�~���b�j���w�肵�܂��B
	float volume
	);

// �t�F�[�h�A�E�g�X�C�b�`�����Č��ݍĐ��T���v�����擾���܂�
DLLAPI int pxtone_Tune_Fadeout( int msec );

// �Ȃ̃{�����[����ݒ肵�܂��B1.0 ���ő�ŁA0.5 �������ł��B
DLLAPI void pxtone_Tune_SetVolume( float v );

// �Ȃ��~���Č��ݍĐ��T���v�����擾
DLLAPI int pxtone_Tune_Stop( void );

// �Đ������ǂ����𒲂ׂ܂�
DLLAPI bool pxtone_Tune_IsStreaming( void );

// ���[�v�Đ��� ON/OFF ��؂�ւ��܂�
DLLAPI void pxtone_Tune_SetLoop( bool bLoop );

// �Ȃ̏����擾���܂�
DLLAPI void pxtone_Tune_GetInformation( int *p_beat_num, float *p_beat_tempo, int *p_beat_clock, int *p_meas_num );

// ���s�[�g���߂��擾���܂�
DLLAPI int pxtone_Tune_GetRepeatMeas( void );

// �L�����t���߂��擾���܂�(LAST�C�x���g���߁B������΍ŏI����)
DLLAPI int pxtone_Tune_GetPlayMeas(   void );

// �Ȃ̖��̂��擾���܂�
DLLAPI const char *pxtone_Tune_GetName(    void );

// �Ȃ̃R�����g���擾���܂�
DLLAPI const char *pxtone_Tune_GetComment( void );

// ���w��̃A�h���X�ɍĐ��o�b�t�@���������݂܂�
DLLAPI bool pxtone_Tune_Vomit(
	void* p,         // �Đ��o�b�t�@��f���o���A�h���X�ł�
	int sample_num  // �������ރT���v�����ł�
	);
// �߂�l�F�܂�����������ꍇ�� true ����ȊO�� false
// �P�A���̊֐����g�p����ꍇ�� pxtone_Ready() �̈��� buffer_sec �ɂ� 0 ��ݒ肵�Ă��������B
//     pxtone �̃X�g���[�~���O�@�\�������ɂȂ�܂��B
// �Q�A�Ȃ̃��[�h�� pxtone_Tune_Start() ���I���Ă��炱�̊֐����Ăяo���Ă��������B
// �R�Asample_num �̓T�C�Y�ł͂Ȃ��ăT���v�����ł��B
//     ��) 11025hz 2ch 8bit ���P�b�f���o���ꍇ�Asample_num �� 11025 ���w�肷��B
//         p �ɂ� 22050�o�C�g���̍Đ��o�b�t�@���������܂��B
// �S�A�X�g���[�~���O�@�\���L���Ȏ��Ɠ��l�� pxtone_Tune_Fadeout() ���̊֐����g���܂��B




// �s�X�g���m�C�Y�𐶐����܂�
typedef struct
{
	unsigned char* p_buf;
	int            size ;
}
PXTONENOISEBUFFER;

DLLAPI void pxtone_Noise_Initialize( void ); // pxtone_Ready() �����ɌĂ�ł���ꍇ�͕K�v����܂���B
DLLAPI void pxtone_Noise_Release( PXTONENOISEBUFFER* p_noise );
DLLAPI PXTONENOISEBUFFER *pxtone_Noise_Create(
	const char* name   ,     // ���\�[�X��      ��ݒ�B�O���t�@�C���̏ꍇ�̓t�@�C���p�X�B
	const char* type   ,     // ���\�[�X�^�C�v����ݒ�B�O���t�@�C���̏ꍇ��NULL�B
	int        channel_num, int sps, int bps );
DLLAPI PXTONENOISEBUFFER *pxtone_Noise_Create_FromMemory(
	const char* p_designdata   , // �m�C�Y�f�U�C�����������o�b�t�@�ւ̃|�C���^
	int         designdata_size, // �m�C�Y�f�U�C�����������o�b�t�@�̃T�C�Y
	int        channel_num , int sps, int bps );

#ifdef __cplusplus
}
#endif
