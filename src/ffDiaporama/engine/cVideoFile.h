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

#ifndef CVIDEOFILE_H
#define CVIDEOFILE_H

#include "cBaseMediaFile.h"

#ifdef IS_FFMPEG_414
//*********************************************************************************************************************************************
// Video file
//*********************************************************************************************************************************************
// extern int MAXCACHEIMAGE; !! not used

class cImageInCache 
{
   public:
      int64_t     Position;
      AVFrame     *FiltFrame,*FrameBufferYUV;
      cImageInCache(int64_t Position,AVFrame *FiltFrame,AVFrame *FrameBufferYUV);
      ~cImageInCache();
};

class cVideoFile : public cBaseMediaFile 
{
   protected:
      bool                    IsOpen;                   // True if Libav open on this file
      bool                    MusicOnly;                // True if object is a music only file
      bool                    IsVorbis;                 // True if vorbis version must be use instead of MP3/WAV version
      bool                    IsMTS;                    // True if file is a MTS file

      QString                 Container;                // Container type (get from file extension)
      QString                 VideoCodecInfo;           
      QString                 AudioCodecInfo;           
      int64_t                 LastAudioReadPosition;  // Use to keep the last read position to determine if a seek is needed
      //!! not used int64_t                 LastVideoReadPosition;  // Use to keep the last read position to determine if a seek is needed 
                                                        
      // Video part                                     
      int                     VideoStreamNumber;        // Number of the video stream
      QImage                  LastImage;                // Keep last image return
      QList<cImageInCache *>  CacheImage;
      QMap<int64_t, cImageInCache *> YUVCache;

      int                     SeekErrorCount;

      // Audio part
      int                     AudioStreamNumber;                  // Number of the audio stream

   public:
      QTime                   StartTime;                // Start position
      QTime                   EndTime;                  // End position
      int64_t                 WantedDuration() const
      {
         return StartTime.msecsTo(EndTime);
      }
      QTime                   AVStartTime;              // time from start_time information from the libavfile
      QTime                   AVEndTime;                // AVStartTime + RealDruation
      int64_t                 LibavStartTime;           // copy of the start_time information from the libavfile
      int                     VideoTrackNbr;            // Number of video stream in file
      int                     AudioTrackNbr;            // Number of audio stream in file
      int                     NbrChapters;              // Number of chapters in the file

      explicit                cVideoFile(cApplicationConfig *ApplicationConfig);
                              ~cVideoFile();
      virtual void            Reset(OBJECTTYPE TheWantedObjectType);

