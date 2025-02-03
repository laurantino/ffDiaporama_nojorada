#include "ffTools.h"

AVPacket* getNewPacket()
{
   AVPacket* p;
#ifdef IS_FFMPEG_440
   p = av_packet_alloc();
#else
   p = new AVPacket();
   av_init_packet(p);
#endif
   return p;
}

void releasePacketData(AVPacket*p)
{
   AV_FREE_PACKET(p);
}

void deletePacket(AVPacket**pp)
{
#ifdef IS_FFMPEG_440
   av_packet_free(pp);
#else
   releasePacketData(*pp);
   delete* pp;
   *pp = NULL;
#endif
   * pp = NULL;
}
