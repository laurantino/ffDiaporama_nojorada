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

#include "DlgEditMusic.h"
#include "ui_DlgEditMusic.h"
#include <QAudioFormat>

#define IconPause   QIcon(":/img/player_play.png")
#define IconPlay    QIcon(":/img/player_pause.png")

#define BUFFERING_NBR_FRAME                 10                                      // Number of frame wanted in the playing buffer
#define BUFFERING_NBR_FRAME_MIN             5
#define BUFFERING_NBR_AUDIO_FRAME           2
#define AUDIOBUFSIZE                        128000

#define NO_DEBUGMESSAGES

DlgEditMusic::DlgEditMusic(cMusicObject *TheMusicObject,cApplicationConfig *ApplicationConfig,QWidget *parent):
    QCustomDialog(ApplicationConfig,parent), ui(new Ui::DlgEditMusic) {
    ui->setupUi(this);

    OkBt                =ui->OKBT;
    CancelBt            =ui->CancelBt;
    HelpBt              =ui->HelpBt;
    HelpFile            ="0124";
    MusicObject         =TheMusicObject;
    StopMaj             =false;
    InSliderMoveEvent   =false;
    IsInit              =false;
    PlayerPlayMode      =false;
    PlayerPauseMode     =false;
    IsSliderProcess     =false;
    ActualPosition      =-1;
    ResetPositionWanted =false;

    AudioBuf            =(u_int8_t *)malloc(AUDIOBUFSIZE);
    AudioBufSize        =0;
    dPreviewFPS = ApplicationConfig->PreviewFPS;
    MixedMusic.SetFPS(double(1000)/dPreviewFPS,2,ApplicationConfig->PreviewSamplingRate,AV_SAMPLE_FMT_S16);
    Music.SetFPS(MixedMusic.WantedDuration,MixedMusic.Channels,MixedMusic.SamplingRate,MixedMusic.SampleFormat);

    setAudioOutput();
    //// Set up the format
    //QAudioFormat format;
    //format.setCodec("audio/pcm");
    //format.setSampleRate(MixedMusic.SamplingRate); // Usually this is specified through an UI option
    //format.setChannelCount(MixedMusic.Channels);
    //format.setSampleSize(16);
    //format.setByteOrder(QAudioFormat::LittleEndian);
    //format.setSampleType(QAudioFormat::SignedInt);

    //// Create audio output stream, set up signals
    //m_audioOutput = new QAudioOutput(format,this);
    //qDebug() << "notifyInterval is " << m_audioOutput->notifyInterval();
    //qDebug() << "periodSize is " << m_audioOutput->periodSize();
    //qDebug() << "bytes per frame is " << format.bytesPerFrame();
    connect( m_audioOutput.get(), SIGNAL(stateChanged(QAudio::State)), this, SLOT(s_stateChanged(QAudio::State)) );
#ifdef DEBUGMESSAGES
    qDebug() << "audio-output period-size is " << m_audioOutput->periodSize();
    qDebug() << "audio-output noitfy interval " << m_audioOutput->notifyInterval();
#endif
    connect( m_audioOutput.get(), SIGNAL(notify()), this, SLOT(notify()) );
}

//====================================================================================================================

DlgEditMusic::~DlgEditMusic() {
    if (ThreadAnalyseMusic.isRunning()) {
        ui->CustomRuler->IsAnalysed=true;
        ThreadAnalyseMusic.waitForFinished();
    }
    SetPlayerToPause();
    if (m_audioOutput->state()==QAudio::SuspendedState) {
        m_audioOutput->reset();
        m_audioOutput->stop();
    }
    if (m_audioOutput->state()!=QAudio::StoppedState) 
	   m_audioOutput->stop();
    audio_outputDevice=NULL;
    AudioBufSize=0;
    //delete m_audioOutput;
    //m_audioOutput=NULL;
    delete ui;
    free(AudioBuf);
}

//====================================================================================================================

