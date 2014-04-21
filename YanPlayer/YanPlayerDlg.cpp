// YanPlayerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "YanPlayer.h"
#include "YanPlayerDlg.h"
#include <process.h>
#include <Mmsystem.h>
#include <fcntl.h>
#pragma comment(lib, "Winmm.lib")  


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int AudioResampling(AVCodecContext * audio_dec_ctx,AVFrame * pAudioDecodeFrame,
										int out_sample_fmt,int out_channels ,int out_sample_rate , uint8_t * out_buf)
{
	//////////////////////////////////////////////////////////////////////////
	SwrContext * swr_ctx = NULL;
	int data_size = 0;
	int ret = 0;
	int64_t src_ch_layout = AV_CH_LAYOUT_STEREO; //初始化这样根据不同文件做调整
	int64_t dst_ch_layout = AV_CH_LAYOUT_STEREO; //这里设定ok
	int dst_nb_channels = 0;
	int dst_linesize = 0;
	int src_nb_samples = 0;
	int dst_nb_samples = 0;
	int max_dst_nb_samples = 0;
	uint8_t **dst_data = NULL;
	int resampled_data_size = 0;

	//重新采样
	if (swr_ctx)
	{
		swr_free(&swr_ctx);
	}
	swr_ctx = swr_alloc();
	if (!swr_ctx)
	{
		printf("swr_alloc error \n");
		return -1;
	}

	src_ch_layout = (audio_dec_ctx->channel_layout && 
		audio_dec_ctx->channels == 
		av_get_channel_layout_nb_channels(audio_dec_ctx->channel_layout)) ? 
		audio_dec_ctx->channel_layout : 
	av_get_default_channel_layout(audio_dec_ctx->channels);

	if (out_channels == 1)
	{
		dst_ch_layout = AV_CH_LAYOUT_MONO;
	}
	else if(out_channels == 2)
	{
		dst_ch_layout = AV_CH_LAYOUT_STEREO;
	}
	else
	{
		//可扩展
	}

	if (src_ch_layout <= 0)
	{
		printf("src_ch_layout error \n");
		return -1;
	}

	src_nb_samples = pAudioDecodeFrame->nb_samples;
	if (src_nb_samples <= 0)
	{
		printf("src_nb_samples error \n");
		return -1;
	}

	/* set options */
	av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate",       audio_dec_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_dec_ctx->sample_fmt, 0);

	av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate",       out_sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", (AVSampleFormat)out_sample_fmt, 0);
	swr_init(swr_ctx); 

	max_dst_nb_samples = dst_nb_samples =
		av_rescale_rnd(src_nb_samples, out_sample_rate, audio_dec_ctx->sample_rate, AV_ROUND_UP);
	if (max_dst_nb_samples <= 0)
	{
		printf("av_rescale_rnd error \n");
		return -1;
	}

	dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
	ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,
		dst_nb_samples, (AVSampleFormat)out_sample_fmt, 0);
	if (ret < 0)
	{
		printf("av_samples_alloc_array_and_samples error \n");
		return -1;
	}


	dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_dec_ctx->sample_rate) +
		src_nb_samples, out_sample_rate, audio_dec_ctx->sample_rate,AV_ROUND_UP);
	if (dst_nb_samples <= 0)
	{
		printf("av_rescale_rnd error \n");
		return -1;
	}
	if (dst_nb_samples > max_dst_nb_samples)
	{
		av_free(dst_data[0]);
		ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
			dst_nb_samples, (AVSampleFormat)out_sample_fmt, 1);
		max_dst_nb_samples = dst_nb_samples;
	}

	data_size = av_samples_get_buffer_size(NULL, audio_dec_ctx->channels,
		pAudioDecodeFrame->nb_samples,
		audio_dec_ctx->sample_fmt, 1);
	if (data_size <= 0)
	{
		printf("av_samples_get_buffer_size error \n");
		return -1;
	}
	resampled_data_size = data_size;

	if (swr_ctx)
	{
		ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, 
			(const uint8_t **)pAudioDecodeFrame->data, pAudioDecodeFrame->nb_samples);
		if (ret <= 0)
		{
			printf("swr_convert error \n");
			return -1;
		}

		resampled_data_size = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
			ret, (AVSampleFormat)out_sample_fmt, 1);
		if (resampled_data_size <= 0)
		{
			printf("av_samples_get_buffer_size error \n");
			return -1;
		}
	}
	else 
	{
		printf("swr_ctx null error \n");
		return -1;
	}

	//将值返回去
	memcpy(out_buf,dst_data[0],resampled_data_size);

	if (dst_data)
	{
		av_freep(&dst_data[0]);
	}
	av_freep(&dst_data);
	dst_data = NULL;

	if (swr_ctx)
	{
		swr_free(&swr_ctx);
	}
	return resampled_data_size;
}

BOOL WaitThreadExit(HANDLE hThread, DWORD dwTimeout)
{
	BOOL bResult = FALSE;
	DWORD dwStatus;
	int iCount = dwTimeout / 10;
	for (int i = 0; i < iCount; i++)
	{
		::GetExitCodeThread(hThread, &dwStatus);
		if (dwStatus != STILL_ACTIVE)
		{
			bResult = TRUE;
			break;
		}
		Sleep(10);
	}

	if (!bResult)
	{
		TerminateThread(hThread, 0xff);
		TRACE("Terminate thread(%x)\n", hThread);
	}
	//  ::CloseHandle(hThread);

	return bResult;
}

// CYanPlayerDlg dialog
int getSize(int stride, int height, int type) // 0=y, 1=uv
{
	int h = 0, s = 0, size = 0;

	s = type==0 ? 0 : 1;
	h = (height + (1 << s) - 1) >> s;
	size = h * stride;

	return size;
}



