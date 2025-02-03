/* ======================================================================
    This file is part of ffDiaporama
    ffDiaporama is a tools to make diaporama as video
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

#include "wgt_QVideoPlayer.h"
#include "MainWindow/mainwindow.h"
#include "ui_wgt_QVideoPlayer.h"
#include <QtMultimedia/QAudioFormat>

#define ICON_PLAYERPLAY                     ":/img/player_play.png"                 // FileName of play icon
#define ICON_PLAYERPAUSE                    ":/img/player_pause.png"                // FileName of pause icon

#define BUFFERING_NBR_FRAME                 10                                      // Number of frame wanted in the playing buffer
#define BUFFERING_NBR_FRAME_MIN             5
#define BUFFERING_NBR_AUDIO_FRAME           2
#define AUDIOBUFSIZE                        128000
static int ctimerticks = 0;

//====================================================================================================================

cVideoPlayer::cVideoPlayer(QWidget *parent) : QWidget(parent),ui(new Ui::wgt_QVideoPlayer) 
{
   ui->setupUi(this);
   AudioBuf                = (u_int8_t *)malloc(AUDIOBUFSIZE);
   AudioBufSize            = 0;

   FLAGSTOPITEMSELECTION   = NULL;
   InPlayerUpdate          = false;
   Diaporama               = NULL;
   IsValide                = false;
   IsInit                  = false;
   DisplayMSec             = true;                                 // add msec to display
   IconPause               = QIcon(ICON_PLAYERPLAY);               // QApplication::style()->standardIcon(QStyle::SP_MediaPlay)
   IconPlay                = QIcon(ICON_PLAYERPAUSE);              // QApplication::style()->standardIcon(QStyle::SP_MediaPause)
   PlayerPlayMode          = false;                                // Is player currently play mode
   PlayerPauseMode         = false;                                // Is player currently plause mode
   IsSliderProcess         = false;
   ActualPosition          = -1;
   tDuration               = QTime(0,0,0,0);
   ResetPositionWanted     = false;
   fixValue = false;
   //Deinterlace             = false;
   AudioPlayed             = 0;

   ui->CustomRuler->ActiveSlider(0);
   //ui->CustomRuler->setSingleStep(25);
   ui->CustomRuler->setSingleStep(40);
   ui->CustomRuler->setPageStep(1000);

   ui->MovieFrame->setText("");
   ui->MovieFrame->setAttribute(Qt::WA_OpaquePaintEvent);

#if QT_VERSION >= 0x050000
   ui->VideoPlayerVolumeBT->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
   ui->VideoPlayerVolumeBT->setPopupMode(QToolButton::InstantPopup);
   connect(ui->VideoPlayerVolumeBT,SIGNAL(pressed()),this,SLOT(s_VideoPlayerVolume()));
#else
   ui->VideoPlayerVolumeBT->setVisible(false);
#endif

   connect(&Timer,SIGNAL(timeout()),this,SLOT(s_TimerEvent()));
   connect(ui->VideoPlayerPlayPauseBT,SIGNAL(clicked()),this,SLOT(s_VideoPlayerPlayPauseBT()));
   connect(ui->MovieFrame,SIGNAL(DoubleClick()),this,SLOT(s_DoubleClick()));
   connect(ui->MovieFrame,SIGNAL(RightClickEvent(QMouseEvent *)),this,SLOT(s_RightClickEvent(QMouseEvent *)));

   // Slider control
   connect(ui->CustomRuler,SIGNAL(sliderPressed()),this,SLOT(s_SliderPressed()));
   connect(ui->CustomRuler,SIGNAL(sliderReleased()),this,SLOT(s_SliderReleased()));
   connect(ui->CustomRuler,SIGNAL(valueChanged(int)),this,SLOT(s_SliderMoved(int)));
   connect(ui->CustomRuler,SIGNAL(PositionChangeByUser()),this,SLOT(s_PositionChangeByUser()));
   connect(ui->CustomRuler,SIGNAL(StartEndChangeByUser()),this,SLOT(s_StartEndChangeByUser()));
   connect(ui->VideoPlayerSaveImageBT,SIGNAL(pressed()),this,SLOT(s_SaveImage()));

   MixedMusic.SetFPS(double(1000)/12.5,2,48000,AV_SAMPLE_FMT_S16);
   Music.SetFPS(MixedMusic.WantedDuration,MixedMusic.Channels,MixedMusic.SamplingRate,MixedMusic.SampleFormat);

   // Set up the format
   QAudioFormat format;
   format.setCodec("audio/pcm");
   format.setSampleRate(MixedMusic.SamplingRate); // Usually this is specified through an UI option
   format.setChannelCount(MixedMusic.Channels);
   format.setSampleSize(16);
   format.setByteOrder(QAudioFormat::LittleEndian);
   format.setSampleType(QAudioFormat::SignedInt);

   // Create audio output stream, set up signals
   audio_outputStream=new QAudioOutput(format,this);
   audio_outputStream->setBufferSize(MixedMusic.NbrPacketForFPS*MixedMusic.SoundPacketSize*BUFFERING_NBR_AUDIO_FRAME);
   audio_outputDevice=audio_outputStream->start();
   AudioPlayed=0;
   audio_outputStream->suspend();
}

//============================================================================================

cVideoPlayer::~cVideoPlayer() 
{
   SetPlayerToPause();         // Ensure player is correctly stoped
   if (audio_outputStream->state() == QAudio::SuspendedState) 
   {
      audio_outputStream->reset();
      audio_outputStream->stop();
   }
   if (audio_outputStream->state() != QAudio::StoppedState) 
      audio_outputStream->stop();
   audio_outputDevice = NULL;
   AudioBufSize = 0;
   delete audio_outputStream;
   audio_outputStream = NULL;
   delete ui;
   free(AudioBuf);
}

//============================================================================================

void cVideoPlayer::closeEvent(QCloseEvent *) 
{
   SetPlayerToPause();
}

//====================================================================================================================

void cVideoPlayer::showEvent(QShowEvent *) 
{
   if (!IsInit && Diaporama == NULL) 
   {
      SetPlayerToPlay();
      IsInit = true;
   }
}

//============================================================================================

void cVideoPlayer::SetEditStartEnd(bool State) 
{
   ui->CustomRuler->EditStartEnd = State;
}

//============================================================================================

//void cVideoPlayer::s_SaveImage() 
//{
//   emit SaveImageEvent();
//}

//============================================================================================

void cVideoPlayer::s_VideoPlayerVolume() 
{
   QWidget *popup = new QWidget(this);
   VolumeSlider = new QSlider(Qt::Horizontal,popup);
   VolumeSlider->setRange(0,100);
   VolumeSlider->setValue(ApplicationConfig->PreviewSoundVolume*100);
   connect(VolumeSlider,SIGNAL(valueChanged(int)),this,SLOT(s_VolumeChanged(int)));

   VolumeLabel = new QLabel(popup);
   VolumeLabel->setAlignment(Qt::AlignCenter);
   VolumeLabel->setNum(ApplicationConfig->PreviewSoundVolume*100);
   VolumeLabel->setMinimumWidth(VolumeLabel->sizeHint().width());
   connect(VolumeSlider,SIGNAL(valueChanged(int)),VolumeLabel,SLOT(setNum(int)));

   QBoxLayout *popupLayout = new QHBoxLayout(popup);
   popupLayout->setMargin(2);
   popupLayout->addWidget(VolumeSlider);
   popupLayout->addWidget(VolumeLabel);

   QWidgetAction *action = new QWidgetAction(this);
   action->setDefaultWidget(popup);

   QMenu *menu = new QMenu(this);
   menu->addAction(action);
   menu->exec(QCursor::pos());
   delete menu;
}

void cVideoPlayer::s_VolumeChanged(int newValue) 
{
#if QT_VERSION >= 0x050000
   audio_outputStream->setVolume(qreal(newValue)/100);
   VolumeLabel->setNum(newValue);
   ApplicationConfig->PreviewSoundVolume = qreal(newValue)/100;
   emit VolumeChanged();
#endif
}

//============================================================================================

//void cVideoPlayer::s_DoubleClick() 
//{
//   emit DoubleClick();
//}

//============================================================================================

//void cVideoPlayer::s_RightClickEvent(QMouseEvent *event) 
//{
//   emit RightClickEvent(event);
//}
//
//============================================================================================

int cVideoPlayer::GetButtonBarHeight() 
{
   return ui->VideoPlayerPlayPauseBT->height();
}

//============================================================================================

void cVideoPlayer::SetBackgroundColor(QColor Background) 
{
   QString Sheet = QString("background-color: rgb(%1,%2,%3);").arg(Background.red(),10).arg(Background.green(),10).arg(Background.blue(),10);
}

//============================================================================================

void cVideoPlayer::EnableWidget(bool State) 
{
   if (ui->CustomRuler != NULL) 
      ui->CustomRuler->setEnabled(State);
}

//============================================================================================

void cVideoPlayer::SetAudioFPS() 
{
   SetPlayerToPause();
   MixedMusic.ClearList();    // Free sound buffers
   Music.ClearList();
   MixedMusic.SetFPS(double(1000)/ApplicationConfig->PreviewFPS,2,ApplicationConfig->PreviewSamplingRate,AV_SAMPLE_FMT_S16);
   Music.SetFPS(MixedMusic.WantedDuration,MixedMusic.Channels,MixedMusic.SamplingRate,MixedMusic.SampleFormat);
   audio_outputStream->stop();
   audio_outputStream->setBufferSize(MixedMusic.NbrPacketForFPS*MixedMusic.SoundPacketSize*BUFFERING_NBR_AUDIO_FRAME);
   audio_outputDevice = audio_outputStream->start();
   AudioPlayed = 0;
#if QT_VERSION >= 0x050000
   audio_outputStream->setVolume(ApplicationConfig->PreviewSoundVolume);
#endif
}

//============================================================================================
// Init a diaporama show
//============================================================================================
bool cVideoPlayer::InitDiaporamaPlay(cDiaporama *Diaporama) 
{
   if (Diaporama == NULL) 
      return false;
   ApplicationConfig = Diaporama->ApplicationConfig();
   this->Diaporama   = Diaporama;

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   ui->CustomRuler->ActiveSlider(Diaporama->GetDuration());
   PlayerPlayMode  = true;
   PlayerPauseMode = true;
   ui->VideoPlayerPlayPauseBT->setIcon(IconPause);
   SetAudioFPS();
   QApplication::restoreOverrideCursor();
   return true;
}

//============================================================================================
// Pause -> play
//============================================================================================

void cVideoPlayer::SwitchPlayerPlayPause() 
{
   s_VideoPlayerPlayPauseBT();
}

//============================================================================================
// Pause -> play
//============================================================================================

void cVideoPlayer::SetPlayerToPlay() 
{
   if (!(PlayerPlayMode && PlayerPauseMode)) 
      return;
   emit playingStarts();
   PlayerPlayMode  = true;
   PlayerPauseMode = false;
   ui->VideoPlayerPlayPauseBT->setIcon(IconPlay);

   //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF );
   // Start timer
   PlayerMutex.lock();
   TimerTick           = true;
   PreviousTimerEvent  = QTime();
   TimerDelta          = 0;
   switch (audio_outputStream->state()) 
   {
      case QAudio::SuspendedState: 
         audio_outputStream->resume();
         break;
      case QAudio::StoppedState:   
         audio_outputDevice = audio_outputStream->start();
         AudioPlayed = 0;
   #if QT_VERSION >= 0x050000
         audio_outputStream->setVolume(ApplicationConfig->PreviewSoundVolume);
   #endif
         break;
      case QAudio::ActiveState:    
         //qDebug()<<"ActiveState";                           
         break;
      case QAudio::IdleState:      
         //qDebug()<<"IdleState";                             
         break;
   }
   ctimerticks = 0;
   Timer.start(int(double(1000)/ApplicationConfig->PreviewFPS)/2);   // Start Timer
   PlayerMutex.unlock();
}

//============================================================================================
// Play -> pause
//============================================================================================

void cVideoPlayer::SetPlayerToPause() 
{
   if (!(PlayerPlayMode && !PlayerPauseMode)) 
      return;
   emit playingStops();
   PlayerMutex.lock();
   Timer.stop();                                   // Stop Timer
   //if (ThreadPrepareVideo.isRunning()) 
   //   ThreadPrepareVideo.waitForFinished();
   if (ThreadPrepareImage.isRunning()) 
      ThreadPrepareImage.waitForFinished();
   if (ThreadAssembly.isRunning())     
      ThreadAssembly.waitForFinished();

   if (audio_outputStream->state() != QAudio::StoppedState) 
   {
      audio_outputStream->stop();
      audio_outputStream->reset();
      AudioBufSize = 0;
   }
   MixedMusic.ClearList();                         // Free sound buffers
   Music.ClearList();                              // Free sound buffers
   FrameList.ClearList();                          // Free FrameList
   PlayerPlayMode  = true;
   PlayerPauseMode = true;
   ui->VideoPlayerPlayPauseBT->setIcon(IconPause);
   ui->BufferState->setValue(0);
   PlayerMutex.unlock();
   //_CrtSetDbgFlag(0);
}

//============================================================================================
// Click on the play/pause button
//============================================================================================

void cVideoPlayer::s_VideoPlayerPlayPauseBT() 
{
   if (!PlayerPlayMode || (PlayerPlayMode && PlayerPauseMode))    
      SetPlayerToPlay();      // Stop/Pause -> play
   else if (PlayerPlayMode && !PlayerPauseMode)                 
      SetPlayerToPause();     // Pause -> play
}

//============================================================================================
// Click on the handle of the slider
//============================================================================================

void cVideoPlayer::s_SliderPressed() 
{
   PreviousPause    = PlayerPauseMode;    // Save pause state
   IsSliderProcess  = true;
   SetPlayerToPause();
}

//============================================================================================
// En slider process
//============================================================================================

void cVideoPlayer::s_SliderReleased() 
{
   IsSliderProcess  = false;
   s_SliderMoved(ActualPosition);
   // Restore saved pause state
   if (!PreviousPause) 
      SetPlayerToPlay();
}

//============================================================================================
// Slider is moving by user
// Slider is moving by timer
// Slider is moving by seek function
//============================================================================================

void cVideoPlayer::s_SliderMoved(int Value) 
{
   if (InPlayerUpdate) 
      return;
   InPlayerUpdate = true;

   if( fixValue )
   {
      Value = Value/40 * 40;
      fixValue = false;
   }
   if (ResetPositionWanted || Value>ui->CustomRuler->maximum()) 
   {
      //Value = Value/40 * 40;
      SetPlayerToPause();
   }

   // Update display in controls
   ui->CustomRuler->setValue(Value);
   ActualPosition = Value;
   ui->Position->setText(GetCurrentPos().toString(DisplayMSec?"hh:mm:ss.zzz":"hh:mm:ss"));
   ui->Duration->setText(tDuration.toString(DisplayMSec?"hh:mm:ss.zzz":"hh:mm:ss"));

   //***********************************************************************
   // If playing
   //***********************************************************************
   if (PlayerPlayMode && !PlayerPauseMode) 
   {
      if (FrameList.count() > 1) // Process
      {                        
         // Retrieve frame information
         cDiaporamaObjectInfo *Frame = FrameList.DetachFirstFrame();

         // Display frame
         if (!Frame->RenderedImage.isNull()) 
            ui->MovieFrame->SetImage(Frame->RenderedImage.scaledToHeight(ui->MovieFrame->height()));

         // If Diaporama mode and needed, set Diaporama to another object
         if (Diaporama) 
         {
            if (Diaporama->CurrentCol != Frame->Current.Object_Number) 
            {
               Diaporama->CurrentCol = Frame->Current.Object_Number;
               ((MainWindow *)ApplicationConfig->TopLevelWindow)->SetTimelineCurrentCell(Frame->Current.Object_Number);

               // Update slider mark
               if (Diaporama->slides.count() > 0)
                  SetStartEndPos(
                     Diaporama->GetObjectStartPosition(Diaporama->CurrentCol),                                                           // Current slide
                     Diaporama->slides[Diaporama->CurrentCol]->GetDuration(),
                     (Diaporama->CurrentCol > 0) ? Diaporama->GetObjectStartPosition(Diaporama->CurrentCol-1) : ((Diaporama->CurrentCol == 0) ? 0 : -1),                            // Previous slide
                     (Diaporama->CurrentCol > 0) ? Diaporama->slides[Diaporama->CurrentCol-1]->GetDuration() : ((Diaporama->CurrentCol == 0) ? Diaporama->GetTransitionDuration(Diaporama->CurrentCol) : 0),
                     Diaporama->CurrentCol<(Diaporama->slides.count()-1) ? Diaporama->GetObjectStartPosition(Diaporama->CurrentCol+1) : -1,    // Next slide
                     Diaporama->CurrentCol<(Diaporama->slides.count()-1) ? Diaporama->slides[Diaporama->CurrentCol+1]->GetDuration() : 0);
               else 
                  SetStartEndPos(0,0,-1,0,-1,0);
            }
            Diaporama->CurrentPosition=Value;
         }
         // Free frame
         delete Frame;
      }

      //***********************************************************************
      // If moving by user
      //***********************************************************************
   } 
   else if (Diaporama) 
   {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      //if (ThreadPrepareVideo.isRunning()) 
      //   ThreadPrepareVideo.waitForFinished();
      if (ThreadPrepareImage.isRunning()) 
         ThreadPrepareImage.waitForFinished();
      if (ThreadAssembly.isRunning())
         ThreadAssembly.waitForFinished();

      // Create a frame from actual position
      cDiaporamaObjectInfo *Frame = new cDiaporamaObjectInfo(NULL,ActualPosition,Diaporama,double(1000)/ApplicationConfig->PreviewFPS,false);

      int H = ui->MovieFrame->height();
      int W = Diaporama->GetWidthForHeight(H);
      if (W > ui->MovieFrame->width()) 
      {
         W = ui->MovieFrame->width();
         H = Diaporama->GetHeightForWidth(W);
      }
      if (Frame->IsTransition && Frame->Transit.Object) 
         Diaporama->CreateObjectContextList(Frame,QSize(W,H),false,true,true,PreparedTransitBrushList);
      Diaporama->CreateObjectContextList(Frame,QSize(W,H),true,true,true,PreparedBrushList);
      PrepareImage(false,true,Frame,W,H);         // This will add frame to the FrameList

      if (ThreadAssembly.isRunning()) 
         ThreadAssembly.waitForFinished();

      Frame = FrameList.DetachFirstFrame();     // Then detach frame from the FrameList

      // Display frame
      ui->MovieFrame->SetImage(Frame->RenderedImage.scaledToHeight(ui->MovieFrame->height()));

      // If needed, set Diaporama to another object
      if ((Diaporama->CurrentCol != Frame->Current.Object_Number) && (!Frame->IsTransition || Diaporama->CurrentCol != Frame->Transit.Object_Number)) 
      {
         if (FLAGSTOPITEMSELECTION!=NULL) 
            *FLAGSTOPITEMSELECTION = true;    // Ensure mainwindow no modify player widget position
         Diaporama->CurrentCol = Frame->Current.Object_Number;
         if (FLAGSTOPITEMSELECTION != NULL) 
            *FLAGSTOPITEMSELECTION = false;
         ((MainWindow *)ApplicationConfig->TopLevelWindow)->SetTimelineCurrentCell(Frame->Current.Object_Number);

         // Update slider mark
         if (Diaporama->slides.count() > 0)
            SetStartEndPos(
               Diaporama->GetObjectStartPosition(Diaporama->CurrentCol),                                                               // Current slide
               Diaporama->slides[Diaporama->CurrentCol]->GetDuration(),
               (Diaporama->CurrentCol > 0) ? Diaporama->GetObjectStartPosition(Diaporama->CurrentCol-1) : ((Diaporama->CurrentCol == 0) ? 0 : -1), // Previous slide
               (Diaporama->CurrentCol > 0) ? Diaporama->slides[Diaporama->CurrentCol-1]->GetDuration() : ((Diaporama->CurrentCol == 0) ? Diaporama->GetTransitionDuration(Diaporama->CurrentCol) : 0),
               Diaporama->CurrentCol < (Diaporama->slides.count()-1) ? Diaporama->GetObjectStartPosition(Diaporama->CurrentCol+1) : -1,        // Next slide
               Diaporama->CurrentCol < (Diaporama->slides.count()-1) ? Diaporama->slides[Diaporama->CurrentCol+1]->GetDuration() : 0);

      }
      Diaporama->CurrentPosition = Value;

      // Free frame
      delete Frame;
      QApplication::restoreOverrideCursor();
   }
   InPlayerUpdate=false;
}

//============================================================================================
// Timer event
//============================================================================================
void cVideoPlayer::s_TimerEvent() 
{
//   static bool inTimer = false;
   if (IsSliderProcess)
      return;     // No re-entrance
   if (!(PlayerPlayMode && !PlayerPauseMode))
      return;     // Only if play mode
   //qDebug() << "timertick " << ++timerticks << " inTimer " << inTimer;
   //if(inTimer)
   //   return;
   //inTimer = true;
   TimerTick = !TimerTick;

#ifdef Q_OS_WIN
   // Trylock is always true on Windows instead of unix/linux system
   if (TimerTick) {
#else
   if (!PlayerMutex.tryLock()) { if (!TimerTick) return; else {
#endif
      /*
      // specific case for windows because never a timer event can happens if a previous timer event was not ended
      // so next trylock is always true
      int Elapsed=0,Wanted=int(double(1000)/ApplicationConfig->PreviewFPS);
      if (!PreviousTimerEvent.isValid()) PreviousTimerEvent.start(); else Elapsed=PreviousTimerEvent.restart();
      if (Elapsed>Wanted) {
      TimerDelta+=Elapsed-Wanted;
      if (TimerDelta>=Wanted) {
      ToLog(LOGMSG_DEBUGTRACE,"FPS preview is too high: One image lost");
      if (FrameList.List.count()>0)   delete (cDiaporamaObjectInfo *)FrameList.DetachFirstFrame(); // Remove first image if we loose one tick
      else                        Diaporama->CurrentPosition+=Wanted; // Increase next position to one frame
      TimerDelta-=Wanted;
      }
      }
      */
   }