      virtual qreal           GetFPSDuration();
      virtual QString         GetFileTypeStr();
      virtual bool            LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes);
      virtual void            SaveBasicInformationToDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      virtual void            SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel) {}
      virtual bool            CheckFormatValide(QWidget *);
      virtual bool            GetChildFullInformationFromFile(bool IsPartial,cCustomIcon *Icon,QStringList *ExtendedProperties);
      virtual QImage          *GetDefaultTypeIcon(cCustomIcon::IconSize Size);

      virtual bool LoadAnalyseSound(QList<qreal> *Peak,QList<qreal> *Moyenne);
      virtual void SaveAnalyseSound(QList<qreal> *Peak,QList<qreal> *Moyenne,qreal MaxMoyenneValue);
                   

      virtual QString  GetTechInfo(QStringList *ExtendedProperties);
      virtual QString  GetTAGInfo(QStringList *ExtendedProperties);
      virtual bool     DoAnalyseSound(QList<qreal> *Peak,QList<qreal> *Moyenne,bool *IsAnalysed,volatile qreal *Analysed);

      virtual int      getThreadFlags(AVCodecID ID);

      virtual bool     OpenCodecAndFile();
      virtual void     CloseCodecAndFile();

      virtual QImage   *ImageAt(bool PreviewMode,int64_t Position,cSoundBlockList *SoundTrackMontage,bool Deinterlace,double Volume,bool ForceSoundOnly,bool DontUseEndPos,int NbrDuration=2);
      virtual AVFrame   *YUVAt(bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackMontage, bool Deinterlace, double Volume, bool ForceSoundOnly, bool DontUseEndPos, int NbrDuration = 2);
      virtual QImage   *ReadFrame(bool PreviewMode,int64_t Position,bool DontUseEndPos,bool Deinterlace,cSoundBlockList *SoundTrackBloc,double Volume,bool ForceSoundOnly,int NbrDuration=2);
      virtual AVFrame   *ReadYUVFrame(bool PreviewMode, int64_t Position, bool DontUseEndPos, bool Deinterlace, cSoundBlockList *SoundTrackBloc, double Volume, bool ForceSoundOnly, int NbrDuration = 2);
      virtual QImage   *ConvertYUVToRGB(bool PreviewMode,AVFrame *Frame);

      virtual bool     SeekFile(AVStream *VideoStream,AVStream *AudioStream,int64_t Position);
      virtual void     CloseResampler();
      virtual void     CheckResampler(int RSC_InChannels,int RSC_OutChannels,AVSampleFormat RSC_InSampleFmt,AVSampleFormat RSC_OutSampleFmt,int RSC_InSampleRate,int RSC_OutSampleRate
                                         #if (defined(LIBAV)&&(LIBAVVERSIONINT>=9)) || defined(FFMPEG)
                                            , AVChannelLayout& RSC_InChannelLayout, AVChannelLayout& RSC_OutChannelLayout
                                         #endif
                                   );
      virtual u_int8_t *Resample(AVFrame *Frame,int64_t *SizeDecoded,int DstSampleSize);

    //*********************
    // video filters part
    //*********************
    AVFilterGraph   *VideoFilterGraph;
    AVFilterContext *VideoFilterIn;
    AVFilterContext *VideoFilterOut;
    bool isCUVIDDecoder;
    bool useDeinterlace;

    virtual int   VideoFilter_Open();
    virtual void  VideoFilter_Close();
    virtual int   cudaFilter_Open();
    virtual void  cudaFilter_Close();

    virtual bool hasChapters()  const { return NbrChapters > 0; }
    virtual int numChapters() const { return NbrChapters; }

   protected:
      AVFormatContext         *LibavAudioFile,*LibavVideoFile;    // LibAVFormat contexts
      const AVCodec                 *VideoDecoderCodec;                 // Associated LibAVCodec for video stream
      AVCodecContext          *VideoCodecContext;
      const AVCodec                 *AudioDecoderCodec;                 // Associated LibAVCodec for audio stream
      AVCodecContext          *AudioCodecContext;
      AVFrame                 *FrameBufferYUV;
      bool                    FrameBufferYUVReady;        // true if FrameBufferYUV is ready to convert
      int64_t                 FrameBufferYUVPosition;     // If FrameBufferYUV is ready to convert then keep FrameBufferYUV position
      QImage *yuv2rgbImage;

      // for yuv2rgb-conversion
      AVFrame *FrameBufferRGB;
      struct SwsContext *img_convert_ctx;

      // Audio resampling
      SwrContext              *RSC;
      AVChannelLayout         RSC_InChannelLayout, RSC_OutChannelLayout;
      int                     RSC_InChannels,RSC_OutChannels;
      int                     RSC_InSampleRate,RSC_OutSampleRate;
      AVSampleFormat          RSC_InSampleFmt,RSC_OutSampleFmt;

   //private:
      struct sAudioContext {
         AVStream        *AudioStream;
         AVCodecContext  *CodecContext;
         cSoundBlockList *SoundTrackBloc;
         int64_t         FPSSize;
         int64_t         FPSDuration;
         int64_t         DstSampleSize;
         bool            DontUseEndPos;
         double          *dEndFile;
         int             NbrDuration;
         double          TimeBase;
         bool            NeedResampling;
         int64_t         AudioLenDecoded;
         int             Counter;
         double          AudioFramePosition;
         double          Volume;
         bool            ContinueAudio;
      };
   //private slots:
      void DecodeAudio(sAudioContext *AudioContext,AVPacket *StreamPacket,int64_t Position);
      void DecodeAudioFrame(sAudioContext *AudioContext,qreal *FramePts,AVFrame *Frame,int64_t Position);
};

//*********************************************************************************************************************************************
// Music file
//*********************************************************************************************************************************************
class cMusicObject : public cVideoFile 
{
   public:
      double  Volume;                 // Volume as % from 10% to 150%
      bool    AllowCredit;            // if true, this music will appear in credit title
      int64_t ForceFadIn;             // Fad-IN duration on sound part (or 0 if none)
      int64_t ForceFadOut;            // Fad-OUT duration on sound part (or 0 if none)
      int64_t startOffset;
      enum MusicStartCode { eDefault_Start, eFrameRelated_Start, eDirectStart };
      MusicStartCode startCode;
      cSoundBlockList SoundTrackBloc;

      cMusicObject(cApplicationConfig *ApplicationConfig);

      virtual bool CheckFormatValide(QWidget *);
      void  SaveToXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      virtual void SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel);
      bool  LoadFromXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag);
      QTime GetDuration();
      qreal GetFading(int64_t Position,bool SlideHaveFadIn,bool SlideHaveFadOut);
      virtual QImage *ReadFrame(bool PreviewMode,int64_t Position,bool DontUseEndPos,bool Deinterlace,cSoundBlockList *SoundTrackBloc,double Volume,bool ForceSoundOnly,int NbrDuration=2);
};

//class QMyImage : public QImage
//{
//   static int offset;
//   static bool offsetDetected;
//   static void detectOffset();
//   void setFormat(QImage::Format format) { *((int*)data_ptr()+offset) = format; }
//   public:
//   	QMyImage(const QImage & image) : QImage(image) {}
//      bool convert_ARGB_PM_to_ARGB_inplace();
//      bool convert_ARGB_to_ARGB_PM_inplace();
//      bool forcePremulFormat();
//};
#endif

#endif // CVIDEOFILE_H