CYanPlayerDlg::CYanPlayerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CYanPlayerDlg::IDD, pParent)
	, m_strFilePath(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_dwLastTime = 0;
	m_dwNowTime = 0;
	m_LastPts = 0;
	m_NowPts = 0;
	m_bFrameFirst = TRUE;
	m_pD3D = NULL;
	m_pd3dDevice = NULL;
	m_pd3dSurface = NULL;
	m_hthreadCodecVideo = INVALID_HANDLE_VALUE;
	m_hthreadCodec = INVALID_HANDLE_VALUE;
	m_hthreadCodecAudio = INVALID_HANDLE_VALUE;
	m_hThreadCodecYUV = INVALID_HANDLE_VALUE;
	m_hThreadCodecPCM = INVALID_HANDLE_VALUE;
	m_bExit = FALSE;
}

void CYanPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_FILE_PATH, m_strFilePath);
}

BEGIN_MESSAGE_MAP(CYanPlayerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SELFILE, &CYanPlayerDlg::OnBnClickedButtonSelfile)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON3, &CYanPlayerDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CYanPlayerDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON_DECODEVIDEO, &CYanPlayerDlg::OnBnClickedButtonDecodevideo)
	ON_BN_CLICKED(IDC_BUTTON_DECODEAUDIO, &CYanPlayerDlg::OnBnClickedButtonDecodeaudio)
	ON_BN_CLICKED(IDC_BUTTON_SEL, &CYanPlayerDlg::OnBnClickedButtonSel)
END_MESSAGE_MAP()


// CYanPlayerDlg message handlers

BOOL CYanPlayerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	av_register_all();

	InitD3D();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CYanPlayerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CYanPlayerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CYanPlayerDlg::OnBnClickedButtonSelfile()
{
		UINT uId;
		m_hthreadCodec = (HANDLE)_beginthreadex(NULL, 0, ThreadCodec, (LPVOID)this, 0, &uId);
}

void CYanPlayerDlg::SaveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
	FILE *pFile;
	char szFileName[32];

	sprintf(szFileName, "frame%d.ppm", iFrame);
	pFile = fopen(szFileName, "wb");
	if (pFile == NULL)
	{
		return;
	}

	fprintf(pFile, "P6\n%d %d\n255\n",width, height);

	for (int i = 0; i < height; i++)
	{
		fwrite(pFrame->data[0] + i*pFrame->linesize[0], 1, width*3, pFile);
	}

	fclose(pFile);
	
}

int CYanPlayerDlg::avcodec_decode_video(AVCodecContext *avctx, AVFrame *picture, int *got_picture_ptr, uint8_t *buf, int buf_size)
{
	AVPacket avpkt;
	av_init_packet(&avpkt);
	avpkt.data = buf;
	avpkt.size = buf_size;
	// HACK for CorePNG to decode as normal PNG by default
	avpkt.flags = AV_PKT_FLAG_KEY;
	return avcodec_decode_video2(avctx, picture, got_picture_ptr, &avpkt);
}

UINT CALLBACK CYanPlayerDlg::ThreadCodecVideo(LPVOID lp)
{
	CYanPlayerDlg *pYanPlayDlg = (CYanPlayerDlg *)lp;


	char szFileName[255] = "";
	CString strFileName = pYanPlayDlg->m_strFilePath;
	strncpy(szFileName, strFileName.GetBuffer(strFileName.GetLength()), strFileName.GetLength());
	szFileName[strFileName.GetLength()] = '\0';
	strFileName.ReleaseBuffer();

	pYanPlayDlg->CodecVideo(szFileName);

	return 0;
}

UINT CALLBACK CYanPlayerDlg::ThreadCodecAudio(LPVOID lp)
{
	CYanPlayerDlg *pYanPlayDlg = (CYanPlayerDlg *)lp;

	char szFileName[255] = "";
	CString strFileName = pYanPlayDlg->m_strFilePath;
	strncpy(szFileName, strFileName.GetBuffer(strFileName.GetLength()), strFileName.GetLength());
	szFileName[strFileName.GetLength()] = '\0';
	strFileName.ReleaseBuffer();

	pYanPlayDlg->CodecAudio(szFileName);

	return 0;
}

void CYanPlayerDlg::CodecAudio(char *pFileName)
{
	if (pFileName == NULL)
	{
		OutputDebugString("解码文件名称为空\n");
		return;
	}

	AVFormatContext *pFormatCtx = avformat_alloc_context();

	int iResult = avformat_open_input(&pFormatCtx, pFileName, NULL, NULL);
	if (iResult < 0)
	{
		OutputDebugString(_T("打开视频文件失败！\n"));
	}

	iResult = av_find_stream_info(pFormatCtx);
	if (iResult < 0)
	{
		OutputDebugString(_T("查找流信息失败!\n"));
	}


	AVCodecContext *pCodecCtx = NULL;

	int iAudioStream = -1;
	for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			iAudioStream = i;
			break;
		}
	}

	if (iAudioStream == -1)
	{
		OutputDebugString("input file has no audio stream\n");
		return;
	}

	pCodecCtx = pFormatCtx->streams[iAudioStream]->codec;//音频的编码上下文

	AVCodec *pCodecAudio = NULL;
	pCodecAudio = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodecAudio == NULL)
	{
		OutputDebugString("error no Codec found\n");
	}

	if (avcodec_open2(pCodecCtx, pCodecAudio, NULL) < 0)
	{
		OutputDebugString("打开音频解码器失败!\n");
		return;
	}

	AVPacket packet;
	av_init_packet(&packet);

	AVFrame *pFrame = NULL;
	pFrame = avcodec_alloc_frame();
	
	CFile File;
	File.Open("D:\\1.pcm", CFile::modeCreate|CFile::modeWrite|CFile::typeBinary);

	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		if (packet.stream_index == iAudioStream)
		{
			int nFrameSize = 0;
			avcodec_decode_audio4(pCodecCtx, pFrame, &nFrameSize, &packet);
			if (nFrameSize)
			{

				//pFrame->format;AVSampleFormat

				int plane_size = 0;
				int planar = av_sample_fmt_is_planar(pCodecCtx->sample_fmt);
				int nBytes = av_samples_get_buffer_size(&plane_size,pCodecCtx->channels,pFrame->nb_samples,pCodecCtx->sample_fmt, 1); 
				int nBytesS16 = av_samples_get_buffer_size(&plane_size,pCodecCtx->channels,pFrame->nb_samples,AV_SAMPLE_FMT_S16, 1); 
				
				uint8_t* outputBuffer = new uint8_t[nBytesS16];
				AudioResampling(pCodecCtx, pFrame, AV_SAMPLE_FMT_S16, pCodecCtx->channels, pFrame->sample_rate, outputBuffer);

				//DSOUND 播放PCM
				if (!m_SoundPlayer.IsPlayed())
				{
					m_SoundPlayer.Open(this->GetSafeHwnd(), pCodecCtx->channels, 16, pFrame->sample_rate);
					m_SoundPlayer.Start(outputBuffer, nBytesS16);
				}
				else
				{
					m_SoundPlayer.CopyDataToBuf(outputBuffer, nBytesS16);
				}

				File.Write(outputBuffer, nBytesS16);

// 				CString strLog;
// 				strLog.Format("nFrameSize = %d ByteNum = %d plane_size = %d\n",nFrameSize, nBytes, plane_size);
// 				OutputDebugString(strLog);

				delete []outputBuffer;
				outputBuffer = NULL;

			}
		}
	}

	m_SoundPlayer.Close();
	File.Close();
	
}