void DlgEditMusic::setAudioOutput()
{
   // Set up the format
   QAudioFormat format;
#if QT_VERSION >= 0x060000
   m_devices = new QMediaDevices(this);

   //format.setCodec("audio/pcm");
   format.setSampleRate(MixedMusic.SamplingRate); // Usually this is specified through an UI option
   format.setChannelCount(MixedMusic.Channels);
   format.setSampleFormat(QAudioFormat::Int16);
   //format.setSampleSize(16);
   //format.setByteOrder(QAudioFormat::LittleEndian);
   //format.setSampleType(QAudioFormat::SignedInt);

   const QAudioDevice& defaultDeviceInfo = m_devices->defaultAudioOutput();
   //QAudioDevice deviceInfo = m_devices->defaultAudioOutput();
   m_audioOutput.reset(new QAudioSink(defaultDeviceInfo, format));
   m_audioOutput->setBufferSize(MixedMusic.NbrPacketForFPS * MixedMusic.SoundPacketSize * BUFFERING_NBR_AUDIO_FRAME);
   m_audioOutput->start();
   m_audioOutput->suspend();

#else
   if (!m_audioOutput.isNull())
      m_audioOutput->stop();
   //delete audio_outputStream;

   format.setCodec("audio/pcm");
   format.setSampleRate(MixedMusic.SamplingRate); // Usually this is specified through an UI option
   format.setChannelCount(MixedMusic.Channels);
   format.setSampleSize(16);
   format.setByteOrder(QAudioFormat::LittleEndian);
   format.setSampleType(QAudioFormat::SignedInt);

   // Create audio output stream, set up signals
   m_audioOutput.reset(new QAudioOutput(format, this));
   m_audioOutput->setBufferSize(MixedMusic.NbrPacketForFPS * MixedMusic.SoundPacketSize * BUFFERING_NBR_AUDIO_FRAME);
   audio_outputDevice = m_audioOutput->start();
   m_audioOutput->suspend();
#endif
}

void DlgEditMusic::resizeEvent(QResizeEvent *ev) {
    QCustomDialog::resizeEvent(ev);
    if (IsInit && ui->CustomRuler->IsAnalysed) {
        ui->CustomRuler->PrepareSoundWave();
        ui->CustomRuler->repaint();
    }
}

void DlgEditMusic::showEvent(QShowEvent *) {
    if (!IsInit) {
        PlayerPlayMode =true;
        PlayerPauseMode=true;
        IsInit         =true;
        // Start sound analyse
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        connect(&ThreadAnalyseMusic,SIGNAL(finished()),this,SLOT(s_EndSoundAnalyse()));
        ThreadAnalyseMusic.setFuture(QT_CONCURRENT_RUN(this,&DlgEditMusic::s_AnalyseSound));
        Timer.start(500);   // to update display on windows
        QApplication::restoreOverrideCursor();
    }
}

// Do sound analyse
void DlgEditMusic::s_AnalyseSound() {
    ui->CustomRuler->AnalyseSound(MusicObject);
}

// End of sound analyse
void DlgEditMusic::s_EndSoundAnalyse() {
    ui->CustomRuler->setEnabled(true);
    ui->DefStartPosBT->setEnabled(true);
    ui->DefEndPosBT->setEnabled(true);
    ui->SeekLeftBt->setEnabled(true);
    ui->SeekRightBt->setEnabled(true);
    ui->StartPosEd->setEnabled(true);
    ui->EndPosEd->setEnabled(true);
    ui->VideoPlayerPlayPauseBT->setEnabled(true);
    Timer.stop();
    RefreshControls();
    // Activate player
    SetPlayerToPlay();
    QApplication::restoreOverrideCursor();
}

//====================================================================================================================

