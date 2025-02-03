#ifndef __FFTOOLS__
#define __FFTOOLS__
extern "C" {
   #include "libavformat/version.h"
   #include "libavcodec/packet.h"
   #include "libavutil/opt.h"
}
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(58, 9, 100)
#define IS_FFMPEG_414
#endif
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(58, 76, 100)
#define IS_FFMPEG_440
#endif

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
#define CODEC_OR_PAR codec
#define CODEC_SAMPLE_FORMAT sample_fmt
#define CODEC_PIX_FORMAT format
#define AV_FREE_PACKET av_free_packet
#else
#define CODEC_OR_PAR codecpar
#define CODEC_SAMPLE_FORMAT format
#define CODEC_PIX_FORMAT format
#define AV_FREE_PACKET av_packet_unref
#endif


#define CC_CHANNELS ch_layout.nb_channels
AVPacket* getNewPacket();
void releasePacketData(AVPacket*);
void deletePacket(AVPacket**);
#endif // __FFTOOLS__
