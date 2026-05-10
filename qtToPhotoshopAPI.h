#ifndef QTTOPHOTOSHOPAPI_H
#define QTTOPHOTOSHOPAPI_H

#include <QPainterPath>
#include <QImage>
#include <vector>
#include <PhotoshopAPI.h>

using namespace NAMESPACE_PSAPI;

///////////// 1. These 3 templates are used as a base for the qt and psApi variants of our text data. /////////////
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
using QtBackTextData = BackTextData<QImage>;
using QtFrontTextData = FrontTextData<std::vector<std::vector<QPainterPath::Element>>, QImage>;
using QtTextData = TextData<std::vector<std::vector<QPainterPath::Element>>, QImage>;
using QtData = std::vector<QtTextData>;

///////////// 3. structures consumed by PhotoshopAPI /////////////
struct PsApiRasterLayerInfo {
    bpp8_t width = 0;
    bpp8_t height = 0;
    void setChannel(Enum::ChannelID channelID, std::vector<bpp8_t> channel);
private:
    std::unordered_map<Enum::ChannelID, std::vector<bpp8_t>> m_channelMap;
};
using PsApiVectorMask = Layer<bpp8_t>::VectorMask;
using PsApiBackTextData = BackTextData<PsApiRasterLayerInfo>;
using PsApiFrontTextData = FrontTextData<PsApiVectorMask, PsApiRasterLayerInfo>;
using PsApiTextData = TextData<PsApiVectorMask, PsApiRasterLayerInfo>;
using PsApiData = std::vector<PsApiTextData>;

///////////// 4. the bridge between our text in QT to PhotoshopAPI! /////////////
PsApiData qtToPsApi (QtData qtInfo);

#endif // QTTOPHOTOSHOPAPI_H