void CYanPlayerDlg::CodecVideo(char *pFileName)
{
	if (pFileName == NULL)
	{
		OutputDebugString("解码文件名称为空!\n");
		return;
	}

	AVFormatContext *pFormatCtx = avformat_alloc_context();

	int iResult = avformat_open_input(&pFormatCtx, pFileName, NULL, NULL);
	if (iResult < 0)
	{
		OutputDebugString(_T("打开视频文件失败！"));
	}


	iResult = av_find_stream_info(pFormatCtx);
	if (iResult < 0)
	{
		OutputDebugString(_T("查找流信息失败!"));
	}

	AVCodecContext *pCodecCtx = NULL;
	AVCodecContext *pCodecCtxAudio = NULL;

	int iVideoStream = -1, iAudioStream = -1;
	for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			iVideoStream = i;
			break;
		}
	}

	for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			iAudioStream = i;
			break;
		}
	}

	AVRational FrameRate = pFormatCtx->streams[iVideoStream]->r_frame_rate;
	int nFrameRate = FrameRate.num / FrameRate.den;//帧率
	int nFrameTime = 1000 / nFrameRate;//帧间隔(ms)


	//视频时长

	int idurationtime = pFormatCtx->streams[iVideoStream]->duration * pFormatCtx->streams[iVideoStream]->time_base.num / pFormatCtx->streams[iVideoStream]->time_base.den;

	if (iVideoStream == -1)
	{
		OutputDebugString(_T("没有找到视频流！"));
	}
	if (iAudioStream == -1)
	{
		OutputDebugString(_T("没有找到音频流！"));
	}

	pCodecCtx = pFormatCtx->streams[iVideoStream]->codec;
	pCodecCtxAudio = pFormatCtx->streams[iAudioStream]->codec;

	//Find decoder for the video stream 
	AVCodec *pCodec = NULL, *pCodecAudio = NULL;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		OutputDebugString(_T("视频流不支持解码!"));
	}

	pCodecAudio = avcodec_find_decoder(pCodecCtxAudio->codec_id);
	if (pCodecAudio == NULL)
	{
		OutputDebugString(_T("音频不支持解码!"));
	}

	iResult = avcodec_open2(pCodecCtx, pCodec, NULL);
	if (iResult < 0)
	{
		OutputDebugString(_T("打开解码器失败！"));
	}

	iResult = avcodec_open2(pCodecCtxAudio, pCodecAudio, NULL);
	if (iResult < 0)
	{
		OutputDebugString(_T("打开解码器失败!"));
		return;
	}

	AVFrame *pFrame = NULL;
	AVFrame *pFrameRGB = NULL;
	pFrame = avcodec_alloc_frame();
	pFrameRGB = avcodec_alloc_frame();
	if (pFrameRGB == NULL)
	{
		return;
	}

	uint8_t *buffer = NULL;
	int numBytes;
	numBytes = avpicture_get_size(PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
	buffer = (uint8_t *)av_mallocz_array(sizeof(uint8_t), numBytes);

	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

	int frameFinished;
	AVPacket packet;
	av_init_packet(&packet);

	int j = 0; 
	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		if (m_bExit)
		{
			break;
		}
		if (packet.stream_index==iVideoStream)
		{
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, packet.data, packet.size);
			int pixFmt = pCodecCtx->pix_fmt;
			if (frameFinished) 
			{
				//struct SwsContext *My_img_convert_ctx;
				//int sws_flags = SWS_BICUBIC;
				//My_img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_BGR24, sws_flags, NULL, NULL, NULL);
				//if (My_img_convert_ctx == NULL)
				//{
				//	return;
				//}
				//int iRet = sws_scale(My_img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

				//ShowFrame((char *)pFrameRGB->data[0], pCodecCtx->width, pCodecCtx->height);
				ShowFrame(pFrame, pCodecCtx->width, pCodecCtx->height, &packet, nFrameTime);
				//SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, j);
			}
		}
		else if (packet.stream_index == iAudioStream)
		{
			int nFrameSize = 0;
			avcodec_decode_audio4(pCodecCtxAudio, pFrame, &nFrameSize, &packet);
			if (nFrameSize)
			{

				//pFrame->format;AVSampleFormat

				int plane_size = 0;
				int planar = av_sample_fmt_is_planar(pCodecCtxAudio->sample_fmt);
				int nBytes = av_samples_get_buffer_size(&plane_size,pCodecCtxAudio->channels,pFrame->nb_samples,pCodecCtxAudio->sample_fmt, 1); 
				int nBytesS16 = av_samples_get_buffer_size(&plane_size,pCodecCtxAudio->channels,pFrame->nb_samples,AV_SAMPLE_FMT_S16, 1); 

				uint8_t* outputBuffer = new uint8_t[nBytesS16];
				AudioResampling(pCodecCtxAudio, pFrame, AV_SAMPLE_FMT_S16, pCodecCtxAudio->channels, pFrame->sample_rate, outputBuffer);

				//DSOUND 播放PCM
				if (!m_SoundPlayer.IsPlayed())
				{
					m_SoundPlayer.Open(this->GetSafeHwnd(), pCodecCtxAudio->channels, 16, pFrame->sample_rate);
					m_SoundPlayer.Start(outputBuffer, nBytesS16);
				}
				else
				{
					m_SoundPlayer.CopyDataToBuf(outputBuffer, nBytesS16);
				}

				delete []outputBuffer;
				outputBuffer = NULL;
			}
		}
	}

	m_SoundPlayer.Close();

	m_bFrameFirst = TRUE;
	av_free_packet(&packet);
	av_free(buffer);
	av_free(pFrameRGB);
	av_free(pFrame);
	avcodec_close(pCodecCtx);
	av_close_input_file(pFormatCtx);
}