void DlgEditMusic::DoInitDialog() {
    ui->SeekLeftBt->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->SeekRightBt->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaSkipForward));
    ui->StartPosEd->setCurrentSection(QDateTimeEdit::MSecSection);  ui->StartPosEd->setCurrentSectionIndex(3);  ui->StartPosEd->MsecStep=1;//MusicObject->GetFPSDuration();
    ui->EndPosEd->setCurrentSection(QDateTimeEdit::MSecSection);    ui->EndPosEd->setCurrentSectionIndex(3);    ui->EndPosEd->MsecStep  =1;//MusicObject->GetFPSDuration();

    Music.SetFPS(double(1000)/dPreviewFPS,2,ApplicationConfig->PreviewSamplingRate,AV_SAMPLE_FMT_S16);

    ui->CustomRuler->EditStartEnd =true;
    ui->CustomRuler->setSingleStep(25);

    // Disable all during sound analyse
    ui->CustomRuler->setEnabled(false);
    ui->DefStartPosBT->setEnabled(false);
    ui->DefEndPosBT->setEnabled(false);
    ui->SeekLeftBt->setEnabled(false);
    ui->SeekRightBt->setEnabled(false);
    ui->StartPosEd->setEnabled(false);
    ui->EndPosEd->setEnabled(false);
    ui->VideoPlayerPlayPauseBT->setEnabled(false);

    SetCurrentPos(MusicObject->StartTime);
    RefreshControls();

    connect(&Timer,SIGNAL(timeout()),this,SLOT(s_TimerEvent()));
    connect(&PlayTimer,SIGNAL(timeout()),this,SLOT(soundToDevice()));
    connect(ui->VideoPlayerPlayPauseBT,SIGNAL(clicked()),this,SLOT(s_VideoPlayerPlayPauseBT()));

    // Slider controls
    connect(ui->CustomRuler,SIGNAL(sliderPressed()),this,SLOT(s_SliderPressed()));
    connect(ui->CustomRuler,SIGNAL(sliderReleased()),this,SLOT(s_SliderReleased()));
    connect(ui->CustomRuler,SIGNAL(valueChanged(int)),this,SLOT(s_SliderMoved(int)));
    connect(ui->CustomRuler,SIGNAL(PositionChangeByUser()),this,SLOT(s_PositionChangeByUser()));
    connect(ui->CustomRuler,SIGNAL(StartEndChangeByUser()),this,SLOT(s_StartEndChangeByUser()));
    connect(ui->CustomRuler,SIGNAL(StartEndChangeByUser()),this,SLOT(s_StartEndChangeByUser()));

    // Edit controls
    connect(ui->DefStartPosBT,SIGNAL(clicked()),this,SLOT(s_DefStartPos()));
    connect(ui->DefEndPosBT,SIGNAL(clicked()),this,SLOT(s_DefEndPos()));
    connect(ui->SeekLeftBt,SIGNAL(clicked()),this,SLOT(s_SeekLeft()));
    connect(ui->SeekRightBt,SIGNAL(clicked()),this,SLOT(s_SeekRight()));
    connect(ui->StartPosEd,SIGNAL(timeChanged(QTime)),this,SLOT(s_EditStartPos(QTime)));
    connect(ui->EndPosEd,SIGNAL(timeChanged(QTime)),this,SLOT(s_EditEndPos(QTime)));

    m_audioOutput->setBufferSize(MixedMusic.NbrPacketForFPS*MixedMusic.SoundPacketSize*BUFFERING_NBR_AUDIO_FRAME);
    audio_outputDevice=m_audioOutput->start();
#ifdef DEBUGMESSAGES
    qDebug() << "audio-output period-size is " << m_audioOutput->periodSize();
#endif
    #if QT_VERSION >= 0x050000
    m_audioOutput->setVolume(ApplicationConfig->PreviewSoundVolume);
    #endif
    m_audioOutput->suspend();
}

//====================================================================================================================

void DlgEditMusic::RefreshControls() {
    if (StopMaj) return;
    StopMaj=true;
    ui->EndPosEd->setMaximumTime(MusicObject->GetRealDuration());
    if (MusicObject->EndTime >= MusicObject->GetRealDuration()) 
      MusicObject->EndTime = MusicObject->GetRealDuration();

    ui->Duration->setText(MusicObject->GetRealDuration().toString("hh:mm:ss.zzz"));
    ui->CustomRuler->setMaximum(QTime(0,0,0,0).msecsTo(MusicObject->GetRealDuration()));

    ui->CustomRuler->StartPos     = QTime(0,0,0,0).msecsTo(MusicObject->StartTime);
    ui->CustomRuler->EndPos       = QTime(0,0,0,0).msecsTo(MusicObject->EndTime);
    ui->CustomRuler->TotalDuration= QTime(0,0,0,0).msecsTo(MusicObject->GetRealDuration());
    QTime Duration=QTime(0,0,0,0).addMSecs(MusicObject->StartTime.msecsTo(MusicObject->EndTime));
    ui->ActualDuration->setText(Duration.toString("hh:mm:ss.zzz"));
    ui->StartPosEd->setMaximumTime(MusicObject->EndTime);    ui->StartPosEd->setTime(MusicObject->StartTime);
    ui->EndPosEd->setMinimumTime(MusicObject->StartTime);    ui->EndPosEd->setTime(MusicObject->EndTime);
    StopMaj=false;
}

