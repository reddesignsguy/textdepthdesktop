# Viewport rendering

The app uses Qt Quick's OpenGL scene graph (selected in `main.cpp`). The viewport
is a `QQuickItem`, and Qt owns the graphics context, shaders, buffers, textures,
and render thread. There is no framebuffer-sized QPainter repaint during a drag
or pulse.

## Where to make changes

- `TextDepthViewport`: receives document/preview messages and schedules updates.
  Change the pulse here. `wireframeColor` and `wireframeWidth` are QML properties;
  width is in document coordinates and scales with canvas zoom.
- `ViewportScene`: prepares images and creates the scene tree. Each unit draws
  its back (or temporary preview), then its front, before the next unit. Change
  layer composition or ordering here.
- `WireframeNode`: turns a path into a triangle mesh and sets its material color.
  Change stroke geometry here. Every straight segment uses six vertices,
  independent of its length. Curves are flattened when a new preview arrives.

## Update costs

A document publication composites raster clips and masks the front once on the
CPU, then uploads the resulting images as textures. QPainter is used only in
that preparation step. Moving the back sends new preview geometry; it does not
re-upload the layer textures. Holding the mouse still and pulsing changes only
the material color, reusing the mesh. Release/cancel removes the preview through
the existing backend message flow.

Document and preview revision counters keep these operations independent.
`updatePaintNode()` is Qt's synchronization point: it reads item state while the
GUI thread is blocked and updates render-thread-owned nodes. Qt deletes the node
tree and its resources when necessary; a newly created tree rebuilds itself from
the current item state. Never retain scene node pointers in the backend or QML.

The back-generation algorithm and immutable document snapshots are independent
of this renderer. GPU rendering can still be limited by visible pixel coverage,
but longer connectors no longer cause CPU path rasterization on every frame.

References: [Qt scene graph](https://doc.qt.io/qt-6.9/qtquick-visualcanvas-scenegraph.html),
[QQuickWindow graphics API](https://doc.qt.io/qt-6.9/qquickwindow.html#setGraphicsApi).

## Validation

The `layer_model` test reads back an actual OpenGL framebuffer and checks masking,
layer order, cached-back replacement, and a connector extending 100,000 document
units beyond the canvas. It also checks constant connector vertex count and
geometry reuse across color changes. It needs a graphics-capable display; Qt's
`offscreen` platform can silently choose software rendering, which this test
rejects. The other tests continue to run headlessly.