void CYanPlayerDlg::CodecFile()
{
	UINT uID1;
	m_hthreadCodecVideo =  (HANDLE)_beginthreadex(NULL, 0, ThreadCodecVideo, (LPVOID)this, 0, &uID1);
	//m_hthreadCodecAudio = (HANDLE)_beginthreadex(NULL, 0, ThreadCodecAudio, (LPVOID)this, 0, &UID2);
}

void CYanPlayerDlg::ShowFrame(AVFrame *pFrame, int iWidth, int iHeight, AVPacket *pPacket, int nFrameTime)
{
	SetVideoSize(iWidth, iHeight);

	if (m_pd3dSurface != NULL)
	{
		m_pd3dSurface->Release();
		m_pd3dSurface = NULL;
	}
	int iRet = m_pd3dDevice->CreateOffscreenPlainSurface(m_nWidth, m_nHeight ,(D3DFORMAT)MAKEFOURCC('Y','V','1','2'), D3DPOOL_DEFAULT, &m_pd3dSurface, NULL);

	long size_y = 0, size_uv = 0, size = 0;
	size_y = getSize(iWidth, iHeight, 0);
	size_uv = getSize(iWidth/2, iHeight, 1);

	size = size_y + size_uv * 2;

	BYTE *pBuf = new BYTE[size];
	int nDataLen = 0;
	for (int i = 0; i < 3; i++)
	{
		int nShift = (i == 0) ? 0 : 1;
		PBYTE pYUVData = (PBYTE)pFrame->data[i];
		for (int j = 0; j < iHeight >> nShift; j++)
		{
			memcpy(pBuf + nDataLen, pYUVData, iWidth >> nShift);
			pYUVData += pFrame->linesize[i];
			nDataLen += (pFrame->width >> nShift);
		}
	}

	SetYUVbuffer((char *)pBuf, size);
	
	delete []pBuf;
	pBuf = NULL;


	m_dwNowTime = timeGetTime();
	m_NowPts = pPacket->pts;
	int nTime = m_dwNowTime - m_dwLastTime;
	int nSleepTime = nFrameTime - nTime;
	CString strLog;
	strLog.Format("nSleepTime = %d m_NowPts = %ld m_LastPts =%u m_dwNowTime = %d m_dwLastTime = %d FrameTime = %d\n",nSleepTime,  m_NowPts, m_LastPts, m_dwNowTime, m_dwLastTime , nFrameTime);
	OutputDebugString(strLog);

	if (!m_bFrameFirst)
	{	
		if (nSleepTime < 0)
		{
			Sleep(40);
		}
		else
		{
			Sleep(nFrameTime);
		}
		
	}

	m_LastPts = m_NowPts;
	m_bFrameFirst = FALSE;
	m_dwLastTime = timeGetTime();


}

void CYanPlayerDlg::ShowFrame(char *pbuf, int iWidth, int iHeight)
{

	//SaveBitmap((BYTE *)pbuf, iWidth, iHeight);
	CRect rc;
	GetDlgItem(IDC_VEDIO_SHOW)->GetWindowRect(&rc);
	ScreenToClient(&rc);

	BITMAPINFO *bmphdr;
	DWORD dwBmpHdr = sizeof(BITMAPINFO);
	bmphdr = new BITMAPINFO[dwBmpHdr];
	bmphdr->bmiHeader.biBitCount = 24;
	bmphdr->bmiHeader.biClrImportant = 0;
	bmphdr->bmiHeader.biSize = dwBmpHdr;
	bmphdr->bmiHeader.biSizeImage = 0;
	bmphdr->bmiHeader.biWidth = iWidth;
	bmphdr->bmiHeader.biHeight = -iHeight;
	bmphdr->bmiHeader.biXPelsPerMeter = 0;
	bmphdr->bmiHeader.biYPelsPerMeter = 0;
	bmphdr->bmiHeader.biClrUsed = 0;
	bmphdr->bmiHeader.biPlanes = 1;
	bmphdr->bmiHeader.biCompression = BI_RGB;
	

//STRETCH_ANDSCANS
	int iRet = SetStretchBltMode(GetDC()->m_hDC,HALFTONE); 
	int nResult = StretchDIBits(GetDC()->m_hDC,
		rc.left,
		rc.top,
		rc.Width(),//rc.right - rc.left,
		rc.Height(),//rc.top,
		0, 0,
		iWidth, iHeight,
		pbuf,
		bmphdr,
		DIB_RGB_COLORS,
		SRCCOPY);

	delete []bmphdr;
	bmphdr = NULL;

}