//====================================================================================================================
// Initiale Undo

void DlgEditMusic::PrepareGlobalUndo() {
    // Save object before modification for cancel button
    Undo=new QDomDocument(APPLICATION_NAME);
    QDomElement root=Undo->createElement("UNDO-DLG");       // Create xml document and root
    MusicObject->SaveToXML(&root,"UNDO-DLG-OBJECT","",true,NULL,NULL,false);  // Save object
    Undo->appendChild(root);                                // Add object to xml document
}

//====================================================================================================================
// Apply Undo : call when user click on Cancel button

void DlgEditMusic::DoGlobalUndo() {
    if (ThreadAnalyseMusic.isRunning()) {
        ui->CustomRuler->IsAnalysed=true;
        ThreadAnalyseMusic.waitForFinished();
    }
    SetPlayerToPause();
    QDomElement root=Undo->documentElement();
    if (root.tagName()=="UNDO-DLG") MusicObject->LoadFromXML(&root,"UNDO-DLG-OBJECT","",NULL,NULL);
}

//====================================================================================================================

void DlgEditMusic::WinFocus() {
    SetPlayerToPlay();
}

void DlgEditMusic::LostFocus() {
    SetPlayerToPause();
}

//============================================================================================

void DlgEditMusic::s_PositionChangeByUser() 
{
#ifdef DEBUGMESSAGES
    qDebug() << "s_PositionChangeByUser";
#endif
    ResetPositionWanted=true;
}

//====================================================================================================================

void DlgEditMusic::s_StartEndChangeByUser() {
    if (StopMaj) return;
    MusicObject->StartTime = QTime(0,0,0,0).addMSecs(ui->CustomRuler->StartPos);
    MusicObject->EndTime = QTime(0,0,0,0).addMSecs(ui->CustomRuler->EndPos);
    ui->StartPosEd->setTime(MusicObject->StartTime);
    ui->EndPosEd->setTime(MusicObject->EndTime);
}

//============================================================================================
// Seek slider public function
//============================================================================================

void DlgEditMusic::SeekPlayer(int Value) {
#ifdef DEBUGMESSAGES
    qDebug() << "SeekPlayer " << Value;
#endif
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    ActualPosition=-1;
    s_SliderMoved(Value);
    QApplication::restoreOverrideCursor();
}

//====================================================================================================================

void DlgEditMusic::s_DefStartPos() {
    if (StopMaj) return;
    MusicObject->StartTime = GetCurrentPos();
    ui->CustomRuler->StartPos = QTime(0,0,0,0).msecsTo(MusicObject->StartTime);
    RefreshControls();
}

//====================================================================================================================

void DlgEditMusic::s_EditStartPos(QTime NewValue) {
    if (StopMaj) return;
    MusicObject->StartTime = NewValue;
    SetCurrentPos(MusicObject->StartTime);
    RefreshControls();
}

//====================================================================================================================

void DlgEditMusic::s_DefEndPos() {
    if (StopMaj) return;
    MusicObject->EndTime = GetCurrentPos();
    if (MusicObject->EndTime >= MusicObject->GetRealDuration()) 
      MusicObject->EndTime = MusicObject->GetRealDuration().addMSecs(-1);
    ui->CustomRuler->EndPos=QTime(0,0,0,0).msecsTo(MusicObject->EndTime);
    RefreshControls();
}

//====================================================================================================================

