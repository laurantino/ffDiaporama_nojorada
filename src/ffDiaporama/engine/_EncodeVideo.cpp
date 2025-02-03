// changed by gerd
/* ======================================================================
    This file is part of ffDiaporama
    ffDiaporama is a tool to make diaporama as video
    Copyright (C) 2011-2014 Dominique Levray <domledom@laposte.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
   ====================================================================== */

#pragma warning(default:4996)
#include "_EncodeVideo.h"
#ifdef _MSC_VER
#pragma warning(default: 4996)      /* enable deprecation */
#endif


extern "C"
{
   #include <libavutil/imgutils.h>
}
#include "ffTools.h"

//#define PIXFMT      PIX_FMT_RGB24
//#define QTPIXFMT    QImage::Format_RGB888
#define PIXFMT      AV_PIX_FMT_BGRA
#define QTPIXFMT    QImage::Format_ARGB32
#define PREVIEW_DIFTIME_MS 1000/25
#define PRELOAD_LIMIT 20
#define PRELOAD_ADVANCE 3
static int iWaitPreload = 0;
static int iWaitAssembly = 0;
static int iWaitEncAudio = 0;
static int iWaitEncVideo = 0;
static int maxFramesInList = 0;
int iMinAvailable = 0;
static int emptyFramesInList = 0;
int reusedFrames = 0;
int numYuvFrames = 0;
//*************************************************************************************************************************************************

//int64_t conversionTime;
extern const char *nano2time(LONGLONG ticks, bool withMillis);
extern AVRational MakeAVRational(int num, int den);
int CheckEncoderCapabilities(VFORMAT_ID FormatId,AVCodecID VideoCodec,AVCodecID AudioCodec) 
{
   if (VideoCodec == AV_CODEC_ID_NONE) 
      return SUPPORTED_COMBINATION;

   int Ret = INVALID_COMBINATION;

   switch (FormatId) 
   {
      case VFORMAT_3GP:
         if ((VideoCodec == AV_CODEC_ID_MPEG4 || VideoCodec == AV_CODEC_ID_H264) 
            && (AudioCodec == AV_CODEC_ID_AMR_NB || AudioCodec == AV_CODEC_ID_AMR_WB))
            Ret = SUPPORTED_COMBINATION;
         break;
      case VFORMAT_MJPEG:
         if (VideoCodec == AV_CODEC_ID_MJPEG && AudioCodec == AV_CODEC_ID_PCM_S16LE)
            Ret = SUPPORTED_COMBINATION;
         break;
      case VFORMAT_OGV:
         if (VideoCodec == AV_CODEC_ID_THEORA && AudioCodec == AV_CODEC_ID_VORBIS)
            Ret = SUPPORTED_COMBINATION;
         break;
      case VFORMAT_WEBM:
         if ( (VideoCodec == AV_CODEC_ID_VP8 || VideoCodec == AV_CODEC_ID_VP9 ) && AudioCodec == AV_CODEC_ID_VORBIS)
            Ret = SUPPORTED_COMBINATION;
         break;
      case VFORMAT_OLDFLV:
         if (VideoCodec == AV_CODEC_ID_FLV1 && AudioCodec == AV_CODEC_ID_MP3)
            Ret = SUPPORTED_COMBINATION;
         break;
      case VFORMAT_FLV:
         if (VideoCodec == AV_CODEC_ID_H264 && AudioCodec == AV_CODEC_ID_AAC)
            Ret = SUPPORTED_COMBINATION;
         break;
      case VFORMAT_MPEG:
         if (VideoCodec == AV_CODEC_ID_MPEG2VIDEO && (AudioCodec == AV_CODEC_ID_AC3 || AudioCodec==AV_CODEC_ID_MP2))
            Ret = SUPPORTED_COMBINATION;
         break;
      case VFORMAT_AVI:
         if ( (VideoCodec == AV_CODEC_ID_MPEG4 || VideoCodec == AV_CODEC_ID_H264)
               && (AudioCodec == AV_CODEC_ID_AC3 || AudioCodec == AV_CODEC_ID_MP3 || AudioCodec == AV_CODEC_ID_PCM_S16LE))
            Ret = SUPPORTED_COMBINATION;
         break;
      case VFORMAT_MP4:
         if ((VideoCodec == AV_CODEC_ID_MPEG4 || VideoCodec == AV_CODEC_ID_H264 || VideoCodec == AV_CODEC_ID_H265)
              && (AudioCodec == AV_CODEC_ID_MP3 || AudioCodec == AV_CODEC_ID_AAC))
            Ret = SUPPORTED_COMBINATION;
         break;
      case VFORMAT_MKV:
            Ret = SUPPORTED_COMBINATION;
         break;
      default: break;
   }
   return Ret;
}

//*************************************************************************************************************************************************

cEncodeVideo::cEncodeVideo() 
{
    StopProcessWanted       = false;
    Diaporama               = NULL;
    Container               = NULL;
    IsOpen                  = false;
    enableFaststart = false;
                              
    // Audio buffers          
    AudioResampler          = NULL;
    AudioResamplerBuffer    = NULL;

    //Video buffers           
    VideoEncodeBuffer       = NULL;
    VideoFrameConverter     = NULL;
    VideoYUVConverter       = NULL;
    InternalWidth           = 0;
    InternalHeight          = 0;
    ExtendV                 = 0;
    VideoFrameBufSize       = 0;
    VideoFrameBuf           = NULL;

    iRenderWidth = iRenderHeight = 0;
    bDSR = false;
    showPreview = true;
    renderOnly = false;
    yuvPassThroughSignal = false;
    useCRF = false;
    iCRF = -1;

    #ifdef Q_OS_WIN
	// switch off energy saving during render-process
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
    #endif
}

//*************************************************************************************************************************************************

cEncodeVideo::~cEncodeVideo() 
{
   CloseEncoder();
   deleteList(PreparedTransitBrushList);
   deleteList(PreparedBrushList);
   pAppConfig->ImagesCache.RemoveImageObject(this);
   #ifdef Q_OS_WIN
   // restore prev energy saving 
   SetThreadExecutionState(ES_CONTINUOUS);
   #endif
}

//*************************************************************************************************************************************************

void cEncodeVideo::CloseEncoder() 
{
   if (Container) 
   {
      if (IsOpen) 
      {
         flush_encoders();
         av_write_trailer(Container);
         avio_close(Container->pb);
         
         #if LIBAVCODEC_VERSION_MAJOR < 58
         // close codecs so that they can do their cleanup
         if(ostVideo.stream)
            avcodec_close(ostVideo.cc);
         if(ostAudio.stream)
            avcodec_close(ostAudio.cc);
         #else
         if (ostVideo.stream)
            avcodec_free_context(&ostVideo.cc);
         if (ostAudio.stream)
            avcodec_free_context(&ostAudio.cc);
         #endif
      }
      avformat_free_context(Container);
      Container = NULL;
   }
   ostVideo.stream = NULL;
   ostAudio.stream = NULL;

   // Audio buffers
   if (ostAudio.frame) 
      av_freep(&ostAudio.frame);

   if (AudioResampler) 
   {
      swr_free(&AudioResampler);
      AudioResampler = NULL;
   }
   if (AudioResamplerBuffer) 
   {
      av_free(AudioResamplerBuffer);
      AudioResamplerBuffer = NULL;
   }

   // Video buffers
   if (VideoEncodeBuffer) 
   {
      av_freep(VideoEncodeBuffer);
   }
   if (VideoFrameConverter) 
   {
      sws_freeContext(VideoFrameConverter);
      VideoFrameConverter = NULL;
   }
   if (ostVideo.frame) 
   {
      if (ostVideo.frame->extended_data && ostVideo.frame->extended_data != ostVideo.frame->data)
         av_freep(&ostVideo.frame->extended_data);
      if (ostVideo.frame->data[0])
         av_freep(&ostVideo.frame->data[0]);
      av_freep(&ostVideo.frame);
   }
   #ifdef Q_OS_WIN
   SetThreadExecutionState(ES_CONTINUOUS);
   #endif
}

//*************************************************************************************************************************************************

int cEncodeVideo::getThreadFlags(AVCodecID ID) 
{
   int Ret = 0;
   switch (ID) 
   {
      case AV_CODEC_ID_PRORES:
      case AV_CODEC_ID_MPEG1VIDEO:
      case AV_CODEC_ID_DVVIDEO:
      case AV_CODEC_ID_MPEG2VIDEO:   Ret = FF_THREAD_SLICE;                    break;
      case AV_CODEC_ID_H264 :        Ret = FF_THREAD_FRAME|FF_THREAD_SLICE;    break;
      default:                       Ret = FF_THREAD_FRAME;                    break;
   }
   return Ret;
}

//*************************************************************************************************************************************************