#ifdef Q_OS_WIN
   PlayerMutex.lock();
#else
   return;}
#endif

   //if (ThreadPrepareVideo.isRunning()) 
   //   ThreadPrepareVideo.waitForFinished();
   if (ThreadPrepareImage.isRunning()) 
      ThreadPrepareImage.waitForFinished();
   if (ThreadAssembly.isRunning())
      ThreadAssembly.waitForFinished();

   int64_t LastPosition = 0, NextPosition = 0;

   if (ResetPositionWanted) 
   {
      Mutex.lock();
      MixedMusic.ClearList();                         // Free sound buffers
      Music.ClearList();                              // Free sound buffers
      FrameList.ClearList();                          // Free FrameList
      ResetPositionWanted=false;
      Mutex.unlock();
      Diaporama->resetSoundBlocks();
   }

   if (ThreadAssembly.isRunning())
      ThreadAssembly.waitForFinished();

   // If no image in the list then create the first
   if (FrameList.count() == 0) 
   {
      LastPosition = Diaporama->CurrentPosition;
      NextPosition = LastPosition+int(double(1000)/ApplicationConfig->PreviewFPS);

      // If no image in the list then prepare a first frame
      cDiaporamaObjectInfo *Frame=new cDiaporamaObjectInfo(NULL,NextPosition,Diaporama,double(1000)/ApplicationConfig->PreviewFPS,true);

      // Ensure AudioTracks are ready
      Frame->ensureAudioTracksReady(double(1000) / double(ApplicationConfig->PreviewFPS), 2, Diaporama->ApplicationConfig()->PreviewSamplingRate, AV_SAMPLE_FMT_S16);
      //// Ensure MusicTracks are ready
      //if (Frame->Current.Object && Frame->Current.Object_MusicTrack == NULL) 
      //{
      //   Frame->Current.Object_MusicTrack=new cSoundBlockList();
      //   Frame->Current.Object_MusicTrack->SetFPS(double(1000)/double(ApplicationConfig->PreviewFPS),2,Diaporama->ApplicationConfig()->PreviewSamplingRate,AV_SAMPLE_FMT_S16);
      //}
      //if (Frame->Transit.Object && Frame->Transit.Object_MusicTrack == NULL && Frame->Transit.Object_MusicObject != NULL && Frame->Transit.Object_MusicObject != Frame->Current.Object_MusicObject) 
      //{
      //   Frame->Transit.Object_MusicTrack=new cSoundBlockList();
      //   Frame->Transit.Object_MusicTrack->SetFPS(double(1000)/double(ApplicationConfig->PreviewFPS),2,Diaporama->ApplicationConfig()->PreviewSamplingRate,AV_SAMPLE_FMT_S16);
      //}

      //// Ensure SoundTracks are ready
      //if (Frame->Current.Object && Frame->Current.Object_SoundTrackMontage == NULL) 
      //{
      //   Frame->Current.Object_SoundTrackMontage=new cSoundBlockList();
      //   Frame->Current.Object_SoundTrackMontage->SetFPS(double(1000)/double(ApplicationConfig->PreviewFPS),2,Diaporama->ApplicationConfig()->PreviewSamplingRate,AV_SAMPLE_FMT_S16);
      //}
      //if (Frame->Transit.Object && Frame->Transit.Object_SoundTrackMontage == NULL) 
      //{
      //   Frame->Transit.Object_SoundTrackMontage=new cSoundBlockList();
      //   Frame->Transit.Object_SoundTrackMontage->SetFPS(double(1000)/double(ApplicationConfig->PreviewFPS),2,Diaporama->ApplicationConfig()->PreviewSamplingRate,AV_SAMPLE_FMT_S16);
      //}

      // Ensure background, image and soundtrack is ready
      int H = ui->MovieFrame->height();
      int W = Diaporama->GetWidthForHeight(H);
      if (W > ui->MovieFrame->width()) 
      {
         W = ui->MovieFrame->width();
         H = Diaporama->GetHeightForWidth(W);
      }
      if (Frame->IsTransition && Frame->Transit.Object) 
         Diaporama->CreateObjectContextList(Frame,QSize(W,H),false,true,true,PreparedTransitBrushList);
      Diaporama->CreateObjectContextList(Frame,QSize(W,H),true,true,true,PreparedBrushList);

      if (audio_outputStream->state() == QAudio::IdleState) 
      {
         int len = audio_outputStream->bytesFree();
         if (len > 0 && len==audio_outputStream->bufferSize()) 
         {
            memset(AudioBuf,0,AUDIOBUFSIZE);
            AudioBufSize=len-MixedMusic.NbrPacketForFPS*MixedMusic.SoundPacketSize;
         }
      }

      PrepareImage(true,true,Frame,W,H);
      if (ThreadAssembly.isRunning())
         ThreadAssembly.waitForFinished();
   }

   //Mutex.lock();
   // Add image to the list if it's not full
   if ( (Diaporama && FrameList.count() < BUFFERING_NBR_FRAME) && !ThreadPrepareImage.isRunning())  
   {
      cDiaporamaObjectInfo *PreviousFrame = FrameList.GetLastFrame();
      if (Diaporama) 
         LastPosition = PreviousFrame->Current.Object_StartTime + PreviousFrame->Current.Object_InObjectTime;
      NextPosition = LastPosition+int(double(1000)/ApplicationConfig->PreviewFPS);

      cDiaporamaObjectInfo *Frame = new cDiaporamaObjectInfo(PreviousFrame,NextPosition,Diaporama,double(1000)/ApplicationConfig->PreviewFPS,true);

      // Ensure AudioTracks are ready
      Frame->ensureAudioTracksReady(double(1000) / double(ApplicationConfig->PreviewFPS), 2, Diaporama->ApplicationConfig()->PreviewSamplingRate, AV_SAMPLE_FMT_S16);
      //// Ensure MusicTracks are ready
      //if (Frame->Current.Object && Frame->Current.Object_MusicTrack == NULL) 
      //{
      //   Frame->Current.Object_MusicTrack = new cSoundBlockList();
      //   Frame->Current.Object_MusicTrack->SetFPS(double(1000)/double(ApplicationConfig->PreviewFPS),2,Diaporama->ApplicationConfig()->PreviewSamplingRate,AV_SAMPLE_FMT_S16);
      //}
      //if (Frame->Transit.Object && Frame->Transit.Object_MusicTrack == NULL && Frame->Transit.Object_MusicObject != NULL && Frame->Transit.Object_MusicObject != Frame->Current.Object_MusicObject) 
      //{
      //   Frame->Transit.Object_MusicTrack = new cSoundBlockList();
      //   Frame->Transit.Object_MusicTrack->SetFPS(double(1000)/double(ApplicationConfig->PreviewFPS),2,Diaporama->ApplicationConfig()->PreviewSamplingRate,AV_SAMPLE_FMT_S16);
      //}

      //// Ensure SoundTracks are ready
      //if (Frame->Current.Object && Frame->Current.Object_SoundTrackMontage == NULL) 
      //{
      //   Frame->Current.Object_SoundTrackMontage = new cSoundBlockList();
      //   Frame->Current.Object_SoundTrackMontage->SetFPS(double(1000)/double(ApplicationConfig->PreviewFPS),2,Diaporama->ApplicationConfig()->PreviewSamplingRate,AV_SAMPLE_FMT_S16);
      //}
      //if (Frame->Transit.Object && Frame->Transit.Object_SoundTrackMontage == NULL) 
      //{
      //   Frame->Transit.Object_SoundTrackMontage = new cSoundBlockList();
      //   Frame->Transit.Object_SoundTrackMontage->SetFPS(double(1000)/double(ApplicationConfig->PreviewFPS),2,Diaporama->ApplicationConfig()->PreviewSamplingRate,AV_SAMPLE_FMT_S16);
      //}

      // Ensure background, image and soundtrack is ready
      int H = ui->MovieFrame->height();
      int W = Diaporama->GetWidthForHeight(H);
      if (W > ui->MovieFrame->width()) 
      {
         W = ui->MovieFrame->width();
         H = Diaporama->GetHeightForWidth(W);
      }
      if (Frame->IsTransition && Frame->Transit.Object ) 
         Diaporama->CreateObjectContextList(Frame,QSize(W,H),false,true,true,PreparedTransitBrushList);
      Diaporama->CreateObjectContextList(Frame,QSize(W,H),true,true,true,PreparedBrushList);
      ThreadPrepareImage.setFuture(QT_CONCURRENT_RUN_5(this,&cVideoPlayer::PrepareImage,true,true,Frame,W,H));
      //PrepareImage(true,true,Frame,W,H);
   }

   PlayerMutex.unlock();
   // Audio
   //qDebug() << "audio_outputStream->state() " << audio_outputStream->state() << " audio_outputStream->bytesFree() " << audio_outputStream->bytesFree() << " AudioBufSize " << AudioBufSize << " MixedMusic.ListCount() " <<MixedMusic.ListCount();
   if ( audio_outputStream->state() == QAudio::ActiveState || audio_outputStream->state() == QAudio::IdleState ) 
   {
      int len = audio_outputStream->bytesFree();
      if (len > 0) 
      {
         //qDebug() << "try play music, inTimer is " << inTimer << " AudioBufSize " << AudioBufSize << " len " <<len;
         Mutex.lock();
         //while( !Mutex.tryLock(1));
         while( AudioBufSize < len && MixedMusic.ListCount() > 0) 
         {
            //qDebug() << "take mixedMusic from position " << MixedMusic.CurrentPosition;
            int16_t *Packet = MixedMusic.DetachFirstPacket(true);
            if (Packet != NULL) 
            {
               memcpy(AudioBuf+AudioBufSize,Packet,MixedMusic.SoundPacketSize);
               AudioBufSize += MixedMusic.SoundPacketSize;
            } 
            else 
            {
               memset(AudioBuf+AudioBufSize,0,MixedMusic.SoundPacketSize);
               AudioBufSize += MixedMusic.SoundPacketSize;
            }
         }
         if (len > AudioBufSize) 
            len = AudioBufSize;
         if (len > 0) 
         {
            int RealLen = audio_outputDevice->write((char *)AudioBuf,len);
            //qDebug() << "len " << len << " RealLen " << RealLen << " AudioBufSize " << AudioBufSize;
            if (RealLen > 0 && RealLen <= AudioBufSize) 
            {
               memcpy(AudioBuf,AudioBuf+RealLen,AudioBufSize-RealLen);
               AudioBufSize -= RealLen;
            }
            if( RealLen > 0 ) 
               AudioPlayed += RealLen;
         }
         Mutex.unlock();

         // Preview is updated if sound played correspond to 1 FPS
         if (AudioPlayed >= MixedMusic.NbrPacketForFPS*MixedMusic.SoundPacketSize) 
         {
            s_SliderMoved(FrameList.GetFirstFrame()->Current.Object_StartTime + FrameList.GetFirstFrame()->Current.Object_InObjectTime);
            AudioPlayed -= MixedMusic.NbrPacketForFPS*MixedMusic.SoundPacketSize;
         }
      }
   }
   //Mutex.unlock();

   ui->BufferState->setValue(FrameList.count());
   if (FrameList.count() < 2)
      ui->BufferState->setStyleSheet("QProgressBar:horizontal {\nborder: 0px;\nborder-radius: 0px;\nbackground: black;\npadding-top: 1px;\npadding-bottom: 2px;\npadding-left: 1px;\npadding-right: 1px;\n}\nQProgressBar::chunk:horizontal {\nbackground: red;\n}");
   else if (FrameList.count() < 4)
      ui->BufferState->setStyleSheet("QProgressBar:horizontal {\nborder: 0px;\nborder-radius: 0px;\nbackground: black;\npadding-top: 1px;\npadding-bottom: 2px;\npadding-left: 1px;\npadding-right: 1px;\n}\nQProgressBar::chunk:horizontal {\nbackground: yellow;\n}");
   else if (FrameList.count() <= BUFFERING_NBR_FRAME)
      ui->BufferState->setStyleSheet("QProgressBar:horizontal {\nborder: 0px;\nborder-radius: 0px;\nbackground: black;\npadding-top: 1px;\npadding-bottom: 2px;\npadding-left: 1px;\npadding-right: 1px;\n}\nQProgressBar::chunk:horizontal {\nbackground: green;\n}");