void DlgEditMusic::s_EditEndPos(QTime NewValue) {
    if (StopMaj) return;
    MusicObject->EndTime = NewValue;
    ui->EndPosEd->setTime(MusicObject->EndTime);
    SetCurrentPos(MusicObject->EndTime);
    RefreshControls();
}

//====================================================================================================================

void DlgEditMusic::s_SeekLeft() {
    SeekPlayer(QTime(0,0,0,0).msecsTo(MusicObject->StartTime));
    SetPlayerToPause();
}

//====================================================================================================================

void DlgEditMusic::s_SeekRight() {
    SeekPlayer(QTime(0,0,0,0).msecsTo(MusicObject->EndTime));
    SetPlayerToPause();
}

//============================================================================================
// Pause -> play
//============================================================================================

void DlgEditMusic::SetPlayerToPlay() {
    if (!(PlayerPlayMode && PlayerPauseMode)) return;
    PlayerPlayMode  = true;
    PlayerPauseMode = false;
    ui->VideoPlayerPlayPauseBT->setIcon(IconPlay);

    // Start timer
    PlayerMutex.lock();
    TimerTick           =true;
    PreviousTimerEvent  =QTime();
    TimerDelta          =0;
    switch (m_audioOutput->state()) {
        case QAudio::SuspendedState: m_audioOutput->resume();
                                     break;
        case QAudio::StoppedState:   audio_outputDevice=m_audioOutput->start();
                                     #if QT_VERSION >= 0x050000
                                     m_audioOutput->setVolume(ApplicationConfig->PreviewSoundVolume);
                                     #endif
#ifdef DEBUGMESSAGES
                                       qDebug() << "audio-output period-size is " << m_audioOutput->periodSize();
#endif
                                     break;
        case QAudio::ActiveState:    /*qDebug()<<"ActiveState";*/                           break;
        case QAudio::IdleState:      /*qDebug()<<"IdleState";*/                             break;
    }
    Timer.start(int(double(1000)/dPreviewFPS)/2);   // Start Timer
    PlayTimer.start(20);
    PlayerMutex.unlock();
}

//============================================================================================
// Play -> pause
//============================================================================================

void DlgEditMusic::SetPlayerToPause() 
{
   if (!(PlayerPlayMode && !PlayerPauseMode)) 
      return;
   Timer.stop();      
   PlayTimer.stop();                             // Stop Timer
   PlayerMutex.lock();
   Mutex.lock();
   if (ThreadPrepareMusic.isRunning()) ThreadPrepareMusic.waitForFinished();
   if (m_audioOutput->state()!=QAudio::SuspendedState) 
   {
      m_audioOutput->suspend();
      //m_audioOutput->reset();
      AudioBufSize=0;
   }
   m_audioOutput->reset();
   MixedMusic.ClearList();                         // Free sound buffers
   Music.ClearList();                              // Free sound buffers
   FrameList.ClearList();                          // Free FrameList
   PlayerPlayMode  = true;
   PlayerPauseMode = true;
   ui->VideoPlayerPlayPauseBT->setIcon(IconPause);
   ui->BufferState->setValue(0);
   Mutex.unlock();
   PlayerMutex.unlock();
}

//============================================================================================
// Click on the play/pause button
//============================================================================================

void DlgEditMusic::s_VideoPlayerPlayPauseBT() 
{
   if ((!PlayerPlayMode)||((PlayerPlayMode && PlayerPauseMode)))    
       SetPlayerToPlay();      // Stop/Pause -> play
   else if (PlayerPlayMode && !PlayerPauseMode)                 
      SetPlayerToPause();     // Pause -> play
}

//============================================================================================
// return current position in QTime format
//============================================================================================

QTime DlgEditMusic::GetCurrentPos() 
{
    if (ActualPosition!=-1) 
    {
        int     TimeMSec    =ActualPosition-(ActualPosition/1000)*1000;
        int     TimeSec     =int(ActualPosition/1000);
        int     TimeHour    =TimeSec/(60*60);
        int     TimeMinute  =(TimeSec%(60*60))/60;
        QTime   tPosition;
        tPosition.setHMS(TimeHour,TimeMinute,TimeSec%60,TimeMSec);
        return tPosition;
    } 
    else 
       return QTime(0,0,0,0);
}