bool cEncodeVideo::OpenEncoder(QWidget *ParentWindow,cDiaporama *Diaporama,QString OutputFileName,int FromSlide,int ToSlide,
                    bool EncodeVideo,int VideoCodecSubId,bool VBR,sIMAGEDEF *ImageDef,int ImageWidth,int ImageHeight,int ExtendV,int InternalWidth,int InternalHeight,AVRational PixelAspectRatio,int VideoBitrate,
                    bool useCRF, int iCRF,
                    bool EncodeAudio,int AudioCodecSubId,int AudioChannels,int AudioBitrate,int AudioSampleRate,QString Language) 
{
   this->Diaporama       = Diaporama;
   this->OutputFileName  = OutputFileName;
   this->FromSlide       = FromSlide;
   this->ToSlide         = ToSlide;
   this->useCRF = useCRF;
   this->iCRF = iCRF;
   QString FMT           = QFileInfo(OutputFileName).suffix();

   if( renderOnly ) // for testing purposes only!!!
   {
      VideoFrameRate.num=1;
      VideoFrameRate.den=25;
      dFPS = qreal(VideoFrameRate.den)/qreal(VideoFrameRate.num);
      NbrFrame = int(qreal(Diaporama->GetPartialDuration(FromSlide,ToSlide))*dFPS/1000);    // Number of frame to generate
      iRenderWidth = InternalWidth;
      iRenderHeight = InternalHeight;

      this->VideoBitrate = VideoBitrate;
      this->ImageDef = ImageDef;
      this->VideoFrameRate = ImageDef->AVFPS;
      this->VideoCodecSubId = VideoCodecSubId;
      this->InternalWidth = InternalWidth;
      this->InternalHeight = InternalHeight;
      this->ExtendV = ExtendV;

      return true;
   }

   //=======================================
   // Prepare container
   //=======================================

   // Search FMT from FROMATDEF
   sFormatDef *FormatDef = NULL;
   int i = 0;
   while (i < VFORMAT_NBR && QString(FORMATDEF[i].FileExtension) != FMT) 
      i++;

   // if FMT not found, search it from AUDIOFORMATDEF
   if (i >= VFORMAT_NBR) 
   {
      int i = 0;
      while (i < NBR_AFORMAT && QString(AUDIOFORMATDEF[i].FileExtension) != FMT) 
         i++;
      if (i >= NBR_AFORMAT) 
      {
         ToLog(LOGMSG_CRITICAL,QString("EncodeVideo-OpenEncoder: Unknown format %1").arg(FMT));
         return false;
      } 
      else 
         FormatDef = &AUDIOFORMATDEF[i];
   } 
   else 
      FormatDef = &FORMATDEF[i];

   // Alloc container
   /* -- */const  AVOutputFormat *oformat = av_guess_format(QString(FormatDef->ShortName).toUtf8().constData(), NULL, NULL);
#ifdef IS_FFMPEG_440
   /* -- */avformat_alloc_output_context2(&Container, NULL, oformat->name, NULL);
#else
   /* -- */avformat_alloc_output_context2(&Container, oformat, NULL, NULL);
#endif
   //-- avformat_alloc_output_context2(&Container, NULL, NULL, OutputFileName.toUtf8().constData());
   if (!Container)
   {
      ToLog(LOGMSG_CRITICAL,"EncodeVideo-OpenEncoder: Unable to allocate AVFormatContext");
      return false;
   }

   //// Prepare container struct
   //-- Container->oformat = av_guess_format(QString(FormatDef->ShortName).toUtf8().constData(),NULL,NULL);
   Container->url = av_strdup(OutputFileName.toUtf8().constData());
   //AVOutputFormat *f414 = my_av_guess_format_414(QString(FormatDef->ShortName).toUtf8().constData(), NULL, NULL);
   //AVOutputFormat *f342 = my_av_guess_format_342(QString(FormatDef->ShortName).toUtf8().constData(), NULL, NULL);
   if (!Container->oformat)
   {
      ToLog(LOGMSG_CRITICAL,QString("EncodeVideo-OpenEncoder: Unable to guess av_guess_format from container %1").arg(QString(FormatDef->ShortName)));
      return false;
   }
   if (Container->oformat->flags & AVFMT_NOFILE) 
   {
      ToLog(LOGMSG_CRITICAL,QString("EncodeVideo-OpenEncoder: Container->oformat->flags==AVFMT_NOFILE"));
      return false;
   }

   //=======================================
   // Open video stream
   //=======================================
   if (EncodeVideo) 
   {
      // Video parameters
      this->VideoBitrate    = VideoBitrate;
      this->ImageDef        = ImageDef;
      this->VideoFrameRate  = ImageDef->AVFPS;
      this->VideoCodecSubId = VideoCodecSubId;
      this->InternalWidth   = InternalWidth;
      this->InternalHeight  = InternalHeight;
      this->ExtendV         = ExtendV;

      // Add stream
      if (!OpenVideoStream(&VIDEOCODECDEF[VideoCodecSubId],VideoCodecSubId,VBR,VideoFrameRate,ImageWidth,ImageHeight+ExtendV,PixelAspectRatio,VideoBitrate))
         return false;
      iRenderWidth = InternalWidth;
      iRenderHeight = InternalHeight;
      if( bDSR )
      {
         //iRenderWidth = 1920*2;
         //iRenderHeight = 1080*2;
         iRenderWidth = 1920;
         iRenderHeight = 1080;
      }
   } 
   else 
   {
      // If sound only, ensure FrameRate have a value
      VideoFrameRate.num=1;
      VideoFrameRate.den=25;
   }

   //=======================================
   // Open Audio stream
   //=======================================
   if (EncodeAudio) 
   {
      // Audio parameters
      this->AudioCodecSubId=AudioCodecSubId;

      // Add stream
      if (!OpenAudioStream(&AUDIOCODECDEF[AudioCodecSubId],AudioChannels,AudioBitrate,AudioSampleRate,Language))
         return false;

      this->AudioChannels   = AudioChannels;
      this->AudioBitrate    = AudioBitrate;
      this->AudioSampleRate = AudioSampleRate;
   }                          

   //********************************************
   // Open file and header
   //********************************************
   if (!PrepareTAG(Language)) 
      return false;

   #if LIBAVCODEC_VERSION_MAJOR < 58
   int errcode = avio_open(&Container->pb, Container->filename, AVIO_FLAG_WRITE/*| AVIO_FLAG_DIRECT*/);
   #else
   int errcode = avio_open(&Container->pb, Container->url, AVIO_FLAG_WRITE/*| AVIO_FLAG_DIRECT*/);
   #endif
   if (errcode < 0)
   {
      ToLog(LOGMSG_CRITICAL,QString("EncodeVideo-OpenEncoder: avio_open() failed : %1").arg(GetAvErrorMessage(errcode)));
      QString ErrorMessage=errcode==-2?QString("\n\n")+QApplication::translate("DlgRenderVideo","invalid path or invalid filename"):QString("\n\n%1").arg(GetAvErrorMessage(errcode));
      CustomMessageBox(ParentWindow,QMessageBox::Critical,QApplication::translate("DlgRenderVideo","Start rendering process"),
         QApplication::translate("DlgRenderVideo","Error starting encoding process:")+ErrorMessage);
      return false;
   }
   int mux_preload = int(0.5*AV_TIME_BASE);
   int mux_max_delay = int(0.7*AV_TIME_BASE);
   int mux_rate = 0;
   int packet_size = -1;

   if (QString(Container->oformat->name) == QString("mpegts")) 
   {
      packet_size = 188;
      mux_rate    = int(ostVideo.cc->bit_rate*1.1);
   } 
   else if (QString(Container->oformat->name) == QString("matroska")) 
   {
      mux_rate      = 10080*1000;
      mux_preload   = AV_TIME_BASE/10;  // 100 ms preloading
      mux_max_delay = 200*1000;         // 500 ms
   } 
   else if (QString(Container->oformat->name) == QString("webm")) 
   {
      mux_rate      = 10080*1000;
      mux_preload   = AV_TIME_BASE/10;  // 100 ms preloading
      mux_max_delay = 200*1000;         // 500 ms
   }
   Container->flags |= AVFMT_FLAG_NONBLOCK;
   Container->max_delay = mux_max_delay;
   av_opt_set_int(Container,"preload",mux_preload,0);
   av_opt_set_int(Container,"muxrate",mux_rate,0);
   
   if (packet_size != -1) 
      Container->packet_size = packet_size;

   AVDictionary * av_opts = NULL;
   if(enableFaststart)
      av_dict_set(&av_opts, "movflags", "faststart+disable_chpl", 0);

   if (avformat_write_header(Container,&av_opts)<0) 
   {
      //av_dict_free( &av_opts );
      ToLog(LOGMSG_CRITICAL,"EncodeVideo-OpenEncoder: avformat_write_header() failed");
      return false;
   }
   //AVDictionaryEntry *t = NULL;
   //while( ( t = av_dict_get( av_opts, "", t, AV_DICT_IGNORE_SUFFIX ) ) )
   //{
   //   //qDebug() << "muxavformat: Unknown option " << t->key;
   //}
   //av_dict_free( &av_opts );

   //********************************************
   // Log output format
   //********************************************
   IsOpen = true;

   //=======================================
   // Init counter
   //=======================================
   dFPS = qreal(VideoFrameRate.den)/qreal(VideoFrameRate.num);
   NbrFrame = int(qreal(Diaporama->GetPartialDuration(FromSlide,ToSlide))*dFPS/1000);    // Number of frame to generate

   return true;
}

//*************************************************************************************************************************************************
// Create a stream
//*************************************************************************************************************************************************
bool cEncodeVideo::AddStream(AVStream **Stream,const AVCodec **codec,const char *CodecName,AVMediaType Type) 
{
   AVCodecContext *c;
    *codec = avcodec_find_encoder_by_name(CodecName);
    if (!(*codec)) 
    {
        ToLog(LOGMSG_CRITICAL,QString("EncodeVideo-AddStream: Unable to find codec %1").arg(CodecName));
        return false;
    }
    if ((*codec)->id == AV_CODEC_ID_NONE) 
    {
        ToLog(LOGMSG_CRITICAL,QString("EncodeVideo-AddStream: codec->id==AV_CODEC_ID_NONE"));
        return false;
    }

    // Create stream
//    *Stream = avformat_new_stream(Container, NULL);
    *Stream = avformat_new_stream(Container, *codec);
    if (!(*Stream))
    {
        ToLog(LOGMSG_CRITICAL,"EncodeVideo-AddStream: avformat_new_stream() failed");
        return false;
    }
    if (Type == AVMEDIA_TYPE_VIDEO)
       c = ostVideo.cc = avcodec_alloc_context3(*codec);
    else
       c = ostAudio.cc = avcodec_alloc_context3(*codec);

#if LIBAVCODEC_VERSION_MAJOR < 59
    if (Type == AVMEDIA_TYPE_VIDEO) 
       Container->oformat->video_codec = (*codec)->id;
    else if (Type == AVMEDIA_TYPE_AUDIO) 
       Container->oformat->audio_codec = (*codec)->id;
#endif

    /* Some formats want stream headers to be separate. */
    if (Container->oformat->flags & AVFMT_GLOBALHEADER)
       c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; 

    int ThreadC = ((getCpuCount()/*-1*/)>1)?(getCpuCount()-1) : 1;
    if (ThreadC > 0)
    {
       if (strcmp(CodecName, "mpeg2video") == 0 ||
          strcmp(CodecName, "libvpx") == 0)
          c->thread_count = 16;
       //(*Stream)->codec->thread_count = 16;
       else
          c->thread_count = 16; // dont use more!! at least x265 will fail
          //(*Stream)->codec->thread_count = 64;// ThreadC * 2;
    }
    //(*Stream)->codec->thread_type = getThreadFlags((*codec)->id);
    c->thread_type = getThreadFlags((*codec)->id);

    return true;
}