//   inTimer = false;
}

//============================================================================================
// Function use directly or with thread to prepare an image number Column at given position
//============================================================================================

void cVideoPlayer::PrepareImage(bool SoundWanted,bool AddStartPos,cDiaporamaObjectInfo *Frame,int W,int H) 
{
  Diaporama->LoadSources(Frame,QSize(W,H),true,AddStartPos,PreparedTransitBrushList,PreparedBrushList);
   // Do Assembly
   ThreadAssembly.setFuture(QT_CONCURRENT_RUN_5(this,&cVideoPlayer::StartThreadAssembly,ComputePCT(Frame->Current.Object?Frame->Current.Object->GetSpeedWave():0,Frame->TransitionPCTDone),Frame,W,H,SoundWanted));
}

void cVideoPlayer::StartThreadAssembly(double PCT,cDiaporamaObjectInfo *Frame,int W,int H,bool SoundWanted) 
{
   Diaporama->DoAssembly(PCT,Frame,QSize(W,H));
   Mutex.lock();
   // Append mixed musique to the queue
   if (SoundWanted && Frame->Current.Object) 
   {
      for (int j = 0; j < Frame->Current.Object_MusicTrack->NbrPacketForFPS; j++) 
      {
         int16_t *Music = ((Frame->IsTransition && Frame->Transit.Object && !Frame->Transit.Object->MusicPause)
            || (!Frame->Current.Object->MusicPause)) ? Frame->Current.Object_MusicTrack->DetachFirstPacket() : NULL;
         int16_t *Sound = (Frame->Current.Object_SoundTrackMontage != NULL) ? Frame->Current.Object_SoundTrackMontage->DetachFirstPacket() : NULL;
         MixedMusic.MixAppendPacket((Frame->Current.Object_StartTime + Frame->Current.Object_InObjectTime)*1000, Music, Sound);
      }
      if (FrameList.count() == 0) 
         MixedMusic.AdjustSoundPosition((Frame->Current.Object_StartTime + Frame->Current.Object_InObjectTime)*1000);
   }

   // Append this image to the queue
   FrameList.AppendFrame(Frame);
   Mutex.unlock();
}

