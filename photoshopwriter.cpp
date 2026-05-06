#include "photoshopwriter.h"

void PhotoshopWriter::write(std::string filename, std::vector<std::vector<QPointF>> subpaths) {
    qDebug() << "Writing to: " << filename;

    using namespace NAMESPACE_PSAPI;
    using PathPoint = ImageLayer<bpp8_t>::PathPoint;
    using SubPath = ImageLayer<bpp8_t>::SubPath;
    using VectorMask = ImageLayer<bpp8_t>::VectorMask;
    using Point2D = Geometry::Point2D<int>;

    // Initialize some constants that we will need throughout the program
    constexpr uint32_t width = 2000u;
    constexpr uint32_t height = 2000u;

    // Create an 8-bit LayeredFile as our starting point, 8- 16- and 32-bit are fully supported
    LayeredFile<bpp8_t> document = { Enum::ColorMode::RGB, width, height };
    // Create our individual channels to add to our image layer. Keep in mind that all these 3 channels need to
    // be specified for RGB mode
    std::unordered_map <Enum::ChannelID, std::vector<bpp8_t>> channel_map;
    channel_map[Enum::ChannelID::Red] = std::vector<bpp8_t>(width * height, 255u);
    channel_map[Enum::ChannelID::Green] = std::vector<bpp8_t>(width * height, 0u);
    channel_map[Enum::ChannelID::Blue] = std::vector<bpp8_t>(width * height, 0u);

    // Create a mask channel which for now is just a semi grey channel. This channel for the time being
    // needs to be the exact same size as the layer even though Photoshop officially supports masks being smaller
    // or larger than channels

    ImageLayer<bpp8_t>::Params layer_params = {};
    layer_params.name = "Layer Red";
    layer_params.width = width;
    layer_params.height = height;
    layer_params.center_x = 32;
    layer_params.center_y = 32;

    auto layer = std::make_shared<ImageLayer<bpp8_t>>(
        std::move(channel_map),
        layer_params
        );

    //PathPoint p1(Point2D(10, 10), Point2D(10,10), Point2D(10,10), false);
    //PathPoint p2(Point2D(20, 10), Point2D(20,10), Point2D(20,10), false);
    //PathPoint p3(Point2D(15, 15), Point2D(15,15), Point2D(15,15), false);
//////////////////////////////////////
    std::vector<SubPath> subPaths;
    for (auto & subpath : subpaths)
    {
        std::vector<PathPoint> pathPts;

        for (QPointF & pt : subpath)
        {
            int x = pt.x();
            int y = pt.y();
            PathPoint p({x, y}, {x,y}, {x,y}, false);
            pathPts.push_back(p);
        }

        SubPath subPath(pathPts, true);
        subPaths.push_back(subPath);
    }

    VectorMask vecMask(subPaths, false);
    layer->set_vector_mask(vecMask);

    document.add_layer(layer);

    // Convert to PhotoshopDocument and write to disk. Note that from this point onwards
    // our LayeredFile instance is no longer usable
    LayeredFile<bpp8_t>::write(std::move(document), "WriteVectorMasks.psd");

    // qDebug() << "Grrrr";
}
