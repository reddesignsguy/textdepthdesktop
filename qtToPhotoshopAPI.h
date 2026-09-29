#ifndef QTTOPHOTOSHOPAPI_H
#define QTTOPHOTOSHOPAPI_H

#include <QPainterPath>
#include <QImage>
#include <QMetaType>
#include <unordered_map>
#include <vector>
#include <PhotoshopAPI.h>

using namespace NAMESPACE_PSAPI;

// Structures prepared by the Qt application.
struct QtBackTextData {
    QImage baseLayer;
    std::vector<QImage> clippedLayers;
};

struct QtFrontTextData {
    QPainterPath vectorMaskData;
    QImage baseLayer;
    std::vector<QImage> clippedLayers;
};

struct QtTextData {
    QtFrontTextData front;
    QtBackTextData back;
};

using QtData = std::vector<QtTextData>;
Q_DECLARE_METATYPE(QtData)

// Structures consumed by PhotoshopAPI.
using PsApiRasterLayerInfo = std::unordered_map<Enum::ChannelID, std::vector<bpp8_t>>;
using PsApiVectorMask = Layer<bpp8_t>::VectorMask;

struct PsApiBackTextData {
    PsApiRasterLayerInfo baseLayer;
    std::vector<PsApiRasterLayerInfo> clippedLayers;
};

struct PsApiFrontTextData {
    PsApiVectorMask vectorMaskData;
    PsApiRasterLayerInfo baseLayer;
    std::vector<PsApiRasterLayerInfo> clippedLayers;
};

struct PsApiTextData {
    PsApiFrontTextData front;
    PsApiBackTextData back;
};

using PsApiLayersDto = std::vector<PsApiTextData>;

// Convert Qt text data to PhotoshopAPI text data.
PsApiLayersDto qtToPsApi (QtData qtInfo);

// Convert bridge data back to Qt, using the same dimensions for all raster layers.
// Empty channel maps produce null images. Nonempty maps require RGB channels;
// alpha is optional (defaults to opaque). Invalid dimensions/channel sizes throw
// std::invalid_argument. Vector paths use qtToPsApi's per-endpoint handle encoding.
QtData psApiToQt(const PsApiLayersDto& psInfo, int width, int height);

#endif // QTTOPHOTOSHOPAPI_H
