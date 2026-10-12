#include "TextDepth.h"
#include "TextDepthUnit.h"
#include "qtToPhotoshopAPI.h"
#include "PhotoshopWriter.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <stdexcept>

static void check(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}

static void writeFixture(const QString &path, int maskCount, bool hasTargets = true)
{
    LayeredFile<bpp8_t> file(Enum::ColorMode::RGB, 64, 256);
    Layer<bpp8_t>::Params params{};
    params.name = hasTargets ? "targets" : "other";
    auto group = std::make_shared<GroupLayer<bpp8_t>>(params);
    for (int i = 0; i < maskCount; ++i) {
        params.name = "shape" + std::to_string(i);
        params.width = 64;
        params.height = 256;
        params.center_x = 32;
        params.center_y = 128;
        PsApiRasterLayerInfo channels;
        for (auto id : {Enum::ChannelID::Red, Enum::ChannelID::Green, Enum::ChannelID::Blue, Enum::ChannelID::Alpha})
            channels[id] = std::vector<bpp8_t>(64 * 256, id == Enum::ChannelID::Red ? 20 + i : 255);
        auto image = std::make_shared<ImageLayer<bpp8_t>>(channels, params);
        using Point = Layer<bpp8_t>::PathPoint;
        using SubPath = Layer<bpp8_t>::SubPath;
        std::vector<Point> points = {
            Point({10,10}, {10,10}, {10,10}, false),
            Point({40,10}, {40,10}, {40,10}, false),
            Point({40,40}, {40,40}, {40,40}, false),
            Point({10,40}, {10,40}, {10,40}, false)
        };
        image->m_vectorMask = std::make_unique<PsApiVectorMask>(
            std::vector<SubPath>{SubPath(points, true)}, false);
        Layer<bpp8_t>::Params unitParams{};
        unitParams.name = "unit" + std::to_string(i);
        auto unit = std::make_shared<GroupLayer<bpp8_t>>(unitParams);
        unit->layers().push_back(image);
        group->layers().push_back(unit);
    }
    // Non-image children must be skipped safely.
    params = {};
    params.name = "nested";
    group->layers().push_back(std::make_shared<GroupLayer<bpp8_t>>(params));
    file.layers({group});
    LayeredFile<bpp8_t>::write(std::move(file), std::filesystem::u8path(path.toUtf8().toStdString()));
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    check(directory.isValid(), "temporary directory");
    TextDepth backend(64, 256);
    int publications = 0;
    int failures = 0;
    RenderedTextDepthUnits published;
    QObject::connect(&backend, &TextDepth::notifyNewQtData, [&](const RenderedTextDepthUnits &data) {
        ++publications;
        published = RenderedTextDepthUnits(data);
    });
    QObject::connect(&backend, &TextDepth::psdLoadFailed, [&](const QString &message) {
        check(!message.isEmpty(), "failure must have a message");
        ++failures;
    });
    const QString valid = directory.filePath(QString::fromUtf8("shapes ü with spaces.psd"));
    writeFixture(valid, 3);
    check(backend.loadPsd(QUrl::fromLocalFile(valid)), "valid PSD loads");
    check(publications == 1 && published.size() == 3, "publishes all units");
    for (const auto &unit : published) {
        check(!unit.front.baseLayer.isNull(), "front raster imported");
        check(!unit.back, "import does not auto-generate a back");
    }
    check(published.front().front.baseLayer.pixelColor(0, 0).red() == 22
          && published.back().front.baseLayer.pixelColor(0, 0).red() == 20,
          "PSD top-to-bottom order becomes bottom-to-top paint order");
    const auto id = published[1].id;
    check(backend.handleToggleUnitVisibility(id), "generate selected back");
    check(!published[0].back && published[1].back && !published[2].back, "one back included");
    auto dto = qtToPsApi(published);
    check(dto[0].back.baseLayer.empty() && !dto[1].back.baseLayer.empty(), "converter omits absent backs");
    auto converted = psApiToQt(dto, 64, 256);
    check(converted[1].back.has_value() && !converted[0].back, "optional back conversion");
    check(converted[1].front.vectorMaskData.boundingRect() == published[1].front.vectorMaskData.boundingRect(),
          "front vector mask survives conversion");
    PhotoshopWriter writer;
    const auto output = directory.filePath("visible.psd");
    writer.write(output.toStdString(), published);
    auto file = LayeredFile<bpp8_t>::read(output.toStdString());
    check(find_layer_as<bpp8_t, ImageLayer>("targets/Layer 2/Back", file) != nullptr, "visible back written");
    backend.handleToggleUnitVisibility(id);
    writer.write(directory.filePath("hidden.psd").toStdString(), published);
    auto hidden = LayeredFile<bpp8_t>::read(directory.filePath("hidden.psd").toStdString());
    check(find_layer_as<bpp8_t, ImageLayer>("targets/Layer 2/Back", hidden) == nullptr, "hidden back not written");
    check(backend.units()[1].back.has_value(), "export does not discard cached data");
    const auto front = published[1].front.baseLayer;
    const QString missing = directory.filePath("missing-group.psd");
    writeFixture(missing, 3, false);
    check(!backend.loadPsd(QUrl::fromLocalFile(missing)), "missing targets rejected");
    check(!backend.loadPsd(QUrl("https://example.com/file.psd")), "remote URL rejected");
    check(!backend.loadPsd(QUrl::fromLocalFile(directory.filePath("absent.psd"))), "missing file rejected");
    check(failures == 3 && published[1].front.baseLayer == front, "failures preserve state");
    check(backend.loadPsd(QUrl::fromLocalFile(valid)) && published.size() == 3 && !published[1].back,
          "reloading replaces previous document and cached backs");
    qInfo("PSD snapshot and export tests passed");
}
