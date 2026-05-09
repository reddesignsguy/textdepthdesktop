#ifndef QTTOPHOTOSHOPAPI_H
#define QTTOPHOTOSHOPAPI_H

#include <QPainterPath>
#include <QImage>
#include <vector>
#include <PhotoshopAPI.h>

using namespace NAMESPACE_PSAPI;

///////////// 1. These 3 templates are used as a bse for the qt and psApi variants of our text data. /////////////
///////////// There is some code duplication in the first 2 templates, but I don't think it's egregious /////////////
template <typename RASTER>
struct BackTextData {
    RASTER baseLayer;
    std::vector<RASTER> clippedLayers;
};

template <typename VECTOR, typename RASTER>
struct FrontTextData {
    VECTOR vectorMaskData;
    RASTER baseLayer;
    std::vector<RASTER> clippedLayers;
};

template <typename VECTOR, typename RASTER>
struct TextData {
    FrontTextData<VECTOR, RASTER> front;
    BackTextData<RASTER> back;
};

///////////// 2. structures prepared by QT application /////////////
using qtBackTextData = BackTextData<QImage>;
using qtFrontTextData = FrontTextData<std::vector<std::vector<QPainterPath::Element>>, QImage>;
using qtTextData = TextData<std::vector<std::vector<QPainterPath::Element>>, QImage>;
using qtData = std::vector<qtTextData>;

///////////// 3. structures consumed by PhotoshopAPI /////////////
struct psApiRasterLayerInfo {
    bpp8_t width = 0;
    bpp8_t height = 0;
    void setChannel(Enum::ChannelID channelID, std::vector<bpp8_t> channel);
private:
    std::unordered_map<Enum::ChannelID, std::vector<bpp8_t>> m_channelMap;
};
using psApiVectorMask = ImageLayer<bpp8_t>::VectorMask;
using psApiBackTextData = BackTextData<psApiRasterLayerInfo>;
using psApiFrontTextData = FrontTextData<psApiVectorMask, psApiRasterLayerInfo>;
using psApiTextData = TextData<psApiVectorMask, psApiRasterLayerInfo>;
using psApiData = std::vector<psApiTextData>;

///////////// 4. the bridge between our text in QT to PhotoshopAPI! /////////////
psApiData qtToPsApi (qtData qtInfo);

#endif // QTTOPHOTOSHOPAPI_H