UINT CALLBACK CYanPlayerDlg::ThreadCodec(LPVOID lp)
{

	CYanPlayerDlg *pYanPlayerDlg = (CYanPlayerDlg *)lp;
	CRect rc;
	pYanPlayerDlg->GetDlgItem(IDC_VEDIO_SHOW)->GetWindowRect(&rc);
	pYanPlayerDlg->ScreenToClient(&rc);

	pYanPlayerDlg->CodecFile();

	return 0;
}

BOOL CYanPlayerDlg::SaveBitmap(BYTE *pBuffer, int iWidth, int iHeight)
{
	int iBufLen = iWidth * iHeight * 3;
	char *pBmpBuffer = new char[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + iBufLen];
	
	HANDLE hf = CreateFile("D:\\1.bmp", GENERIC_WRITE, 
		FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hf == INVALID_HANDLE_VALUE) return 0;
	
	// 写文件头
	BITMAPFILEHEADER fileheader;
	ZeroMemory(&fileheader, sizeof(BITMAPFILEHEADER));
	fileheader.bfType = 'MB';
	fileheader.bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+iWidth*iHeight*3;
	fileheader.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	DWORD dwWritter = 0;
	WriteFile(hf, &fileheader, sizeof(BITMAPFILEHEADER), &dwWritter, NULL);
	memcpy_s(pBmpBuffer, sizeof(BITMAPFILEHEADER), &fileheader, sizeof(BITMAPFILEHEADER));

	// 写文图格式
	BITMAPINFOHEADER infoHeader;
	ZeroMemory(&infoHeader, sizeof(BITMAPINFOHEADER));
	infoHeader.biSize = sizeof(BITMAPINFOHEADER);
	//infoHeader.biSizeImage = lBufferLen;
	infoHeader.biWidth = iWidth;
	infoHeader.biHeight = -iHeight;
	infoHeader.biBitCount = 24;
	WriteFile(hf, &infoHeader, sizeof(BITMAPINFOHEADER), &dwWritter, NULL);
	memcpy_s(pBmpBuffer + sizeof(BITMAPFILEHEADER), sizeof(BITMAPINFOHEADER), &infoHeader, sizeof(BITMAPINFOHEADER));
	
	// 写位图数据
	WriteFile(hf, pBuffer, iWidth*iHeight*3, &dwWritter, NULL);
	memcpy_s(pBmpBuffer + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), iBufLen, pBuffer, iBufLen);

	HBITMAP hBitmap = NULL;
	SetBitmapBits(hBitmap, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + iBufLen, pBmpBuffer);
	delete []pBmpBuffer;
	pBmpBuffer = NULL;

	CloseHandle(hf);
	return 0;
}

void CYanPlayerDlg::SetVideoSize(long lWidth, long lHeight)
{
	m_nWidth = lWidth;
	m_nHeight = lHeight;
}

void CYanPlayerDlg::InitD3D()
{
	if (m_pD3D != NULL)
	{
		m_pD3D->Release();
		m_pD3D = NULL;
	}

	if (m_pd3dDevice != NULL)
	{
		m_pd3dDevice->Release();
		m_pd3dDevice = NULL;
	}

	if (m_pd3dSurface != NULL)
	{
		m_pd3dSurface->Release();
		m_pd3dSurface = NULL;
	}

	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (m_pD3D == NULL)
	{
		return;
	}

	D3DPRESENT_PARAMETERS d3dpp; 
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;


	int iRet  = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDlgItem(IDC_VEDIO_SHOW)->GetSafeHwnd(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &m_pd3dDevice);
	
	iRet = m_pd3dDevice->CreateOffscreenPlainSurface(m_nWidth, m_nHeight ,(D3DFORMAT)MAKEFOURCC('Y','V','1','2'), D3DPOOL_DEFAULT, &m_pd3dSurface, NULL);

}
void CYanPlayerDlg::SetYUVbuffer(char *pBuffer,long lLen)
{
	if(m_pd3dSurface == NULL)return;
	D3DLOCKED_RECT d3d_rect;
	if( FAILED(m_pd3dSurface->LockRect(&d3d_rect,NULL,D3DLOCK_DONOTWAIT)))
		return ;

	const int w = m_nWidth,h = m_nHeight;
	BYTE * const p = (BYTE *)d3d_rect.pBits;
	const int stride = d3d_rect.Pitch;
	int i = 0;
	for(i = 0;i < h;i ++)
	{
		memcpy(p + i * stride,pBuffer + i * w, w);
	}
	for(i = 0;i < h/2;i ++)
	{
		memcpy(p + stride * h + i * stride / 2,pBuffer + w * h + w * h / 4 + i * w / 2, w / 2);
	}
	for(i = 0;i < h/2;i ++)
	{
		memcpy(p + stride * h + stride * h / 4 + i * stride / 2,pBuffer + w * h + i * w / 2, w / 2);
	}

	if( FAILED(m_pd3dSurface->UnlockRect()))
	{
		return ;
	}

	DrawImage();
}