//============================================================================================
// Define zone selection on the ruller
//============================================================================================

void cVideoPlayer::SetStartEndPos(int StartPos,int Duration,int PreviousStartPos,int PrevisousDuration,int NextStartPos,int NextDuration) 
{
   ui->CustomRuler->StartPos          = StartPos;
   ui->CustomRuler->EndPos            = StartPos + Duration;
   ui->CustomRuler->PreviousStartPos  = PreviousStartPos;
   ui->CustomRuler->PrevisousEndPos   = PreviousStartPos + PrevisousDuration;
   ui->CustomRuler->NextStartPos      = NextStartPos;
   ui->CustomRuler->NextEndPos        = NextStartPos + NextDuration;
   ui->CustomRuler->repaint();
}

//============================================================================================
// Seek slider public function
//============================================================================================

void cVideoPlayer::SeekPlayer(int Value) 
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   ActualPosition = -1;
   s_SliderMoved(Value);
   QApplication::restoreOverrideCursor();
}

//============================================================================================
// return current position in QTime format
//============================================================================================

QTime cVideoPlayer::GetCurrentPos() 
{
   if (ActualPosition != -1) 
   {
      int   TimeMSec   = ActualPosition - (ActualPosition/1000)*1000;
      int   TimeSec    = int(ActualPosition/1000);
      int   TimeHour   = TimeSec/(60*60);
      int   TimeMinute = (TimeSec%(60*60))/60;
      QTime tPosition;
      tPosition.setHMS(TimeHour,TimeMinute,TimeSec%60,TimeMSec);
      return tPosition;
   } 
   return QTime(0,0,0,0);
}