//============================================================================================
// Set current position in QTime format
// 
//============================================================================================

void DlgEditMusic::SetCurrentPos(QTime Pos) 
{
#ifdef DEBUGMESSAGES
    qDebug() << "SetCurrentPos " << Pos;
#endif
    SeekPlayer(QTime(0,0,0,0).msecsTo(Pos));
}

//============================================================================================
// Click on the handle of the slider
//============================================================================================

void DlgEditMusic::s_SliderPressed() 
{
#ifdef DEBUGMESSAGES
    qDebug() << "s_SliderPressed ";
#endif
    PreviousPause    = PlayerPauseMode;    // Save pause state
    IsSliderProcess  = true;
    SetPlayerToPause();
}

//============================================================================================
// En slider process
//============================================================================================

void DlgEditMusic::s_SliderReleased() {
#ifdef DEBUGMESSAGES
    qDebug() << "s_SliderReleased ";
#endif
    IsSliderProcess  = false;
    s_SliderMoved(ActualPosition);
    // Restore saved pause state
    if (!PreviousPause) SetPlayerToPlay();
}

//============================================================================================
// Slider is moving by user
// Slider is moving by timer
// Slider is moving by seek function
//============================================================================================

void DlgEditMusic::s_SliderMoved(int Value) 
{
#ifdef DEBUGMESSAGES
    qDebug() << "s_SliderMoved " << Value << " ResetPositionWanted " << ResetPositionWanted;
#endif
    PreviousPause    = PlayerPauseMode;    // Save pause state
    if (ResetPositionWanted) 
      SetPlayerToPause();

    if (InSliderMoveEvent) return;
    InSliderMoveEvent=true;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Update display in controls
    ui->CustomRuler->setValue(Value);
    ActualPosition=Value;
    ui->Position->setText(GetCurrentPos().toString("hh:mm:ss.zzz"));

    // Audio
    if (m_audioOutput->state() == QAudio::ActiveState || m_audioOutput->state() == QAudio::IdleState) 
    {
        Mutex.lock();
        int len = AUDIOBUFSIZE-AudioBufSize;
        if( len > 0 )
        {
           //qDebug() << "music to buffer todo , AudioBufSize now " << AudioBufSize << " MixedMusic.ListCount() " << MixedMusic.ListCount();
           // put packets into buffer
           while( (AudioBufSize < AUDIOBUFSIZE-MixedMusic.SoundPacketSize) && (MixedMusic.ListCount()>0)) 
           {
                int16_t *Packet=MixedMusic.DetachFirstPacket(true);
                if (Packet!=NULL) 
                {
                    memcpy(AudioBuf+AudioBufSize,Packet,MixedMusic.SoundPacketSize);
                    AudioBufSize+=MixedMusic.SoundPacketSize;
                }
            }
            //qDebug() << "music to buffer done, AudioBufSize now " << AudioBufSize;
        }
        Mutex.unlock();
        /*
        int len=m_audioOutput->bytesFree();
#ifdef DEBUGMESSAGES
        qDebug() <<"audio output bytes free " << len;
#endif
        if (len>0) {
            Mutex.lock();
            while ((AudioBufSize<len)&&(MixedMusic.ListCount()>0)) {
                int16_t *Packet=MixedMusic.DetachFirstPacket(true);
                if (Packet!=NULL) {
                    memcpy(AudioBuf+AudioBufSize,Packet,MixedMusic.SoundPacketSize);
                    //qDebug() << "memcpy offset " << AudioBufSize << " size " << MixedMusic.SoundPacketSize;
                    AudioBufSize+=MixedMusic.SoundPacketSize;
                }
            }
#ifdef DEBUGMESSAGES
            qDebug() << AudioBufSize << " bytes in buffer";
#endif
            if ((len)&&(len<=AudioBufSize)) {
                int RealLen=audio_outputDevice->write((char *)AudioBuf,len);
//#ifdef DEBUGMESSAGES
                qDebug() << RealLen << " bytes of " << len << "written to audio-output, enough for " << m_audioOutput->format().durationForBytes(RealLen) / 1000 << "ms";
//#endif
                if (RealLen > 0 && RealLen<=AudioBufSize) {
                    memcpy(AudioBuf,AudioBuf+RealLen,AudioBufSize-RealLen);
                    //qDebug() << "memmove offset " << RealLen << " size " << AudioBufSize-RealLen;
                    AudioBufSize-=RealLen;
                }
#ifdef DEBUGMESSAGES
                qDebug() << "AudioBufSize now " << AudioBufSize;
#endif
            }
            Mutex.unlock();
            
        }*/
    }
    if (ResetPositionWanted) 
       if (!PreviousPause) SetPlayerToPlay();
    if (PlayerPlayMode && !PlayerPauseMode) {
        if (ActualPosition>=QTime(0,0,0,0).msecsTo(MusicObject->GetRealDuration())) {
            SetPlayerToPause(); // Stop if it's the end
        } else if (FrameList.count() > 1) {                        // Process
            cDiaporamaObjectInfo *Frame = FrameList.DetachFirstFrame();   // Retrieve frame information
            delete Frame;
        }
    }

    QApplication::restoreOverrideCursor();
    InSliderMoveEvent=false;
}