void CYanPlayerDlg::DrawImage()
{
	if (m_pd3dDevice != NULL)
	{
		double dbAspect = (double)m_nWidth / m_nHeight;
		CRect rtClient;
		GetDlgItem(IDC_VEDIO_SHOW)->GetClientRect(&rtClient);
		m_rtViewport  = rtClient;

		if(rtClient.Width() > rtClient.Height() * dbAspect)
		{
			//width lager than height,adjust the width
			int nValidW(static_cast<int>(rtClient.Height() * dbAspect));
			int nLost(rtClient.Width() - nValidW);
			m_rtViewport.left += nLost / 2;
			m_rtViewport.right = m_rtViewport.left + nValidW;
		}
		else
		{
			//height lager than width,adjust the height
			int nValidH(static_cast<int>(rtClient.Width() / dbAspect));
			int nLost(rtClient.Height() - nValidH);
			m_rtViewport.top += nLost / 2;
			m_rtViewport.bottom = m_rtViewport.top + nValidH;
		}

		m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 );
		m_pd3dDevice->BeginScene();
		IDirect3DSurface9 * pBackBuffer = NULL;

		m_pd3dDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer);
		m_pd3dDevice->StretchRect(m_pd3dSurface,NULL,pBackBuffer,&m_rtViewport,D3DTEXF_LINEAR);
		m_pd3dDevice->EndScene();
		m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
		pBackBuffer->Release();
	}
}
void CYanPlayerDlg::OnClose()
{

	m_bExit = TRUE;
	DWORD dwTimeOunt = 3000;
	WaitThreadExit(m_hthreadCodecVideo, dwTimeOunt);
	//WaitThreadExit(m_hthreadCodecAudio, dwTimeOunt);
	WaitThreadExit(m_hthreadCodec, dwTimeOunt);
	WaitThreadExit(m_hThreadCodecPCM, dwTimeOunt);
	WaitThreadExit(m_hThreadCodecYUV, dwTimeOunt);

	CloseHandle(m_hthreadCodec);
	//CloseHandle(m_hthreadCodecAudio);
	CloseHandle(m_hthreadCodecVideo);
	CloseHandle(m_hThreadCodecPCM);
	CloseHandle(m_hThreadCodecYUV);

	m_hthreadCodec = INVALID_HANDLE_VALUE;
	m_hthreadCodecAudio = INVALID_HANDLE_VALUE;
	m_hthreadCodecVideo = INVALID_HANDLE_VALUE;
	m_hThreadCodecYUV = INVALID_HANDLE_VALUE;
	m_hThreadCodecPCM = INVALID_HANDLE_VALUE;


	CDialog::OnClose();
}


void CYanPlayerDlg::OnBnClickedButton3()
{
	CFileDialog FileDlg(FALSE, "*.mp4", "*.mp4", OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, "*.mp4|*.*", this);
	if (FileDlg.DoModal() == IDOK)
	{
		m_strDstFilePath = FileDlg.GetPathName();

		UINT uiD = 1;
		m_hThreadEncodeYUV = (HANDLE)_beginthreadex(NULL, 0, ThreadEncodeYUV, (LPVOID)this, 0, &uiD);
	}

}

UINT CALLBACK CYanPlayerDlg::ThreadEncodeYUV(LPVOID lp)
{
	CYanPlayerDlg *pDlg = (CYanPlayerDlg *)lp;
	pDlg->EncodeYUV(pDlg->m_strFilePath, pDlg->m_strDstFilePath);

	return 0;
}

