#include <qtToPhotoshopAPI.h>

void psApiRasterLayerInfo::setChannel(Enum::ChannelID channelID, std::vector<bpp8_t> channel) {
    if (width == -1 || height == -1) {
        throw std::runtime_error("setChannel(): must initialize width and height first with a non-zero value!");
    }

    if (channel.size() != width * height) {
        throw std::runtime_error("setChannel(): the channel passed must have a size equal to width * height!");
    }

    m_channelMap[channelID] = channel;
}

psApiData qtToPsApi (qtData qtInfo) {
    using PathPoint = ImageLayer<bpp8_t>::PathPoint;
    using SubPath = ImageLayer<bpp8_t>::SubPath;
    using Point2D = Geometry::Point2D<int>;
}
