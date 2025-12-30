#include "utils/owned.hpp"

namespace mvplayer::deallocator {
void AVFrameDeallocator(AVFrame* frame) { av_frame_free(&frame); }
void AVPacketDeallocator(AVPacket* packet) { av_packet_free(&packet); }
}  // namespace mvplayer::deallocator