//*************************************************************************************************************************************************
// Create video streams
//*************************************************************************************************************************************************
bool cEncodeVideo::OpenVideoStream(sVideoCodecDef *VideoCodecDef,int VideoCodecSubId,bool VBR,AVRational VideoFrameRate,
                                   int ImageWidth,int ImageHeight,AVRational PixelAspectRatio,int VideoBitrate) 
{
   const AVCodec *codec;
   if (!AddStream(&ostVideo.stream,&codec,VideoCodecDef->ShortName,AVMEDIA_TYPE_VIDEO)) 
      return false;

   bool isCudaEncoder = QString(VideoCodecDef->ShortName).contains("nvenc");
   bool isQSVEncoder = QString(VideoCodecDef->ShortName).contains("qsv");
   AVDictionary *opts=NULL;
   int MinRate = -1;
   int MaxRate = -1;
   int BufSize = -1;
   int BFrames = -1;

   // Setup codec parameters
   ostVideo.cc->width               = ImageWidth;
   ostVideo.cc->height              = ImageHeight;
   ostVideo.cc->pix_fmt             = AV_PIX_FMT_YUV420P;
   if (isQSVEncoder )
      ostVideo.cc->pix_fmt = AV_PIX_FMT_NV12;
   ostVideo.cc->time_base           = VideoFrameRate;
   ostVideo.cc->framerate = av_inv_q(VideoFrameRate);// MakeAVRational(VideoFrameRate.den, VideoFrameRate.num); // toCheck!!! av_inv_q
   //ostVideo.cc->framerate = VideoFrameRate;
   ostVideo.stream->time_base                  = VideoFrameRate;   //new ffmpeg 2.5.0!!!!
   ostVideo.cc->sample_aspect_ratio = PixelAspectRatio;
   ostVideo.stream->sample_aspect_ratio        = PixelAspectRatio;
   ostVideo.cc->gop_size = qRound(1/av_q2d(VideoFrameRate))/2; //12;

   if ((codec->id != AV_CODEC_ID_H264 && codec->id != AV_CODEC_ID_H265) || (!VBR))
   {
      ostVideo.cc->bit_rate = VideoBitrate;
      av_dict_set(&opts,"b",QString("%1").arg(VideoBitrate).toUtf8(),0);
   }

   if (codec->id == AV_CODEC_ID_MPEG2VIDEO) 
   {
      BFrames = 2;
   } 
   else if (codec->id == AV_CODEC_ID_MJPEG) 
   {
      //-qscale 2 -qmin 2 -qmax 2
      ostVideo.cc->pix_fmt             = AV_PIX_FMT_YUVJ420P; //  AV_PIX_FMT_YUV420P; //
      ostVideo.cc->qmin                = 2;
      ostVideo.cc->qmax                = 2;
      ostVideo.cc->bit_rate_tolerance  = int(qreal(int64_t(ImageWidth)*int64_t(ImageHeight)*int64_t(VideoFrameRate.den))/qreal(VideoFrameRate.num))*10;
      //ostVideo.cc->color_range = AVCOL_RANGE_MPEG;
   } 
   else if (codec->id == AV_CODEC_ID_VP8) 
   {
      BFrames=3;
      ostVideo.cc->gop_size = 120;
      ostVideo.cc->qmax     = ImageHeight<=576?63:51;                av_dict_set(&opts,"qmax",QString("%1").arg(ostVideo.cc->qmax).toUtf8(),0);
      ostVideo.cc->qmin     = ImageHeight<=576?1:11;                 av_dict_set(&opts,"qmin",QString("%1").arg(ostVideo.cc->qmin).toUtf8(),0);
      ostVideo.cc->mb_lmin  = ostVideo.cc->qmin*FF_QP2LAMBDA;
      ostVideo.cc->mb_lmax = ostVideo.cc->qmax*FF_QP2LAMBDA;
      #if LIBAVCODEC_VERSION_MAJOR < 58
      ostVideo.cc->lmin     = ostVideo.cc->qmin*FF_QP2LAMBDA;
      ostVideo.cc->lmax     = ostVideo.cc->qmax*FF_QP2LAMBDA;
      #endif

      if (ImageHeight <= 720) 
         av_dict_set(&opts,"profile","0",0); 
      else 
         av_dict_set(&opts,"profile","1",0);
      if (ImageHeight > 576)  
         av_dict_set(&opts,"slices","4",0);

      av_dict_set(&opts,"lag-in-frames","16",0);
      av_dict_set(&opts,"deadline","good",0);
      if (ostVideo.cc->thread_count > 0) 
         av_dict_set(&opts,"cpu-used",QString("%1").arg(ostVideo.cc->thread_count).toUtf8(),0);
   } 
   else if (codec->id == AV_CODEC_ID_VP9)
   {
      ostVideo.cc->gop_size = 240;
      if (useCRF)
      {
         //ostVideo.cc->bit_rate = 0;
         av_dict_set(&opts, "b", QString("%1").arg(0).toUtf8(), 0);
         av_dict_set(&opts, "crf", QString("%1").arg(iCRF).toUtf8(), 0);
      }
      else
      {
      }
      av_dict_set(&opts, "lag-in-frames", "16", 0);
      av_dict_set(&opts, "deadline", "good", 0);
      if (ostVideo.cc->thread_count > 0)
         av_dict_set(&opts, "cpu-used", QString("%1").arg(4).toUtf8(), 0);
      av_dict_set(&opts, "row-mt", "1", 0);

      MinRate = int(double(VideoBitrate)*VBRMINCOEF);
      MaxRate = int(double(VideoBitrate)*VBRMAXCOEF);
      //av_dict_set(&opts, "minrate", QString("%1").arg(MinRate).toUtf8(), 0);
      //av_dict_set(&opts, "maxrate", QString("%1").arg(MaxRate).toUtf8(), 0);
   }
   else if (codec->id == AV_CODEC_ID_H264 || codec->id == AV_CODEC_ID_H265)
   {
      ostVideo.cc->qmin = 0;             
      ostVideo.cc->qmax = 69;            
      av_dict_set(&opts,"qmin",QString("%1").arg(ostVideo.cc->qmin).toUtf8(),0);
      av_dict_set(&opts,"qmax",QString("%1").arg(ostVideo.cc->qmax).toUtf8(),0);

      ostVideo.cc->ticks_per_frame = 2; // tocheck
      if (ostVideo.cc->thread_count > 0)     
         av_dict_set(&opts,"threads",QString("%1").arg(ostVideo.cc->thread_count).toUtf8(),0);
      //av_dict_set(&opts,"lookahead-threads",QString("%1").arg(ostVideo.cc->thread_count).toUtf8(),0);

      cApplicationConfig* appConfig = pAppConfig;
      switch (VideoCodecSubId) 
      {
         case VCODEC_H264HQ:     // High Quality H.264 AVC/MPEG-4 AVC
         case VCODEC_H264PQ:     // Phone Quality H.264 AVC/MPEG-4 AVC
         case VCODEC_H265:
            if(isCudaEncoder)
               av_dict_set(&opts, "refs", "0", 0);
            else
               av_dict_set(&opts, "refs", "3", 0);
            if(!useCRF)
               av_dict_set(&opts, "b", QString("%1").arg(VideoBitrate).toUtf8(), 0);
            if (isCudaEncoder)
            {
               //av_dict_set(&opts, "b", QString("%1").arg(VideoBitrate).toUtf8(), 0);
               av_dict_set_int(&opts, "qmin", 10, 0);
               av_dict_set_int(&opts, "qmax", 69, 0);
               av_dict_set(&opts, "rc", "vbr", 0);
            }
            if (VBR)
            {
               MinRate = int(double(VideoBitrate)*VBRMINCOEF);
               MaxRate = int(double(VideoBitrate)*VBRMAXCOEF);
               BufSize = int(double(VideoBitrate) * 4);
            }
            else
            {
               MinRate = int(double(VideoBitrate)*0.9);
               MaxRate = int(double(VideoBitrate)*1.1);
               BufSize = int(double(VideoBitrate) * 2);
            }

            if (VideoCodecSubId == VCODEC_H264HQ || VideoCodecSubId == VCODEC_H265)
            {
               QString presetHQ = appConfig->Preset_HQ;
               if (isCudaEncoder && presetHQ == "faster")
                  presetHQ = "p2";
               av_dict_set(&opts, "preset", presetHQ.toLocal8Bit(), 0);
               //const AVOption* o = av_opt_find((void *)codec, "tune", NULL, 0, 0);
               if(isCudaEncoder)
                  av_dict_set_int(&opts, "tune", 1, 0);
               else
                  av_dict_set(&opts, "tune", appConfig->Tune_HQ.toLocal8Bit(), 0);
               if (VideoCodecSubId == VCODEC_H264HQ && appConfig->Preset_HQ.compare("slow") == 0 )
                  av_dict_set(&opts, "x264-params", "b-adapt=2:trellis=1:me=umh", 0);
            }
            else
            {
               av_dict_set(&opts, "preset", appConfig->Preset_PQ.toLocal8Bit(), 0);
               av_dict_set(&opts, "tune", appConfig->Tune_PQ.toLocal8Bit(), 0);
            }
            ////                                                           High Quality   Phone Quality
            //av_dict_set(&opts,"preset",  (VideoCodecSubId == VCODEC_H264HQ ? pAppConfig->Preset_HQ  : pAppConfig->Preset_PQ).toLocal8Bit(),0);
            //av_dict_set(&opts,"vprofile",(VideoCodecSubId == VCODEC_H264HQ ? pAppConfig->Profile_HQ : pAppConfig->Profile_PQ).toLocal8Bit(),0);  // 2 versions to support differents libav/ffmpeg
            //if(VideoCodecSubId != VCODEC_H265)
            //   av_dict_set(&opts,"profile", (VideoCodecSubId == VCODEC_H264HQ ? pAppConfig->Profile_HQ : pAppConfig->Profile_PQ).toLocal8Bit(),0);
            //av_dict_set(&opts,"tune",    (VideoCodecSubId == VCODEC_H264HQ ? pAppConfig->Tune_HQ    : pAppConfig->Tune_PQ).toLocal8Bit(),0);
            break;

         case VCODEC_X264LL: // x264 lossless
            av_dict_set(&opts,"preset","veryfast",0);
            av_dict_set(&opts,"refs","3",0);
            av_dict_set(&opts,"qp",  "0",0);
            break;
         }
   }

   ostVideo.cc->keyint_min = 1;
   //...
   //if ((codec->id!=AV_CODEC_ID_H264)||(!VBR))
   //  ostVideo.cc->keyint_min=25;
   //...
   av_dict_set(&opts,"g",QString("%1").arg(ostVideo.cc->gop_size).toUtf8(),0);
   av_dict_set(&opts,"keyint_min",QString("%1").arg(ostVideo.cc->keyint_min).toUtf8(),0);
   //av_dict_set(&opts, "r", "25", 0);
   if (MinRate != -1 && !useCRF)
   {
      av_dict_set(&opts,"minrate",QString("%1").arg(MinRate).toUtf8(),0);
      av_dict_set(&opts,"maxrate",QString("%1").arg(MaxRate).toUtf8(),0);
      av_dict_set(&opts,"bufsize",QString("%1").arg(BufSize).toUtf8(),0);
   }
   if(useCRF)
      av_dict_set(&opts, "crf", QString("%1").arg(iCRF).toUtf8(), 0);

   if (BFrames != -1) 
   {
      ostVideo.cc->max_b_frames = BFrames;
      av_dict_set(&opts,"bf",QString("%1").arg(BFrames).toUtf8(),0);
   }
   ostVideo.cc->has_b_frames = (ostVideo.cc->max_b_frames > 0) ? 1 : 0;

   //// debug: show the option-dictionary
   AVDictionaryEntry *t = NULL;
   TOLOG(LOGMSG_INFORMATION, QString("video encoding options for %1 ").arg(codec->name));
   while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) 
   {
      TOLOG(LOGMSG_INFORMATION, QString("option %1 value %2").arg(t->key).arg(t->value));
      qDebug() << "opt " << t->key << " " << t->value;
   }

   //{
   //   char *buffer = 0;
   //   int err = av_dict_get_string(opts, &buffer, ' ', ':');
   //   qDebug() << buffer;
   //}
   // Open encoder
   int errcode = avcodec_open2(ostVideo.cc,codec,&opts);
   //if (av_dict_count(opts) > 0)
   //{
   //   char *buffer = 0;
   //   int err = av_dict_get_string(opts, &buffer,' ', ':');
   //   qDebug() << buffer;
   //}
   if (errcode < 0) 
   {
      char Buf[2048];
      av_strerror(errcode,Buf,2048);
      ToLog(LOGMSG_CRITICAL,"EncodeVideo-OpenostVideo.stream: avcodec_open2() failed: "+QString(Buf));
      return false;
   }

   
   // debug: show the option-dictionary
   // at this point the dictionary contains only values that can't be used
   if (av_dict_count(opts) > 0)
   {
      t = NULL;
      TOLOG(LOGMSG_INFORMATION, QString("\nunused video encoding options: "));
      while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) 
      {
         TOLOG(LOGMSG_INFORMATION, QString("option %1 value %2").arg(t->key).arg(t->value));
         qDebug() << "opt " << t->key << " " << t->value;
      }
   }

   // Create VideoFrameConverter
   VideoFrameConverter = sws_getContext(
      InternalWidth,ostVideo.cc->height,PIXFMT,                                    // Src Widht,Height,Format
      ostVideo.cc->width,ostVideo.cc->height,ostVideo.cc->pix_fmt,   // Destination Width,Height,Format
      SWS_BICUBIC,                                                                        // flags
      NULL,NULL,NULL);                                                                    // src Filter,dst Filter,param
   if (!VideoFrameConverter) 
   {
      ToLog(LOGMSG_CRITICAL,"EncodeVideo-OpenostVideo.stream: sws_getContext() failed");
      return false;
   }

   // Create and prepare VideoFrame and VideoFrameBuf
   ostVideo.frame = ALLOCFRAME();  // Allocate structure for RGB image
   if (!ostVideo.frame)
   {
      ToLog(LOGMSG_CRITICAL,"EncodeVideo-OpenostVideo.stream: avcodec_alloc_frame() failed");
      return false;
   } 
   else 
   {
      //VideoFrameBufSize = avpicture_get_size(ostVideo.cc->pix_fmt,ostVideo.cc->width,ostVideo.cc->height);
      VideoFrameBufSize = av_image_get_buffer_size(ostVideo.cc->pix_fmt, ostVideo.cc->width, ostVideo.cc->height,1);
      VideoFrameBuf = (u_int8_t *)av_malloc(VideoFrameBufSize);
      if (!VideoFrameBufSize || !VideoFrameBuf) 
      {
         ToLog(LOGMSG_CRITICAL,"EncodeVideo-OpenostVideo.stream: av_malloc() failed for VideoFrameBuf");
         return false;
      }
      ostVideo.frame->width = ostVideo.cc->width;
      ostVideo.frame->height = ostVideo.cc->height;
      ostVideo.frame->format = ostVideo.cc->pix_fmt;//AV_PIX_FMT_RGB24;//AV_PIX_FMT_YUV420P;//AV_PIX_FMT_RGB24;
      if( av_image_fill_arrays(ostVideo.frame->data, ostVideo.frame->linesize, VideoFrameBuf,
            ostVideo.cc->pix_fmt, ostVideo.cc->width, ostVideo.cc->height,1) 
         != VideoFrameBufSize)
      {
         ToLog(LOGMSG_CRITICAL, "EncodeVideo-EncodeVideo: avpicture_fill() failed for VideoFrameBuf");
         return false;
      }

   }
   /* copy the stream parameters to the muxer */
   /*ret = */avcodec_parameters_from_context(ostVideo.stream->codecpar, ostVideo.cc);
   //if (ret < 0)
   //{
   //   fprintf(stderr, "Could not copy the stream parameters\n");
   //   exit(1);
   //}
   return true;
}