void CYanPlayerDlg::EncodeYUV(CString strSrcFile, CString strDstFile)
{
	GetDlgItem(IDC_STATIC_STATE)->SetWindowText("正在解码YUV数据.......");
	char pFileName[128];
	ZeroMemory(pFileName, 128);
	strncpy(pFileName, strSrcFile.GetBuffer(0), strSrcFile.GetLength());
	pFileName[strSrcFile.GetLength()] = '\0';

	if (pFileName == NULL)
	{
		OutputDebugString("解码文件名称为空!\n");
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	AVFormatContext *pFormatCtx = avformat_alloc_context();

	int iResult = avformat_open_input(&pFormatCtx, pFileName, NULL, NULL);
	if (iResult < 0)
	{
		OutputDebugString(_T("打开视频文件失败！\n"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	iResult = av_find_stream_info(pFormatCtx);
	if (iResult < 0)
	{
		OutputDebugString(_T("查找流信息失败!\n"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	AVCodecContext *pCodecCtx = NULL;

	int iVideoStream = -1;
	for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			iVideoStream = i;
			break;
		}
	}


	AVRational FrameRate = pFormatCtx->streams[iVideoStream]->r_frame_rate;
	int nFrameRate = FrameRate.num / FrameRate.den;//帧率
	int nFrameTime = 1000 / nFrameRate;//帧间隔(ms)

	if (iVideoStream == -1)
	{
		OutputDebugString(_T("没有找到视频流！\n"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	pCodecCtx = pFormatCtx->streams[iVideoStream]->codec;

	//Find decoder for the video stream 
	AVCodec *pCodec = NULL, *pCodecAudio = NULL;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		OutputDebugString(_T("视频流不支持解码!\n"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	iResult = avcodec_open2(pCodecCtx, pCodec, NULL);
	if (iResult < 0)
	{
		OutputDebugString(_T("打开解码器失败！\n"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	AVFrame *pFrame = NULL;
	pFrame = avcodec_alloc_frame();

	int frameFinished;
	AVPacket packet;
	av_init_packet(&packet);

	//编码
	AVCodec *OutCodec;
	AVCodecContext *outCodecCtx = NULL;
	int i = 0, ret, got_output;
	AVPacket outpkt;
	uint8_t endcode[] = {0, 0, 1, 0xb7};


	AVOutputFormat *fmt;
	fmt = av_guess_format(NULL, strDstFile, NULL);
	
	OutCodec = avcodec_find_encoder(fmt->video_codec);
	if (!OutCodec)
	{
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}
	
	outCodecCtx = avcodec_alloc_context3(OutCodec);
	if (!outCodecCtx) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	outCodecCtx->bit_rate = pCodecCtx->bit_rate;
	outCodecCtx->width = pCodecCtx->width;
	outCodecCtx->height = pCodecCtx->height;
	outCodecCtx->time_base.num = 1;
	outCodecCtx->time_base.den = 30;
	outCodecCtx->gop_size = pCodecCtx->gop_size;
	outCodecCtx->max_b_frames = pCodecCtx->max_b_frames;
	outCodecCtx->pix_fmt = pCodecCtx->pix_fmt;

	if (pCodecCtx->codec_id == AV_CODEC_ID_H264)
		av_opt_set(outCodecCtx->priv_data, "preset", "slow", 0);

	/* open it */
	if (avcodec_open2(outCodecCtx, OutCodec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	int j = 0; 
	CFile File;
	File.Open(strDstFile, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary);
	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		if (m_bExit)
		{
			break;
		}
		if (packet.stream_index==iVideoStream)
		{
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, packet.data, packet.size);
			int pixFmt = pCodecCtx->pix_fmt;
			if (frameFinished) 
			{
				av_init_packet(&outpkt);
				outpkt.data = NULL;
				outpkt.size = 0;

				ret = avcodec_encode_video2(outCodecCtx, &outpkt, pFrame, &got_output);
				if (ret < 0)
				{
					fprintf(stderr, "Error encoding frame\n");
					exit(1);
				}

				if (got_output) {
					printf("Write frame %3d (size=%5d)\n", i, outpkt.size);
					File.Write(outpkt.data , outpkt.size);
					File.Flush();
					av_free_packet(&outpkt);
				}
				i++;

				//long size_y = 0, size_uv = 0, size = 0;
				//size_y = getSize(pCodecCtx->width, pCodecCtx->height, 0);
				//size_uv = getSize(pCodecCtx->width/2, pCodecCtx->height, 1);

				//size = size_y + size_uv * 2;

				//BYTE *pBuf = new BYTE[size];
				//int nDataLen = 0;
				//for (int i = 0; i < 3; i++)
				//{
				//	int nShift = (i == 0) ? 0 : 1;
				//	PBYTE pYUVData = (PBYTE)pFrame->data[i];
				//	for (int j = 0; j < pCodecCtx->height >> nShift; j++)
				//	{
				//		memcpy(pBuf + nDataLen, pYUVData, pCodecCtx->width >> nShift);
				//		pYUVData += pFrame->linesize[i];
				//		nDataLen += (pFrame->width >> nShift);
				//	}
				//}

				//delete []pBuf;
				//pBuf = NULL;
				//ShowFrame(pFrame, pCodecCtx->width, pCodecCtx->height, &packet, nFrameTime);
				//SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, j);
			}
		}
	}

	/* get the delayed frames */
	for (got_output = 1; got_output; i++) {

		ret = avcodec_encode_video2(outCodecCtx, &outpkt, NULL, &got_output);
		if (ret < 0) {
			fprintf(stderr, "Error encoding frame\n");
			exit(1);
		}

		if (got_output) {
			printf("Write frame %3d (size=%5d)\n", i, outpkt.size);
			File.Write(outpkt.data , outpkt.size);
			av_free_packet(&outpkt);
		}
	}

	File.Write(endcode, sizeof(endcode));

	GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码完毕");
	File.Close();
	av_free_packet(&packet);
	av_free(pFrame);
	avcodec_close(pCodecCtx);
	av_close_input_file(pFormatCtx);
}

void CYanPlayerDlg::OnBnClickedButton4()
{

}

void CYanPlayerDlg::OnBnClickedButtonDecodevideo()
{
	CFileDialog FileDlg(FALSE, "*.yuv", "*.yuv", OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, "*.yuv|*.*", this);
	if (FileDlg.DoModal() == IDOK)
	{
		m_strDstFilePath = FileDlg.GetPathName();
		UINT uID = 1;
		m_hThreadCodecYUV = (HANDLE)_beginthreadex(NULL, 0, ThreadCodecYUV, (LPVOID)this, 0,&uID);
	}
}

UINT CALLBACK CYanPlayerDlg::ThreadCodecYUV(LPVOID lp)
{
	CYanPlayerDlg *pDlg = (CYanPlayerDlg *)lp;
	pDlg->CodecYuvFile(pDlg->m_strFilePath, pDlg->m_strDstFilePath);

	return 0;
}

void CYanPlayerDlg::CodecYuvFile(CString strSrcFile, CString strDstFile)
{
	GetDlgItem(IDC_STATIC_STATE)->SetWindowText("正在解码YUV数据.......");
	char pFileName[128];
	ZeroMemory(pFileName, 128);
	strncpy(pFileName, strSrcFile.GetBuffer(0), strSrcFile.GetLength());
	pFileName[strSrcFile.GetLength()] = '\0';

	if (pFileName == NULL)
	{
		OutputDebugString("解码文件名称为空!\n");
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	AVFormatContext *pFormatCtx = avformat_alloc_context();

	int iResult = avformat_open_input(&pFormatCtx, pFileName, NULL, NULL);
	if (iResult < 0)
	{
		OutputDebugString(_T("打开视频文件失败！"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	iResult = av_find_stream_info(pFormatCtx);
	if (iResult < 0)
	{
		OutputDebugString(_T("查找流信息失败!"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	AVCodecContext *pCodecCtx = NULL;

	int iVideoStream = -1;
	for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			iVideoStream = i;
			break;
		}
	}


	AVRational FrameRate = pFormatCtx->streams[iVideoStream]->r_frame_rate;
	int nFrameRate = FrameRate.num / FrameRate.den;//帧率
	int nFrameTime = 1000 / nFrameRate;//帧间隔(ms)

	if (iVideoStream == -1)
	{
		OutputDebugString(_T("没有找到视频流！"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	pCodecCtx = pFormatCtx->streams[iVideoStream]->codec;

	//Find decoder for the video stream 
	AVCodec *pCodec = NULL, *pCodecAudio = NULL;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		OutputDebugString(_T("视频流不支持解码!"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	iResult = avcodec_open2(pCodecCtx, pCodec, NULL);
	if (iResult < 0)
	{
		OutputDebugString(_T("打开解码器失败！"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	AVFrame *pFrame = NULL;
	pFrame = avcodec_alloc_frame();

	int frameFinished;
	AVPacket packet;
	av_init_packet(&packet);

	int j = 0; 
	CFile File;
	File.Open(strDstFile, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary);
	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		if (m_bExit)
		{
			break;
		}
		if (packet.stream_index==iVideoStream)
		{
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, packet.data, packet.size);
			int pixFmt = pCodecCtx->pix_fmt;
			if (frameFinished) 
			{

				long size_y = 0, size_uv = 0, size = 0;
				size_y = getSize(pCodecCtx->width, pCodecCtx->height, 0);
				size_uv = getSize(pCodecCtx->width/2, pCodecCtx->height, 1);

				size = size_y + size_uv * 2;

				BYTE *pBuf = new BYTE[size];
				int nDataLen = 0;
				for (int i = 0; i < 3; i++)
				{
					int nShift = (i == 0) ? 0 : 1;
					PBYTE pYUVData = (PBYTE)pFrame->data[i];
					for (int j = 0; j < pCodecCtx->height >> nShift; j++)
					{
						memcpy(pBuf + nDataLen, pYUVData, pCodecCtx->width >> nShift);
						pYUVData += pFrame->linesize[i];
						nDataLen += (pFrame->width >> nShift);
					}
				}

				File.Write(pBuf, size);

				delete []pBuf;
				pBuf = NULL;
				//ShowFrame(pFrame, pCodecCtx->width, pCodecCtx->height, &packet, nFrameTime);
				//SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, j);
			}
		}
	}

	GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码完毕");
	File.Close();
	av_free_packet(&packet);
	av_free(pFrame);
	avcodec_close(pCodecCtx);
	av_close_input_file(pFormatCtx);
}

void CYanPlayerDlg::CodecPCMFile(CString strSrcFile, CString strDstFile)
{
	char pFileName[128];
	ZeroMemory(pFileName, 128);
	strncpy(pFileName, strSrcFile.GetBuffer(0),strSrcFile.GetLength());
	pFileName[strSrcFile.GetLength()] = '\0';

	GetDlgItem(IDC_STATIC_STATE)->SetWindowText("正在解码PCM音频数据.....");

	if (pFileName == NULL)
	{
		OutputDebugString("解码文件名称为空\n");
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	AVFormatContext *pFormatCtx = avformat_alloc_context();

	int iResult = avformat_open_input(&pFormatCtx, pFileName, NULL, NULL);
	if (iResult < 0)
	{
		OutputDebugString(_T("打开视频文件失败！\n"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	iResult = av_find_stream_info(pFormatCtx);
	if (iResult < 0)
	{
		OutputDebugString(_T("查找流信息失败!\n"));
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}


	AVCodecContext *pCodecCtx = NULL;

	int iAudioStream = -1;
	for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			iAudioStream = i;
			break;
		}
	}

	if (iAudioStream == -1)
	{
		OutputDebugString("input file has no audio stream\n");
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	pCodecCtx = pFormatCtx->streams[iAudioStream]->codec;//音频的编码上下文

	AVCodec *pCodecAudio = NULL;
	pCodecAudio = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodecAudio == NULL)
	{
		OutputDebugString("error no Codec found\n");
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	if (avcodec_open2(pCodecCtx, pCodecAudio, NULL) < 0)
	{
		OutputDebugString("打开音频解码器失败!\n");
		GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码失败!");
		return;
	}

	AVPacket packet;
	av_init_packet(&packet);

	AVFrame *pFrame = NULL;
	pFrame = avcodec_alloc_frame();

	CFile File;
	File.Open(strDstFile, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary);

	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		if (packet.stream_index == iAudioStream)
		{
			int nFrameSize = 0;
			avcodec_decode_audio4(pCodecCtx, pFrame, &nFrameSize, &packet);
			if (nFrameSize)
			{

				//pFrame->format;AVSampleFormat

				int plane_size = 0;
				int planar = av_sample_fmt_is_planar(pCodecCtx->sample_fmt);
				int nBytes = av_samples_get_buffer_size(&plane_size,pCodecCtx->channels,pFrame->nb_samples,pCodecCtx->sample_fmt, 1); 
				int nBytesS16 = av_samples_get_buffer_size(&plane_size,pCodecCtx->channels,pFrame->nb_samples,AV_SAMPLE_FMT_S16, 1); 

				uint8_t* outputBuffer = new uint8_t[nBytesS16];
				AudioResampling(pCodecCtx, pFrame, AV_SAMPLE_FMT_S16, pCodecCtx->channels, pFrame->sample_rate, outputBuffer);

				File.Write(outputBuffer, nBytesS16);

				delete []outputBuffer;
				outputBuffer = NULL;

			}
		}
	}
	File.Close();

	GetDlgItem(IDC_STATIC_STATE)->SetWindowText("解码完毕!");
}


void CYanPlayerDlg::OnBnClickedButtonDecodeaudio()
{
	CFileDialog FileDlg(FALSE, "*.pcm", "*.pcm", OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, "*.pcm|*.*");
	if (FileDlg.DoModal() == IDOK)
	{
		m_strDstFilePath = FileDlg.GetPathName();
		UINT uID = 0;
		m_hThreadCodecPCM = (HANDLE)_beginthreadex(NULL, 0, ThreadCodecPCM, (LPVOID)this, 0, &uID);
	}
}

UINT CALLBACK CYanPlayerDlg::ThreadCodecPCM(LPVOID lp)
{
	CYanPlayerDlg *pDlg = (CYanPlayerDlg *)lp;
	pDlg->CodecPCMFile(pDlg->m_strFilePath, pDlg->m_strDstFilePath);

	return 0;
}

void CYanPlayerDlg::OnBnClickedButtonSel()
{
	CString strFilter = _T("RMVB(*.rmvb)|*.rmvb|H264(*.264)|*.264|MP4(*.mp4)|*.mp4||");
	CFileDialog FileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter, this);
	if (FileDlg.DoModal() == IDOK)
	{
		m_strFilePath = FileDlg.GetPathName();
		UpdateData(FALSE);
	}
}
