// YanPlayerDlg.h : header file
//

#pragma once

extern "C"{
#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#include <stdint.h>
#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libavformat/avformat.h"
#include "ffmpeg/libavutil/avutil.h"
#include "ffmpeg/libavutil/frame.h"
#include "ffmpeg/libavutil/pixfmt.h"
#include "ffmpeg/libavutil/mem.h"
#include "ffmpeg/libswscale/swscale.h"
#include "ffmpeg/libswresample/swresample.h"
#include "ffmpeg/libavutil/opt.h"
#include "ffmpeg/libavutil/mathematics.h"
#endif
}

#include <D3D9.h>
#pragma comment(lib, "D3d9.lib")  

#include "SoundPlayer.h"

struct FrameStruct 
{
	int iWidth;
	int iHeight;
	char *buf;
};
// CYanPlayerDlg dialog
class CYanPlayerDlg : public CDialog
{
// Construction
public:
	CYanPlayerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_YANPLAYER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	afx_msg void OnBnClickedButtonSelfile();


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	HANDLE m_hthreadCodec;//解码线程句柄
	HANDLE m_hthreadCodecVideo;
	HANDLE m_hthreadCodecAudio;
	HANDLE m_hThreadCodecYUV;
	HANDLE m_hThreadCodecPCM;
	HANDLE m_hThreadEncodeYUV;
	CString m_strFilePath;
	CString m_strDstFilePath;
	DWORD m_dwLastTime;
	DWORD m_dwNowTime;
	BOOL m_bFrameFirst;
	BOOL m_bExit;
	INT64 m_NowPts;
	INT64 m_LastPts;
	
	int m_nWidth;
	int m_nHeight;//保存视频宽高信息
	IDirect3D9 * m_pD3D;
	IDirect3DDevice9 * m_pd3dDevice;
	IDirect3DSurface9 * m_pd3dSurface;//D3D绘图用变量
	CRect m_rtViewport;//视频显示区域（要保持宽高比）
	CSoundPlayer m_SoundPlayer;

	void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);
	void ShowFrame(char *pbuf, int iWidth, int iHeight);
	void ShowFrame(AVFrame *pFrame, int iWidth, int iHeight, AVPacket *pPacket, int nFrameTime);
	void CodecFile();
	int avcodec_decode_video(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, uint8_t *buf, int buf_size);
	BOOL SaveBitmap(BYTE *pBuffer, int iWidth, int iHeight);
	void SetVideoSize(long lWidth, long lHeight);
	void SetYUVbuffer(char *pBuffer,long lLen);
	void DrawImage();
	void InitD3D();
	void CodecVideo(char *pFileName);
	void CodecAudio(char *pFileName);
	void EncodeYUV(CString strSrcFile, CString strDstFile);
	static UINT CALLBACK ThreadCodec(LPVOID lp);
	static UINT CALLBACK ThreadCodecVideo(LPVOID lp);
	static UINT CALLBACK ThreadCodecAudio(LPVOID lp);
	static UINT CALLBACK ThreadCodecPCM(LPVOID lp);
	static UINT CALLBACK ThreadCodecYUV(LPVOID lp);
	static UINT CALLBACK ThreadEncodeYUV(LPVOID lp);
	afx_msg void OnClose();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButtonDecodevideo();
	void CodecYuvFile(CString strSrcFile, CString strDstFile);
	void CodecPCMFile(CString strSrcFile, CString strDstFile);
	afx_msg void OnBnClickedButtonDecodeaudio();
	afx_msg void OnBnClickedButtonSel();
};