//*************************************************************************************************************************************************
// Create audio streams
//*************************************************************************************************************************************************
bool cEncodeVideo::OpenAudioStream(sAudioCodecDef *AudioCodecDef,int &AudioChannels,int &AudioBitrate,int &AudioSampleRate,QString Language) 
{
   const AVCodec *codec;
   if (!AddStream(&ostAudio.stream,&codec,AudioCodecDef->ShortName,AVMEDIA_TYPE_AUDIO)) 
      return false;

   AVDictionary *opts = NULL;

   // Setup codec parameters
   ostAudio.cc->sample_fmt = AV_SAMPLE_FMT_S16;
   ostAudio.cc->CC_CHANNELS = AudioChannels;
   ostAudio.cc->sample_rate = AudioSampleRate;
   ostAudio.cc->time_base = MakeAVRational(1, AudioSampleRate);
   ostAudio.stream->time_base = MakeAVRational( 1, AudioSampleRate );

   av_dict_set(&ostAudio.stream->metadata,"language",Language.toUtf8().constData(),0);

   if (codec->id == AV_CODEC_ID_PCM_S16LE) 
   {
      AudioBitrate = AudioSampleRate*16*AudioChannels;
   } 
   else if (codec->id == AV_CODEC_ID_FLAC) 
   {
      av_dict_set(&opts,"lpc_coeff_precision","15",0);
      av_dict_set(&opts,"lpc_type","2",0);
      av_dict_set(&opts,"lpc_passes","1",0);
      av_dict_set(&opts,"min_partition_order","0",0);
      av_dict_set(&opts,"max_partition_order","8",0);
      av_dict_set(&opts,"prediction_order_method","0",0);
      av_dict_set(&opts,"ch_mode","-1",0);
   } 
   else if (codec->id == AV_CODEC_ID_AAC) 
   {
      //ostVideo.cc->profile=FF_PROFILE_AAC_MAIN;
      if (QString(AUDIOCODECDEF[2].ShortName)=="aac") 
      {
         ostAudio.cc->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
         ostAudio.cc->sample_fmt = AV_SAMPLE_FMT_FLTP;
      }
   } 
   else if (codec->id == AV_CODEC_ID_MP2) 
   {

   } 
   else if (codec->id == AV_CODEC_ID_MP3) 
   {
      ostAudio.cc->sample_fmt = AV_SAMPLE_FMT_S16P;
      av_dict_set(&opts,"reservoir","1",0);
   } 
   else if (codec->id == AV_CODEC_ID_VORBIS) 
   {
      ostAudio.cc->sample_fmt =AV_SAMPLE_FMT_FLTP;
   } 
   else if (codec->id == AV_CODEC_ID_AC3) 
   {
      ostAudio.cc->sample_fmt = AV_SAMPLE_FMT_FLTP;
   } 
   else if (codec->id == AV_CODEC_ID_AMR_NB) 
   {
      ostAudio.cc->CC_CHANNELS = 1;
      AudioChannels = 1;
   } 
   else if (codec->id == AV_CODEC_ID_AMR_WB) 
   {
      ostAudio.cc->CC_CHANNELS = 1;
      AudioChannels = 1;
   }
   ostAudio.cc->bit_rate = AudioBitrate;
   av_dict_set(&opts,"ab",QString("%1").arg(AudioBitrate).toUtf8(),0);
   AVChannelLayout avcl;
   av_channel_layout_default(&avcl, (ostAudio.cc->CC_CHANNELS == 2 ? 2 : 1));
   ostAudio.cc->ch_layout = avcl;

   int errcode = avcodec_open2(ostAudio.cc,codec,&opts);
   if (errcode < 0) 
   {
      char Buf[2048];
      av_strerror(errcode,Buf,2048);
      ToLog(LOGMSG_CRITICAL,"EncodeVideo-OpenAudioStream: avcodec_open2() failed: "+QString(Buf));
      return false;
   }

   ostAudio.frame = ALLOCFRAME();  // Allocate structure for audio data
   if (ostAudio.frame == NULL) 
   {
      ToLog(LOGMSG_CRITICAL,QString("EncodeVideo-OpenAudioStream:: avcodec_alloc_frame failed"));
      return false;
   }

   /*ret = */avcodec_parameters_from_context(ostAudio.stream->codecpar, ostAudio.cc);
   //if (ret < 0)
   //{
   //   fprintf(stderr, "Could not copy the stream parameters\n");
   //   exit(1);
   //}
   return true;
}

//*************************************************************************************************************************************************

bool cEncodeVideo::PrepareTAG(QString Language) 
{
   // Set TAGS
   av_dict_set(&Container->metadata,"language",Language.toUtf8().constData(),0);
   av_dict_set(&Container->metadata,"title",AdjustMETA(Diaporama->ProjectInfo()->Title() == "" ? QFileInfo(OutputFileName).baseName() : Diaporama->ProjectInfo()->Title()).toUtf8().constData(),0);
   av_dict_set(&Container->metadata,"artist",AdjustMETA(Diaporama->ProjectInfo()->Author()).toUtf8().constData(),0);
   av_dict_set(&Container->metadata,"album",AdjustMETA(Diaporama->ProjectInfo()->Album()).toUtf8().constData(),0);
   av_dict_set(&Container->metadata,"comment",AdjustMETA(Diaporama->ProjectInfo()->Comment()).toUtf8().constData(),0);
   av_dict_set(&Container->metadata,"date",QString("%1").arg(Diaporama->ProjectInfo()->EventDate().year()).toUtf8().constData(),0);
   av_dict_set(&Container->metadata,"composer",QString(QString(APPLICATION_NAME)+QString(" ")+CurrentAppName).toUtf8().constData(),0);
   av_dict_set(&Container->metadata,"creation_time",QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toUtf8().constData(),0); // ISO 8601 format

   // Set Chapters (only if video stream)
   if (ostVideo.stream) 
   {
      for (int i = FromSlide; i <= ToSlide; i++) 
         if (i == FromSlide || Diaporama->slides[i]->StartNewChapter) 
         {
            AVChapter *Chapter = (AVChapter *)av_mallocz(sizeof(AVChapter));
            int64_t Start = Diaporama->GetObjectStartPosition(i) + (i > FromSlide ? Diaporama->slides[i]->GetTransitDuration() : 0) - Diaporama->GetObjectStartPosition(FromSlide);
            int64_t Duration = Diaporama->slides[i]->GetDuration() - (i > FromSlide ? Diaporama->slides[i]->GetTransitDuration() : 0);
            int NextC = i+1;
            while (NextC < ToSlide && !Diaporama->slides[NextC]->StartNewChapter)
            {
               Duration = Duration+Diaporama->slides[NextC]->GetDuration();
               NextC++;
               if (NextC <= ToSlide) 
                  Duration = Duration - Diaporama->slides[NextC-1]->GetTransitDuration();
            }
            int64_t End = Start+Duration;
            int64_t ts_off = av_rescale_q(Container->start_time,AV_TIME_BASE_Q,ostVideo.stream->time_base);
            Chapter->id = Container->nb_chapters;
            Chapter->time_base = ostVideo.stream->time_base;
            Chapter->start = av_rescale_q((Start-ts_off)*1000,AV_TIME_BASE_Q,ostVideo.stream->time_base);
            Chapter->end = av_rescale_q((End-ts_off)*1000,AV_TIME_BASE_Q,ostVideo.stream->time_base);
            QString CptName = Diaporama->slides[i]->StartNewChapter ? Diaporama->slides[i]->ChapterName : Diaporama->ProjectInfo()->Title();
            av_dict_set(&Chapter->metadata,"title",CptName.toUtf8(),0);
            Container->chapters=(AVChapter **)av_realloc(Container->chapters,sizeof(AVChapter)*(Container->nb_chapters+1));
            Container->chapters[Container->nb_chapters] = Chapter;
            Container->nb_chapters++;
         }
   }
   return true;
}

//*************************************************************************************************************************************************

QString cEncodeVideo::AdjustMETA(QString Text) 
{
   //Metadata keys or values containing special characters (’=’, ’;’, ’#’, ’\’ and a newline) must be escaped with a backslash ’\’.
   Text.replace("=","\\=");
   Text.replace(";","\\;");
   Text.replace("#","\\#");
   //Text.replace("\\","\\\\");
   Text.replace("\n","\\\n");
#ifdef Q_OS_WIN
   return Text.toUtf8();
#else
   return Text;
#endif
}