//============================================================================================
// Timer event
//============================================================================================

void DlgEditMusic::s_TimerEvent() {
    // on windows, repaint can't be send by thread, so use a timer for that
    if (!ui->CustomRuler->IsAnalysed) {
        ui->CustomRuler->repaint();
        return;
    }
    if (IsSliderProcess)                            return;     // No re-entrance
    if (!(PlayerPlayMode && !PlayerPauseMode))      return;     // Only if play mode

    TimerTick=!TimerTick;

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
                ToLog(LOGMSG_DEBUGTRACE,"FPS preview is too high: One frame lost");
                if (FrameList.List.count()>0) {
                    delete (cDiaporamaObjectInfo *)FrameList.DetachFirstFrame(); // Remove first frame if we loose one tick
                } else {
                    // Increase next position to one frame
                    ActualPosition+=Wanted;
                }
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

    if (ThreadPrepareMusic.isRunning()) 
      ThreadPrepareMusic.waitForFinished();

    int64_t LastPosition=0,NextPosition=0;

    if (ResetPositionWanted) 
    {
        Mutex.lock();
        MixedMusic.ClearList();                         // Free sound buffers
        Music.ClearList();                              // Free sound buffers
        FrameList.ClearList();                          // Free FrameList
        ResetPositionWanted=false;
        Mutex.unlock();
    }

    // If no frame in the list then create the first
    if (FrameList.count()==0) 
    {
        cDiaporamaObjectInfo *NewFrame=new cDiaporamaObjectInfo(NULL,ActualPosition,NULL,double(1000)/dPreviewFPS,true);
        NewFrame->Current.Object_StartTime   =0;
        PrepareSoundFrame(NewFrame,NewFrame->Current.Object_InObjectTime);

        if (m_audioOutput->state()==QAudio::IdleState) {
            int len=m_audioOutput->bytesFree();
            if (len==m_audioOutput->bufferSize()) {
                memset(AudioBuf,0,AUDIOBUFSIZE);
                //AudioBufSize=len-MixedMusic.NbrPacketForFPS*MixedMusic.SoundPacketSize;
                //AudioBufSize=AUDIOBUFSIZE-MixedMusic.NbrPacketForFPS*MixedMusic.SoundPacketSize;
                AudioBufSize=0;
#ifdef DEBUGMESSAGES
                qDebug() << "set audiobuffer to 0, AudioBufSize is now " << AudioBufSize;
#endif
            }
        }
    }

    cDiaporamaObjectInfo *PreviousFrame = FrameList.GetLastFrame();
    LastPosition=PreviousFrame?PreviousFrame->Current.Object_InObjectTime:0;
    NextPosition=LastPosition+int(double(1000)/dPreviewFPS);

    // Add frame to the list if it's not full
    if ((FrameList.count()<BUFFERING_NBR_FRAME)&&(!ThreadPrepareMusic.isRunning())) 
    {
        cDiaporamaObjectInfo *NewFrame=new cDiaporamaObjectInfo(PreviousFrame,NextPosition,NULL,int(double(1000)/dPreviewFPS),true);
        NewFrame->Current.Object_StartTime   =0;
        ThreadPrepareMusic.setFuture(QT_CONCURRENT_RUN_2(this,&DlgEditMusic::PrepareSoundFrame,NewFrame,NewFrame->Current.Object_InObjectTime));
    }
    PlayerMutex.unlock();

    if ((TimerTick)&&(FrameList.count()>BUFFERING_NBR_FRAME_MIN)) 
    //if (FrameList.List.count()>BUFFERING_NBR_FRAME_MIN) 
      s_SliderMoved(FrameList.GetFirstFrame()->Current.Object_StartTime + FrameList.GetFirstFrame()->Current.Object_InObjectTime);

    ui->BufferState->setValue(FrameList.count());
    if (FrameList.count() < 2)
        ui->BufferState->setStyleSheet("QProgressBar:horizontal {\nborder: 0px;\nborder-radius: 0px;\nbackground: black;\npadding-top: 0px;\npadding-bottom: 0px;\npadding-left: 0px;\npadding-right: 0px;\n}\nQProgressBar::chunk:horizontal {\nbackground: red;\n}");
    else if (FrameList.count() < 4)
        ui->BufferState->setStyleSheet("QProgressBar:horizontal {\nborder: 0px;\nborder-radius: 0px;\nbackground: black;\npadding-top: 0px;\npadding-bottom: 0px;\npadding-left: 0px;\npadding-right: 0px;\n}\nQProgressBar::chunk:horizontal {\nbackground: yellow;\n}");
    else if (FrameList.count() <= BUFFERING_NBR_FRAME)
        ui->BufferState->setStyleSheet("QProgressBar:horizontal {\nborder: 0px;\nborder-radius: 0px;\nbackground: black;\npadding-top: 0px;\npadding-bottom: 0px;\npadding-left: 0px;\npadding-right: 0px;\n}\nQProgressBar::chunk:horizontal {\nbackground: green;\n}");
}

