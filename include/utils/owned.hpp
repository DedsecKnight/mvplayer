#pragma once

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

namespace mvplayer {

namespace deallocator {
void AVFrameDeallocator(AVFrame* frame);
void AVPacketDeallocator(AVPacket* packet);
}  // namespace deallocator

#define OWNED_RESOURCE(RESOURCE)                                        \
  class Owned##RESOURCE                                                 \
      : public std::unique_ptr<                                         \
            RESOURCE, decltype(&deallocator::RESOURCE##Deallocator)> {  \
   private:                                                             \
    using BaseType =                                                    \
        std::unique_ptr<RESOURCE,                                       \
                        decltype(&deallocator::RESOURCE##Deallocator)>; \
                                                                        \
   public:                                                              \
    using BaseType::unique_ptr;                                         \
    [[nodiscard]] Owned##RESOURCE(RESOURCE* r)                          \
        : BaseType{r, deallocator::RESOURCE##Deallocator} {}            \
  };

OWNED_RESOURCE(AVFrame)
OWNED_RESOURCE(AVPacket)

}  // namespace mvplayer