//*************************************************************************************************************************************************
#define noTIMING_ENCODE
#define ASSEMBLY_WAITTIME 15
#define ENCODER_WAITTIME 15
bool cEncodeVideo::DoEncode() 
{
   iWaitPreload = 0;
   iWaitAssembly = 0;
   iWaitEncAudio = 0;
   iWaitEncVideo = 0;
   AdjustedDuration = -1;
   //conversionTime = 0;
   volatile bool Continue = true;
   allDone = false;
   RenderMusic.reset();
   ToEncodeMusic.reset();
   cDiaporamaObjectInfo *PreviousFrame = NULL;
   QSharedPointer<cDiaporamaObjectInfo> sFrame;
   QSharedPointer<cDiaporamaObjectInfo> sPreviousFrame;
   cDiaporamaObjectInfo *Frame = NULL;
   int FrameSize = 0;
   int renderImageByteCount = iRenderWidth * iRenderHeight * 4;
   if (renderImageByteCount != 0)
   {
      int64_t cacheSize = pAppConfig->ImagesCache.getMaxValue();
      int framelistmax = (cacheSize / 100 * 20) / renderImageByteCount;
      if (framelistmax < 5)
         framelistmax = 5;
      frameListFree.release(framelistmax); // TODO: make value dynamic!!
      cLuLoBlockCacheObject *blockObj;
      for (int i = 0; i < framelistmax; i++)
      {
         blockObj = new cLuLoBlockCacheObject(this, i, renderImageByteCount, &pAppConfig->ImagesCache);
         pAppConfig->ImagesCache.addBlockingItem(blockObj);
      }
   }
   else
      frameListFree.release(10); // TODO: make value dynamic!!

   maxFramesInList = 0;
   iMinAvailable = frameListFree.available();
   reusedFrames = 0;
   numYuvFrames = 0;

   //int endCached = ToSlide;
   //preloadSlidesCounter.release(5);
   //Diaporama->PreLoadItems(FromSlide,ToSlide,&preloadSlidesCounter,&Continue);
   //ThreadPreload.setFuture(QtConcurrent::run(Diaporama,&cDiaporama::PreLoadItems,FromSlide,ToSlide,&preloadSlidesCounter,&Continue));
   //Diaporama->PreLoadRenderItems(FromSlide, ToSlide > FromSlide ? FromSlide+1 :FromSlide ,&Continue);
   //Diaporama->PreLoadRenderItems(FromSlide, endCached, &Continue);
   ostVideo.IncreasingPts = qreal(1000)/dFPS;

   // Init RenderMusic and ToEncodeMusic
   if (ostAudio.stream)
   {
      RenderMusic.SetFPS(ostVideo.IncreasingPts,AudioChannels,AudioSampleRate,AV_SAMPLE_FMT_S16);
      FrameSize = ostAudio.cc->frame_size;
      if (!FrameSize && ostAudio.cc->codec_id == AV_CODEC_ID_PCM_S16LE) 
         FrameSize = 1024;
      if (FrameSize == 0 && ostVideo.stream) 
         FrameSize = (ostAudio.cc->sample_rate*ostAudio.stream->time_base.num)/ ostAudio.stream->time_base.den;
      // LIBAV 9 AND FFMPEG => ToEncodeMusic use AV_SAMPLE_FMT_S16 format
      int ComputedFrameSize = AudioChannels*av_get_bytes_per_sample(AV_SAMPLE_FMT_S16)*FrameSize;
      if (ComputedFrameSize == 0) 
         ComputedFrameSize = RenderMusic.SoundPacketSize;
      ToEncodeMusic.SetFrameSize(ComputedFrameSize,RenderMusic.Channels,AudioSampleRate,AV_SAMPLE_FMT_S16);
   }

   AudioFrameNbr = 0;
   VideoFrameNbr = 0;
   ostAudio.LastPts = 0;
   ostVideo.LastPts = 0;
   ostAudio.IncreasingPts = ostAudio.stream ? FrameSize*1000* ostAudio.cc->time_base.num/ ostAudio.cc->time_base.den : 0;
   StartTime = QTime::currentTime();
   Position = Diaporama->GetObjectStartPosition(FromSlide);  // Render current position
   ColumnStart = -1;                                            // Render start position of current object
   Column = FromSlide-1;                                   // Render current object
   RenderedFrame = 0;
   EncodedFrames = 0;

   Diaporama->resetSoundBlocks();
   int preLoadStart = Column;
   int preLoadEnd = Column + PRELOAD_LIMIT;
   if( preLoadEnd > ToSlide )
      preLoadEnd = ToSlide;
   Diaporama->PreLoadRenderItems(preLoadStart, preLoadEnd, &Continue);
   //ThreadPreload.setFuture(QtConcurrent::run(Diaporama, &cDiaporama::PreLoadRenderItems, preLoadStart, preLoadEnd, &Continue));
   //Diaporama->PreLoadRenderItems(Column, ToSlide > FromSlide ? Column + 1 : Column, &Continue);
   //Diaporama->PreLoadRenderItems(Column, ToSlide > FromSlide ? ToSlide : Column, &Continue);
   //pAppConfig->ImagesCache.dumpCache("after preload");
   //Diaporama->PreLoadRenderItems(Column < 0 ? 0 : Column,ToSlide,&Continue);
   // Init Resampler (if needed)
   if (ostAudio.stream)
   {
      if (ostAudio.cc->sample_fmt!=RenderMusic.SampleFormat 
         || ostAudio.cc->CC_CHANNELS != RenderMusic.Channels
         || AudioSampleRate!=RenderMusic.SamplingRate) 
      {
         if (!AudioResampler) 
         {
            AudioResampler = swr_alloc();
            AVChannelLayout avcl;
            av_channel_layout_default(&avcl, ToEncodeMusic.Channels);
            av_opt_set_chlayout(AudioResampler, "in_chlayout", &avcl, 0);
            av_opt_set_int(AudioResampler,"in_sample_rate",        ToEncodeMusic.SamplingRate,0);
            av_opt_set_chlayout(AudioResampler, "out_chlayout",    &ostAudio.cc->ch_layout, 0);
            av_opt_set_int(AudioResampler,"out_sample_rate",       ostAudio.cc->sample_rate,0);
            av_opt_set_int(AudioResampler,"in_channel_count",      ToEncodeMusic.Channels,0);
            av_opt_set_int(AudioResampler,"out_channel_count",     ostAudio.cc->CC_CHANNELS,0);
            av_opt_set_sample_fmt(AudioResampler,"out_sample_fmt", ostAudio.cc->sample_fmt,0);
            av_opt_set_sample_fmt(AudioResampler,"in_sample_fmt",  ToEncodeMusic.SampleFormat,0);
            if (swr_init(AudioResampler) < 0) 
            {
               ToLog(LOGMSG_CRITICAL,QString("DoEncode: swr_alloc_set_opts failed"));
               Continue = false;
            }
         }
      }
   }

   // Define InterleaveFrame to not compute it for each frame
   InterleaveFrame = renderOnly || (strcmp(Container->oformat->name,"avi") != 0);
#ifdef Q_OS_WIN
   QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
#endif

   QList<QTime> times;
   previewTimer.start();
   lastPreview = 0;
   ThreadEncode.setFuture(QT_CONCURRENT_RUN_1(this,&cEncodeVideo::Encoding,&Continue));
   QSize renderSize = QSize(iRenderWidth, iRenderHeight);
   for (RenderedFrame = 0; Continue && (RenderedFrame < NbrFrame); RenderedFrame++)
   {
      //autoTimer rf("renderFrame ");
#ifdef TIMING_ENCODE
      LONGLONG cp = curPCounter();
#endif
      // Calculate position & column
      AdjustedDuration = (Column >= 0 && Column < Diaporama->slides.count()) ? Diaporama->slides[Column]->GetDuration()-Diaporama->GetTransitionDuration(Column+1) : 0;
      if (AdjustedDuration < 33) 
         AdjustedDuration = 33; // Not less than 1/30 sec

      if (ColumnStart == -1 ||  Column == -1 || (Column < Diaporama->slides.count() && (ColumnStart+AdjustedDuration) <= Position) ) 
      {
         Column++;
         times.append(QTime::currentTime());
         Diaporama->ApplicationConfig()->ImagesCache.RemoveObjectImages();
         if (times.count() > 3)
         {
            Diaporama->ApplicationConfig()->ImagesCache.RemoveOlder(times.at(times.count() - 3));
            //pAppConfig->ImagesCache.dumpCache("after removeOlder");
         }
         //Diaporama->ApplicationConfig->ImagesCache.clear();
         clearLumaCache();
         AdjustedDuration = (Column >= 0 && Column < Diaporama->slides.count()) ? Diaporama->slides[Column]->GetDuration()-Diaporama->GetTransitionDuration(Column+1) : 0;
         if (AdjustedDuration < 33) 
            AdjustedDuration = 33; // Not less than 1/30 sec
         ColumnStart = Diaporama->GetObjectStartPosition(Column);
         Diaporama->CloseUnusedLibAv(Column,true);
         //if( ThreadPreload.isRunning() )
         //{
         //   ThreadPreload.waitForFinished();
         //   iWaitPreload++;
         //}
         if (Column >= (preLoadEnd-PRELOAD_ADVANCE) && Column < ToSlide - PRELOAD_LIMIT && !ThreadPreload.isRunning() )
         {
            preLoadStart = Column + 1;
            preLoadEnd = Column + 1 + PRELOAD_LIMIT;
            if (preLoadEnd > ToSlide)
               preLoadEnd = ToSlide;
            ThreadPreload.setFuture(QT_CONCURRENT_RUN_3(Diaporama, &cDiaporama::PreLoadRenderItems, preLoadStart, preLoadEnd, &Continue));
         }
         if( Column < ToSlide-3 )
            ThreadPreload.setFuture(QT_CONCURRENT_RUN_3(Diaporama,&cDiaporama::PreLoadRenderItems,Column+1,Column+2,&Continue));
         //if( Column == endCached )
        //{
         //   endCached = ToSlide;
         //   Diaporama->PreLoadRenderItems(Column, endCached, &Continue);
         //}
      }
#ifdef TIMING_ENCODE
      qDebug() << "DoEncode stage 1 " << PC2time(curPCounter()-cp,true);
      cp = curPCounter();
#endif
      // Get current frame
      Frame = new cDiaporamaObjectInfo(PreviousFrame,Position,Diaporama,ostVideo.IncreasingPts, ostAudio.stream !=NULL);
      sFrame.reset(Frame);
#ifdef TIMING_ENCODE
      qDebug() << "DoEncode stage 2 " << PC2time(curPCounter()-cp,true);
      cp = curPCounter();
#endif
      // Ensure AudioTracks are ready
      if(ostAudio.stream)
         Frame->ensureAudioTracksReady(ostVideo.IncreasingPts, AudioChannels, AudioSampleRate, AV_SAMPLE_FMT_S16);

      // Prepare frame (if W=H=0 then soundonly)
      if (Frame->IsTransition && Frame->Transit.Object) 
         Diaporama->CreateObjectContextList(Frame, renderSize, false, false, true, PreparedTransitBrushList);
#ifdef TIMING_ENCODE
      qDebug() << "DoEncode stage 7 " << PC2time(curPCounter()-cp,true);
      cp = curPCounter();
#endif

      Diaporama->CreateObjectContextList(Frame, renderSize, true, false, true, PreparedBrushList);
#ifdef TIMING_ENCODE
      qDebug() << "DoEncode stage 8 " << PC2time(curPCounter()-cp,true);
      cp = curPCounter();
#endif
      Diaporama->LoadSources(Frame, renderSize, false, true, PreparedTransitBrushList, PreparedBrushList, 2);
#ifdef TIMING_ENCODE
      qDebug() << "DoEncode stage 8a " << PC2time(curPCounter()-cp,true);
      cp = curPCounter();
#endif

#ifdef TIMING_ENCODE
      qDebug() << "DoEncode stage 8b " << PC2time(curPCounter()-cp,true);
      cp = curPCounter();
#endif
      // mix audio data
      if (Frame->Current.Object_MusicTrack) 
      {
         int MaxJ = Frame->Current.Object_MusicTrack->NbrPacketForFPS;
         RenderMusic.Mutex.lock();
         for (int j = 0; j < MaxJ; j++) 
         {
            int16_t *Music=(((Frame->IsTransition)&&(Frame->Transit.Object)&&(!Frame->Transit.Object->MusicPause)) ||
               (!Frame->Current.Object->MusicPause)) ? Frame->Current.Object_MusicTrack->DetachFirstPacket(true) : NULL;
            int16_t *Sound = (Frame->Current.Object_SoundTrackMontage != NULL) ? Frame->Current.Object_SoundTrackMontage->DetachFirstPacket(true) : NULL;
            RenderMusic.MixAppendPacket(Frame->Current.Object_StartTime + Frame->Current.Object_InObjectTime,Music,Sound,true);
         }
         RenderMusic.Mutex.unlock();
      }
#ifdef TIMING_ENCODE
      qDebug() << "DoEncode stage 9 " << PC2time(curPCounter()-cp,true);
      cp = curPCounter();
#endif
      Assembly(sFrame,sPreviousFrame,&Continue);
#ifdef TIMING_ENCODE
      qDebug() << "DoEncode stage 10 " << PC2time(curPCounter()-cp,true);
      cp = curPCounter();
#endif
      // Calculate next position
      if (Continue) 
      {
         //qDebug() << "Position " << qSetRealNumberPrecision(15) << Position << " + ostVideo.IncreasingPts " << ostVideo.IncreasingPts << " = " << (Position + ostVideo.IncreasingPts);
         Position += ostVideo.IncreasingPts;
         sPreviousFrame = sFrame;
         PreviousFrame = Frame;
         Frame = NULL;
      }

      // Stop the process if error occur or user ask to stop
      Continue = Continue && !StopProcessWanted;
   }
   //qDebug() << "loop ends, RenderedFrames = " << RenderedFrame;
   if( ThreadPreload.isRunning() ) 
   {
      ThreadPreload.waitForFinished();
      iWaitPreload++;
   }
   allDone = true;
   if( ThreadEncode.isRunning() ) 
      ThreadEncode.waitForFinished();

#ifdef Q_OS_WIN
   QThread::currentThread()->setPriority(QThread::HighestPriority);
#endif

   Position = -1;
   ColumnStart = 0;
   AdjustedDuration = 0;

   CloseEncoder();
   //qDebug() << "total conversion time is " << nano2time(conversionTime, true);
//   qDebug() << "iWaitPreload " << iWaitPreload;
//   qDebug() << "iWaitAssembly " << iWaitAssembly << " (" << iWaitAssembly*ASSEMBLY_WAITTIME << " msec)";
//   qDebug() << "iWaitEncAudio " << iWaitEncAudio;
//   qDebug() << "iWaitEncVideo " << iWaitEncVideo << " (" << iWaitEncVideo*ENCODER_WAITTIME << " msec)";
//   qDebug() << "maxFramesInList " << maxFramesInList;
//   qDebug() << "emptyFramesInList " << emptyFramesInList;
   return Continue;
}