void DlgEditMusic::PrepareSoundFrame(cDiaporamaObjectInfo *NewFrame,int Position) {
    MusicObject->ReadFrame(true,Position*1000,true,false,&Music,1,true);
#ifdef DEBUGMESSAGES
    qDebug() << "Music.NbrPacketForFPS " << Music.NbrPacketForFPS << " Music.CurrentPosition " << Music.CurrentPosition;
#endif
    for (int j=0;j<Music.NbrPacketForFPS;j++) 
      MixedMusic.AppendPacket(Music.CurrentPosition,Music.DetachFirstPacket());
    FrameList.AppendFrame(NewFrame);
}

void DlgEditMusic::s_stateChanged(QAudio::State state)
{
   Q_UNUSED(state)
#ifdef DEBUGMESSAGES
   qDebug() << "audi o state changed to " << state << " err is " << m_audioOutput->error();
#endif
}

void DlgEditMusic::notify()
{
#ifdef DEBUGMESSAGES
   qDebug() << "notify, bytes free = " << m_audioOutput->bytesFree();
#endif
}

void DlgEditMusic::soundToDevice()
{
   Mutex.lock();
   if( AudioBufSize > 0 )
   {
      int len = m_audioOutput->bytesFree();
      if( len > 0 )
      {
         if( len > AudioBufSize )
            len = AudioBufSize;
         int RealLen = audio_outputDevice->write((char *)AudioBuf,len);
#ifdef DEBUGMESSAGES
         qDebug() << RealLen << " bytes written to audio-output (len was " << len << ", AudioBufSize was " << AudioBufSize <<")";
#endif
         if (RealLen > 0 ) 
         {
            // shift the rest of the buffer in front
            memcpy(AudioBuf,AudioBuf+RealLen,AudioBufSize-RealLen);
            AudioBufSize -= RealLen;
         }
      }
   }
   Mutex.unlock();
}