//============================================================================================
// Set current position in QTime format
//============================================================================================

void cVideoPlayer::SetCurrentPos(QTime Pos) 
{
   SeekPlayer(QTime(0,0,0,0).msecsTo(Pos));
}

//============================================================================================
// return current duration in QTime format
//============================================================================================

QTime cVideoPlayer::GetActualDuration() 
{
   return tDuration;
}

//============================================================================================
// define duration (in msec)
//============================================================================================

void cVideoPlayer::SetActualDuration(int Duration) 
{
   if (ui->CustomRuler != NULL) 
   {
      ui->CustomRuler->setMaximum(Duration-1);
      //ui->CustomRuler->repaint();
      ui->CustomRuler->TotalDuration = Duration;
   }
   //ui->CustomRuler->repaint();
   int TimeMSec   = (Duration %1000);
   int TimeSec    = int(Duration/1000);
   int TimeHour   = TimeSec/(60*60);
   int TimeMinute = (TimeSec%(60*60))/60;
   tDuration.setHMS(TimeHour,TimeMinute,TimeSec%60,TimeMSec);
   ui->Position->setText(GetCurrentPos().toString(DisplayMSec ? "hh:mm:ss.zzz" : "hh:mm:ss"));
   ui->Duration->setText(tDuration.toString(DisplayMSec ? "hh:mm:ss.zzz" : "hh:mm:ss"));
}

//============================================================================================

void cVideoPlayer::s_PositionChangeByUser() 
{
   ResetPositionWanted = true;
   fixValue = true;
}

void cVideoPlayer::s_StartEndChangeByUser() 
{
   StartPos = QTime(0,0,0,0).addMSecs(ui->CustomRuler->StartPos);
   EndPos = QTime(0,0,0,0).addMSecs(ui->CustomRuler->EndPos);
   emit StartEndChangeByUser();
}