//*************************************************************************************************************************************************
void cEncodeVideo::Assembly(QSharedPointer<cDiaporamaObjectInfo> Frame,QSharedPointer<cDiaporamaObjectInfo> PreviousFrame,volatile bool *Continue) 
{
   //AUTOTIMER(ass,"assembly");
   // Make final assembly
   bool msgDone = false;
   if( !Frame->hasYUV)
      Diaporama->DoAssembly(ComputePCT(Frame.data()->Current.Object ? Frame.data()->Current.Object->GetSpeedWave() : 0, Frame.data()->TransitionPCTDone), Frame.data(), QSize(iRenderWidth,iRenderHeight), QTPIXFMT);
   // place frame into framelist
   bool emptyFrame = (PreviousFrame && !PreviousFrame->IsTransition && Frame->IsShotStatic && !Frame->IsTransition) ? true : false;
   //if( !emptyFrame /*&& !Frame->hasYUV*/)
   if (iMinAvailable > frameListFree.available())
      iMinAvailable = frameListFree.available();
   if (!emptyFrame || Frame->hasYUV)
   {
      //QImage *Image = (PreviousFrame && !PreviousFrame->IsTransition && Frame->IsShotStatic && !Frame->IsTransition) ? NULL : &Frame->RenderedImage;
      //if( Image && bDSR && Image->width() > 1920 /*!= InternalWidth*/ )
     // {
      //   *Image = Image->scaledToWidth(InternalWidth,Qt::SmoothTransformation);
      //}
      while( !frameListFree.tryAcquire(1,ASSEMBLY_WAITTIME) )
      {
         if( !*Continue )
            return;
         iWaitAssembly++;
         msgDone = true;
      }
   }
   else
      reusedFrames++;
   if (Frame->hasYUV)
      numYuvFrames++;
   frameListMutex.lock();
   frameList.append(sFrames(Frame,PreviousFrame));
   if (frameList.size() > maxFramesInList)
      maxFramesInList = frameList.size();
   frameListMutex.unlock();
   frameListFilled.release();
}

void cEncodeVideo::Encoding(volatile bool *Continue)
{
   bool msgDone = false;
   while(Continue)
   {
      if( frameListFilled.tryAcquire(1,ENCODER_WAITTIME) )
      {
         //autoTimer enc("encodingloop");
         msgDone = false;
         QSharedPointer<cDiaporamaObjectInfo> pFrame;
         frameListMutex.lock();
         sFrames sf = frameList.takeFirst();
         frameListMutex.unlock();
         cDiaporamaObjectInfo *Frame = sf.Frame.data();
         cDiaporamaObjectInfo *PreviousFrame = sf.PreviousFrame.data();
         bool emptyFrame = (PreviousFrame && !PreviousFrame->IsTransition && Frame->IsShotStatic && !Frame->IsTransition) ? true : false;
         if( !emptyFrame || Frame->hasYUV)
            frameListFree.release();

         if (ThreadEncodeAudio.isRunning()) 
         {
            ThreadEncodeAudio.waitForFinished();
            iWaitEncAudio++;
         }

         // Write data to disk
         if (*Continue && ostAudio.stream && ostAudio.frame)
            ThreadEncodeAudio.setFuture(QT_CONCURRENT_RUN_4(this,&cEncodeVideo::EncodeMusic,Frame,&RenderMusic,&ToEncodeMusic,Continue));

         if (*Continue && (renderOnly || (ostVideo.stream && VideoFrameConverter && ostVideo.frame)) )
         {
            if (Frame->hasYUV)
            {
               if (!yuvPassThroughSignal)
               {
                  emit YUVPassThrough();
                  yuvPassThroughSignal = true;
               }
               if (renderOnly)
               {
                  VideoFrameNbr++;
                  EncodedFrames++;
                  ostVideo.LastPts += ostVideo.IncreasingPts;
               }
               else
                  EncodeVideoFrame(Frame->YUVFrame, Continue);
               //qDebug() << " av_frame_free " << Frame->YUVFrame;
               FREEFRAME(&Frame->YUVFrame);

            }
            else
            {
               QImage *Image = (PreviousFrame && !PreviousFrame->IsTransition && Frame->IsShotStatic && !Frame->IsTransition) ? NULL : &Frame->RenderedImage;
               if (Image && bDSR && iRenderWidth != InternalWidth)
               {
                  *Image = Image->scaledToWidth(InternalWidth, Qt::SmoothTransformation);
               }
               if (renderOnly)
               {
                  VideoFrameNbr++;
                  EncodedFrames++;
                  if (showPreview && Image)
                  {
                     qint64 currentElapsed = previewTimer.elapsed();
                     if (currentElapsed - lastPreview > PREVIEW_DIFTIME_MS)
                     {
                        emit ImageReady(Image->copy());
                        lastPreview = currentElapsed;
                        yuvPassThroughSignal = false;
                     }
                  }
                  ostVideo.LastPts += ostVideo.IncreasingPts;
               }
               else
                  EncodeVideo(Image, Continue);
            }
         }
      }
      else
      {
         //if( !msgDone) 
         //{
         //   qDebug() << "wait in encoder";
         //}
         iWaitEncVideo++;
         msgDone = true;
      }
      if( allDone && !frameListFilled.available() )
      {
         if (ThreadEncodeAudio.isRunning()) 
         {
            ThreadEncodeAudio.waitForFinished();
            iWaitEncAudio++;
         }
         return;
      }
   }
}

//*************************************************************************************************************************************************

void cEncodeVideo::EncodeMusic(cDiaporamaObjectInfo *Frame, cSoundBlockList *RenderMusic, cSoundBlockList *ToEncodeMusic, volatile bool *Continue) 
{
   // Transfer RenderMusic data to EncodeMusic data
   int MaxPackets = RenderMusic->NbrPacketForFPS;
   if (MaxPackets > RenderMusic->ListCount()) 
      MaxPackets = RenderMusic->ListCount();
   for (int Packets = 0; Continue && Packets < MaxPackets; Packets++) 
   {
      u_int8_t *PacketSound = (u_int8_t *)RenderMusic->DetachFirstPacket();
      if (PacketSound == NULL) 
      {
         PacketSound = (u_int8_t *)av_malloc(RenderMusic->SoundPacketSize+8);
         memset(PacketSound,0,RenderMusic->SoundPacketSize);
      }
      // LIBAV 9 AND FFMPEG => ToEncodeMusic is converted during encoding process
      ToEncodeMusic->AppendData(Frame->Current.Object_StartTime+Frame->Current.Object_InObjectTime,(int16_t *)PacketSound,RenderMusic->SoundPacketSize);
      av_free(PacketSound);
   }

   int      errcode;
   int64_t  DestNbrSamples = ToEncodeMusic->SoundPacketSize/(ToEncodeMusic->Channels*av_get_bytes_per_sample(ToEncodeMusic->SampleFormat));
   int      DestSampleSize = ostAudio.cc->CC_CHANNELS * av_get_bytes_per_sample(ostAudio.cc->sample_fmt);
   u_int8_t *DestPacket    = NULL;
   int16_t  *PacketSound   = NULL;
   int64_t  DestPacketSize = DestNbrSamples*DestSampleSize;

   // Flush audio frame of ToEncodeMusic
   while (*Continue && ToEncodeMusic->ListCount() > 0 && !StopProcessWanted) 
   {
      PacketSound = ToEncodeMusic->DetachFirstPacket();
      if (PacketSound == NULL) 
      {
         ToLog(LOGMSG_CRITICAL,QString("EncodeMusic: PacketSound==NULL"));
         *Continue = false;
      } 
      else 
      {
         if (AudioResampler != NULL) 
         {
            int out_samples = av_rescale_rnd(swr_get_delay(AudioResampler,ToEncodeMusic->SamplingRate)+DestNbrSamples,ostAudio.cc->sample_rate,ToEncodeMusic->SamplingRate,AV_ROUND_UP);
            av_samples_alloc(&AudioResamplerBuffer,NULL,ostAudio.cc->CC_CHANNELS,out_samples,ostAudio.cc->sample_fmt,0);
            if (!AudioResamplerBuffer) 
            {
               ToLog(LOGMSG_CRITICAL,QString("EncodeMusic: AudioResamplerBuffer allocation failed"));
               *Continue = false;
            }
            u_int8_t *in_data[RESAMPLE_MAX_CHANNELS]={0},*out_data[RESAMPLE_MAX_CHANNELS] = {0};
            int     in_linesize = 0, out_linesize = 0;
            if (av_samples_fill_arrays(in_data,&in_linesize,(u_int8_t *)PacketSound,ToEncodeMusic->Channels,DestNbrSamples,ToEncodeMusic->SampleFormat,0) < 0) 
            {
               ToLog(LOGMSG_CRITICAL,QString("failed in_data fill arrays"));
               *Continue = false;
            } 
            else 
            {
               if (av_samples_fill_arrays(out_data,&out_linesize,AudioResamplerBuffer,ostAudio.cc->CC_CHANNELS,out_samples,ostAudio.cc->sample_fmt,0) < 0)
               {
                  ToLog(LOGMSG_CRITICAL,QString("failed out_data fill arrays"));
                  *Continue = false;
               } 
               else 
               {
                  DestPacketSize = swr_convert(AudioResampler,out_data,out_samples,(const u_int8_t **)in_data,DestNbrSamples)*DestSampleSize;
                  if (DestPacketSize <= 0) 
                  {
                     ToLog(LOGMSG_CRITICAL,QString("EncodeMusic: swr_convert failed"));
                     *Continue = false;
                  }
                  DestPacket = (u_int8_t *)out_data[0];
               }
            }
         } 
         else 
            DestPacket = (u_int8_t *)PacketSound;
         if (Continue) 
         {
            // Init AudioFrame
            AVRational AVR;

#if (FFMPEGVERSIONINT>=220)
            av_frame_unref(ostAudio.frame);
#else
            avcodec_get_frame_defaults(ostAudio.frame);
#endif

            AVR.num                     = 1;
            AVR.den                     = ostAudio.cc->sample_rate;
            ostAudio.frame->nb_samples      = DestPacketSize/DestSampleSize;
            ostAudio.frame->pts = ostAudio.LastPts;// av_rescale_q(ostAudio.frame->nb_samples*AudioFrameNbr, AVR, ostAudio.stream->time_base);
            ostAudio.frame->format          = ostAudio.cc->sample_fmt;
            ostAudio.frame->ch_layout = ostAudio.cc->ch_layout;
            ostAudio.frame->CC_CHANNELS = ostAudio.cc->CC_CHANNELS; // !!!

            // fill buffer
            errcode = avcodec_fill_audio_frame(ostAudio.frame,ostAudio.cc->CC_CHANNELS,ostAudio.cc->sample_fmt,(const u_int8_t*)DestPacket,DestPacketSize,1);
            if (errcode >= 0) 
            {
               // Init packet
               AVPacket* pkt = av_packet_alloc();

               encode(ostAudio.cc, ostAudio.frame, pkt);
               av_packet_free(&pkt); 
               ostAudio.LastPts += ostAudio.frame->nb_samples; //ostAudio.IncreasingPts;
               AudioFrameNbr++;
               //ToLog(LOGMSG_INFORMATION,QString("Audio:  Stream:%1 - Frame:%2 - PTS:%3").arg(AudioStream->index).arg(ostAudio.frameNbr).arg(LastAudioPts));
            } 
            else 
            {
               char Buf[2048];
               av_strerror(errcode,Buf,2048);
               ToLog(LOGMSG_CRITICAL,QString("EncodeMusic: avcodec_fill_audio_frame() failed: ")+QString(Buf));
               *Continue = false;
            }
         }

         if (ostAudio.frame->extended_data && ostAudio.frame->extended_data != ostAudio.frame->data) 
         {
            av_free(ostAudio.frame->extended_data);
            ostAudio.frame->extended_data = NULL;
         }

      }
      av_free(PacketSound);

#if defined(USELIBSWRESAMPLE)
      av_free(AudioResamplerBuffer);
      AudioResamplerBuffer = NULL;
#endif
   }
}

//*************************************************************************************************************************************************

void cEncodeVideo::EncodeVideo(const QImage *SrcImage,volatile bool *Continue) 
{
   const QImage *Image = SrcImage;
//   int errcode;

   if (Image)
   {
       // Apply ExtendV
       if (*Continue && !StopProcessWanted && Image->height() != ostVideo.cc->height)
       {
          //QTime t;
          //t.start();
          QImage *ExtVImage = new QImage(InternalWidth, ostVideo.cc->height, QTPIXFMT);
          ExtVImage->fill(Qt::black);
          QPainter P;
          P.begin(ExtVImage);
          P.drawImage(0, (ostVideo.cc->height - SrcImage->height()) / 2, *SrcImage);
          P.end();
          Image = ExtVImage;
       }

       // Now, convert image
       if (Continue && !StopProcessWanted)
       {
          if (showPreview)
          {
             qint64 currentElapsed = previewTimer.elapsed();
             if (currentElapsed - lastPreview > PREVIEW_DIFTIME_MS)
             {
                emit ImageReady(Image->copy());
                yuvPassThroughSignal = false;
                lastPreview = currentElapsed;
             }
          }
          u_int8_t *data = { (u_int8_t *)Image->constBits() };
          int LineSize = Image->bytesPerLine();
          int Ret = sws_scale(
             VideoFrameConverter,    // libswscale converter
             &data,                  // Source buffer
             &LineSize,              // Source Stride ?
             0,                      // Source SliceY:the position in the source image of the slice to process, that is the number (counted starting from zero) in the image of the first row of the slice
             Image->height(),        // Source SliceH:the height of the source slice, that is the number of rows in the slice
             ostVideo.frame->data,       // Destination buffer
             ostVideo.frame->linesize    // Destination Stride
          );
          if (Ret != Image->height())
          {
             ToLog(LOGMSG_CRITICAL, "EncodeVideo-ConvertRGBToYUV: sws_scale() failed");
             *Continue = false;
          }
       }
   }

   if ((VideoFrameNbr % ostVideo.cc->gop_size) == 0)
      ostVideo.frame->pict_type = AV_PICTURE_TYPE_I;
   else
      ostVideo.frame->pict_type = AV_PICTURE_TYPE_NONE;//(AVPictureType)0;
   ostVideo.frame->pts = VideoFrameNbr;

   if (*Continue && !StopProcessWanted)
   {
      AVPacket* pkt = av_packet_alloc();

      encode(ostVideo.cc, ostVideo.frame, pkt);
      av_packet_free(&pkt);
 
      ostVideo.LastPts += ostVideo.IncreasingPts;
      VideoFrameNbr++;
      //ToLog(LOGMSG_INFORMATION,QString("Video:  Stream:%1 - Frame:%2 - PTS:%3").arg(ostVideo.stream->index).arg(VideoFrameNbr).arg(LastVideoPts));

      if (ostVideo.frame->extended_data && ostVideo.frame->extended_data != ostVideo.frame->data)
      {
         av_free(ostVideo.frame->extended_data);
         ostVideo.frame->extended_data = NULL;
      }
   }
   if (Image && Image != SrcImage)
      delete Image;
   EncodedFrames++;
}

void cEncodeVideo::EncodeVideoFrame(AVFrame *frame, volatile bool *Continue)
{
   //int errcode;
   if(frame == NULL )
      return;
//   ostVideo.cc->width, ostVideo.cc->height, ostVideo.cc->pix_fmt,   // Destination Width,Height,Format

   AVFrame *renderFrame = frame;
   if (frame->format != ostVideo.cc->pix_fmt  || frame->width != ostVideo.cc->width || frame->height != ostVideo.cc->height)
   {
      VideoYUVConverter = sws_getCachedContext(VideoYUVConverter,
         frame->width,                                                     // Src width
         frame->height,                                                    // Src height
         (AVPixelFormat)frame->format,                                       // Src Format
         ostVideo.cc->width,                                                                // Destination width
         ostVideo.cc->height,                                                                // Destination height
         ostVideo.cc->pix_fmt,                                                           // Destination Format
         /*SWS_FAST_BILINEAR*/SWS_BICUBIC /*| SWS_ACCURATE_RND*/, NULL, NULL, NULL);                                      // flags,src Filter,dst Filter,param
      sws_scale(
         VideoYUVConverter,                                         // libswscale converter
         frame->data,                                               // Source buffer
         frame->linesize,                                           // Source Stride ?
         0,                                                         // Source SliceY:the position in the source image of the slice to process, that is the number (counted starting from zero) in the image of the first row of the slice
         frame->height,                                             // Source SliceH:the height of the source slice, that is the number of rows in the slice
         ostVideo.frame->data,                                      // Destination buffer
         ostVideo.frame->linesize                                   // Destination Stride
      );
      renderFrame = ostVideo.frame;
   }
   if ((VideoFrameNbr % ostVideo.cc->gop_size) == 0)
      renderFrame->pict_type = AV_PICTURE_TYPE_I;
   else
      renderFrame->pict_type = AV_PICTURE_TYPE_NONE;//(AVPictureType)0;
   renderFrame->pts = VideoFrameNbr;

   if (*Continue && !StopProcessWanted)
   {
      AVPacket* pkt = av_packet_alloc();
      encode(ostVideo.cc, renderFrame, pkt);
      av_packet_free(&pkt);
      ostVideo.LastPts += ostVideo.IncreasingPts;
      VideoFrameNbr++;
      //ToLog(LOGMSG_INFORMATION,QString("Video:  Stream:%1 - Frame:%2 - PTS:%3").arg(ostVideo.stream->index).arg(VideoFrameNbr).arg(LastVideoPts));

      if (ostVideo.frame->extended_data && ostVideo.frame->extended_data != ostVideo.frame->data)
      {
         av_free(ostVideo.frame->extended_data);
         ostVideo.frame->extended_data = NULL;
      }
   }
   EncodedFrames++;
}

void cEncodeVideo::flush_encoders(void)
{
   AVPacket* pkt = av_packet_alloc();


   if(hasVideoStream())
      encode(ostVideo.cc, NULL, pkt);
   if (hasAudioStream())
      encode(ostAudio.cc, NULL, pkt);
   av_packet_free(&pkt); 
}
 
bool cEncodeVideo::testEncode()
{
   //iWaitPreload = 0;
   //iWaitAssembly = 0;
   //iWaitEncAudio = 0;
   //iWaitEncVideo = 0;
   volatile bool Continue = true;
   //allDone = false;
   //RenderMusic.reset();
   //ToEncodeMusic.reset();
   //cDiaporamaObjectInfo *PreviousFrame = NULL;//, *PreviousPreviousFrame = NULL;
   //QSharedPointer<cDiaporamaObjectInfo> sFrame;
   //QSharedPointer<cDiaporamaObjectInfo> sPreviousFrame;
   //cDiaporamaObjectInfo *Frame = NULL;
   //int FrameSize = 0;
   //frameListFree.release(10); // TODO: make value dynamic!!

   //                            //preloadSlidesCounter.release(5);
   //                            //Diaporama->PreLoadItems(FromSlide,ToSlide,&preloadSlidesCounter,&Continue);
   //                            //ThreadPreload.setFuture(QtConcurrent::run(Diaporama,&cDiaporama::PreLoadItems,FromSlide,ToSlide,&preloadSlidesCounter,&Continue));
   //                            //Diaporama->PreLoadRenderItems(FromSlide, ToSlide > FromSlide ? FromSlide+1 :FromSlide ,&Continue);
   //IncreasingVideoPts = qreal(1000) / dFPS;

   //// Init RenderMusic and ToEncodeMusic
   //if (AudioStream)
   //{
   //   RenderMusic.SetFPS(IncreasingVideoPts, AudioChannels, AudioSampleRate, AV_SAMPLE_FMT_S16);
   //   FrameSize = /*AudioStream->codec*/ AudioCodecContext->frame_size;
   //   if (!FrameSize && /*AudioStream->codec*/ AudioCodecContext->codec_id == AV_CODEC_ID_PCM_S16LE)
   //      FrameSize = 1024;
   //   if (FrameSize == 0 && VideoStream)
   //      FrameSize = (/*AudioStream->codec*/ AudioCodecContext->sample_rate*AudioStream->time_base.num) / AudioStream->time_base.den;
   //   // LIBAV 9 AND FFMPEG => ToEncodeMusic use AV_SAMPLE_FMT_S16 format
   //   int ComputedFrameSize = AudioChannels*av_get_bytes_per_sample(AV_SAMPLE_FMT_S16)*FrameSize;
   //   if (ComputedFrameSize == 0)
   //      ComputedFrameSize = RenderMusic.SoundPacketSize;
   //   ToEncodeMusic.SetFrameSize(ComputedFrameSize, RenderMusic.Channels, AudioSampleRate, AV_SAMPLE_FMT_S16);
   //}

   //AudioFrameNbr = 0;
   //VideoFrameNbr = 0;
   //LastAudioPts = 0;
   //LastVideoPts = 0;
   //IncreasingAudioPts = AudioStream ? FrameSize * 1000 * /*AudioStream->codec*/ AudioCodecContext->time_base.num / /*AudioStream->codec*/ AudioCodecContext->time_base.den : 0;
   //StartTime = QTime::currentTime();
   //Position = Diaporama->GetObjectStartPosition(FromSlide);  // Render current position
   //ColumnStart = -1;                                            // Render start position of current object
   //Column = FromSlide - 1;                                   // Render current object
   //RenderedFrame = 0;
   //EncodedFrames = 0;

   //Diaporama->resetSoundBlocks();
   ////Diaporama->PreLoadRenderItems(Column, ToSlide > FromSlide ? Column + 1 : Column, &Continue);
   //Diaporama->PreLoadRenderItems(Column, ToSlide > FromSlide ? ToSlide : Column, &Continue);
   ////Diaporama->PreLoadRenderItems(Column < 0 ? 0 : Column,ToSlide,&Continue);
   //// Init Resampler (if needed)
   //if (AudioStream)
   //{
   //   if ((/*AudioStream->codec*/ AudioCodecContext->sample_fmt != RenderMusic.SampleFormat) || (/*AudioStream->codec*/ AudioCodecContext->channels != RenderMusic.Channels) || (AudioSampleRate != RenderMusic.SamplingRate))
   //   {
   //      if (!AudioResampler)
   //      {
   //         AudioResampler = swr_alloc();
   //         av_opt_set_int(AudioResampler, "in_channel_layout", av_get_default_channel_layout(ToEncodeMusic.Channels), 0);
   //         av_opt_set_int(AudioResampler, "in_sample_rate", ToEncodeMusic.SamplingRate, 0);
   //         av_opt_set_int(AudioResampler, "out_channel_layout", /*AudioStream->codec*/ AudioCodecContext->channel_layout, 0);
   //         av_opt_set_int(AudioResampler, "out_sample_rate", /*AudioStream->codec*/ AudioCodecContext->sample_rate, 0);
   //         av_opt_set_int(AudioResampler, "in_channel_count", ToEncodeMusic.Channels, 0);
   //         av_opt_set_int(AudioResampler, "out_channel_count", /*AudioStream->codec*/ AudioCodecContext->channels, 0);
   //         av_opt_set_sample_fmt(AudioResampler, "out_sample_fmt", /*AudioStream->codec*/ AudioCodecContext->sample_fmt, 0);
   //         av_opt_set_sample_fmt(AudioResampler, "in_sample_fmt", ToEncodeMusic.SampleFormat, 0);
   //         if (swr_init(AudioResampler)<0)
   //         {
   //            ToLog(LOGMSG_CRITICAL, QString("DoEncode: swr_alloc_set_opts failed"));
   //            Continue = false;
   //         }
   //      }
   //   }
   //}

   //// Define InterleaveFrame to not compute it for each frame
   //InterleaveFrame = renderOnly || (strcmp(Container->oformat->name, "avi") != 0);
   //#ifdef Q_OS_WIN
   //QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
   //#endif

   //QList<QTime> times;
   //previewTimer.start();
   //lastPreview = 0;
   //ThreadEncode.setFuture(QtConcurrent::run(this, &cEncodeVideo::nEncoding, &Continue));
   //for (RenderedFrame = 0; Continue && (RenderedFrame < NbrFrame); RenderedFrame++)
   //{
   //   autoTimer a("render Frame");
   //   // Calculate position & column
   //   AdjustedDuration = (Column >= 0 && Column < Diaporama->slides.count()) ? Diaporama->slides[Column]->GetDuration() - Diaporama->GetTransitionDuration(Column + 1) : 0;
   //   if (AdjustedDuration < 33) AdjustedDuration = 33; // Not less than 1/30 sec

   //   // start a new column if necessary
   //   if (ColumnStart == -1 || Column == -1 || (Column < Diaporama->slides.count() && (ColumnStart + AdjustedDuration) <= Position))
   //   {
   //      
   //      Column++;
   //      times.append(QTime::currentTime());
   //      Diaporama->ApplicationConfig()->ImagesCache.RemoveObjectImages();
   //      if (times.count() > 3)
   //         Diaporama->ApplicationConfig()->ImagesCache.RemoveOlder(times.at(times.count() - 3));
   //      clearLumaCache();
   //      AdjustedDuration = (Column >= 0 && Column < Diaporama->slides.count()) ? Diaporama->slides[Column]->GetDuration() - Diaporama->GetTransitionDuration(Column + 1) : 0;
   //      if (AdjustedDuration < 33) AdjustedDuration = 33; // Not less than 1/30 sec
   //      ColumnStart = Diaporama->GetObjectStartPosition(Column);
   //      Diaporama->CloseUnusedLibAv(Column, true);
   //      if (ThreadPreload.isRunning())
   //      {
   //         ThreadPreload.waitForFinished();
   //         iWaitPreload++;
   //      }
   //      if (Column < ToSlide - 3)
   //         ThreadPreload.setFuture(QtConcurrent::run(Diaporama, &cDiaporama::PreLoadRenderItems, Column + 1, Column + 2, &Continue));
   //   }
   //   // Get current frame
   //   Frame = new cDiaporamaObjectInfo(PreviousFrame, Position, Diaporama, IncreasingVideoPts, AudioStream != NULL);
   //   sFrame.reset(Frame);

   //   // Ensure AudioTracks are ready
   //   if (AudioStream)
   //      Frame->ensureAudioTracksReady(IncreasingVideoPts, AudioChannels, AudioSampleRate, AV_SAMPLE_FMT_S16);

   //   // Prepare frame (if W=H=0 then soundonly)
   //   if (Frame->IsTransition && Frame->Transit.Object)
   //      Diaporama->CreateObjectContextList(Frame, QSize(iRenderWidth, iRenderHeight), false, false, true, PreparedTransitBrushList);
   //   Diaporama->CreateObjectContextList(Frame, QSize(iRenderWidth, iRenderHeight), true, false, true, PreparedBrushList);
   //   Diaporama->LoadSourcesYUV(Frame, QSize(iRenderWidth, iRenderHeight), false, true, PreparedTransitBrushList, PreparedBrushList, 2);

   //   // mix audio data
   //   if (Frame->Current.Object_MusicTrack)
   //   {
   //      int MaxJ = Frame->Current.Object_MusicTrack->NbrPacketForFPS;
   //      RenderMusic.Mutex.lock();
   //      for (int j = 0; j < MaxJ; j++)
   //      {
   //         int16_t *Music = (((Frame->IsTransition) && (Frame->Transit.Object) && (!Frame->Transit.Object->MusicPause)) ||
   //            (!Frame->Current.Object->MusicPause)) ? Frame->Current.Object_MusicTrack->DetachFirstPacket(true) : NULL;
   //         int16_t *Sound = (Frame->Current.Object_SoundTrackMontage != NULL) ? Frame->Current.Object_SoundTrackMontage->DetachFirstPacket(true) : NULL;
   //         RenderMusic.MixAppendPacket(Frame->Current.Object_StartTime + Frame->Current.Object_InObjectTime, Music, Sound, true);
   //      }
   //      RenderMusic.Mutex.unlock();
   //   }
   //   //nAssembly(sFrame, sPreviousFrame, &Continue);
   //   double pct = ComputePCT(sFrame.data()->Current.Object ? sFrame.data()->Current.Object->GetSpeedWave() : 0, sFrame.data()->TransitionPCTDone);
   //   //Diaporama->DoAssembly(pct, sFrame.data(), QSize(iRenderWidth, iRenderHeight), QTPIXFMT);
   //   // doRender!!
   //   EncodeVideoFrame(sFrame.data()->YUVFrame, &Continue);
   //   if( sFrame.data()->hasYUV)
   //      av_frame_free(&sFrame.data()->YUVFrame);

   //   // Calculate next position
   //   if (Continue)
   //   {
   //      Position += IncreasingVideoPts;
   //      sPreviousFrame = sFrame;
   //      PreviousFrame = Frame;
   //      Frame = NULL;
   //   }

   //   // Stop the process if error occur or user ask to stop
   //   Continue = Continue && !StopProcessWanted;
   //}

   //if (ThreadPreload.isRunning())
   //{
   //   ThreadPreload.waitForFinished();
   //   iWaitPreload++;
   //}
   //allDone = true;
   //if (ThreadEncode.isRunning())
   //   ThreadEncode.waitForFinished();

   //#ifdef Q_OS_WIN
   //QThread::currentThread()->setPriority(QThread::HighestPriority);
   //#endif

   //Position = -1;
   //ColumnStart = 0;
   //AdjustedDuration = 0;

   //CloseEncoder();
   ////qDebug() << "total conversion time is " << nano2time(conversionTime, true);
   //qDebug() << "iWaitPreload " << iWaitPreload;
   //qDebug() << "iWaitAssembly " << iWaitAssembly << " (" << iWaitAssembly*ASSEMBLY_WAITTIME << " msec)";
   //qDebug() << "iWaitEncAudio " << iWaitEncAudio;
   //qDebug() << "iWaitEncVideo " << iWaitEncVideo << " (" << iWaitEncVideo*ENCODER_WAITTIME << " msec)";

   return Continue;
}

bool cEncodeVideo::encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt)
{
   int ret;

   /* send the frame for encoding */
   ret = avcodec_send_frame(ctx, frame);
   if (ret < 0)
   {
      char Buf[2048];
      av_strerror(ret, Buf, 2048);
      ToLog(LOGMSG_CRITICAL, "cEncodeVideo::encode: avcodec_send_frame() failed: " + QString(Buf));
      //fprintf(stderr, "Error sending the frame to the encoder\n");
      exit(1);
   }

   /* read all the available output packets (in general there may be any number of them */
   while (ret >= 0)
   {
      ret = avcodec_receive_packet(ctx, pkt);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
         return true;
      else if (ret < 0)
      {
         return false;
         //fprintf(stderr, "Error encoding audio frame\n");
         //exit(1);
      }

      encoderMutex.lock();
      OutputStream *stream = &ostVideo;
      if( ctx->codec_type == AVMEDIA_TYPE_AUDIO)
         stream = &ostAudio;

      pkt->stream_index = stream->stream->index;
      av_packet_rescale_ts(pkt, stream->cc->time_base, stream->stream->time_base);
      if (InterleaveFrame)
      {
         ret = av_interleaved_write_frame(Container, pkt);
      }
      else
      {
         pkt->pts = AV_NOPTS_VALUE;
         pkt->dts = AV_NOPTS_VALUE;
         ret = av_write_frame(Container, pkt);
      }
      encoderMutex.unlock();
      av_packet_unref(pkt);
   }
   return true;
}
