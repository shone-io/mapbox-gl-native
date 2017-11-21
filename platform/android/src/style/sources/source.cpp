#include "source.hpp"
#include "../android_conversion.hpp"

#include <jni/jni.hpp>

#include <mbgl/style/style.hpp>
#include <mbgl/util/logging.hpp>

// Java -> C++ conversion
#include <mbgl/style/conversion.hpp>
#include <mbgl/style/conversion/source.hpp>

// C++ -> Java conversion
#include "../conversion/property_value.hpp"

#include <mbgl/style/sources/geojson_source.hpp>
#include <mbgl/style/sources/image_source.hpp>
#include <mbgl/style/sources/raster_source.hpp>
#include <mbgl/style/sources/vector_source.hpp>

#include "geojson_source.hpp"
#include "image_source.hpp"
#include "raster_source.hpp"
#include "unknown_source.hpp"
#include "vector_source.hpp"
#include "custom_geometry_source.hpp"

#include <string>

namespace mbgl {
namespace android {

static unique_ptr<Source> createSourcePeer(jni::JNIEnv& env, mbgl::style::Source& coreSource, AndroidRendererFrontend& frontend) {
    if (coreSource.is<mbgl::style::VectorSource>()) {
        return std::make_unique<VectorSource>(env, *coreSource.as<mbgl::style::VectorSource>(), frontend);
    } else if (coreSource.is<mbgl::style::RasterSource>()) {
        return std::make_unique<RasterSource>(env, *coreSource.as<mbgl::style::RasterSource>(), frontend);
    } else if (coreSource.is<mbgl::style::GeoJSONSource>()) {
        return std::make_unique<GeoJSONSource>(env, *coreSource.as<mbgl::style::GeoJSONSource>(), frontend);
    } else if (coreSource.is<mbgl::style::ImageSource>()) {
        return std::make_unique<ImageSource>(env, *coreSource.as<mbgl::style::ImageSource>(), frontend);
    } else {
        return std::make_unique<UnknownSource>(env, coreSource, frontend);
    }
}

jni::Object<Source> Source::peerForCoreSource(jni::JNIEnv& env, mbgl::style::Source& coreSource, AndroidRendererFrontend& frontend) {
    if (coreSource.peer.has_value()) {
        return mbgl::util::any_cast<std::unique_ptr<Source>&>(coreSource.peer)->javaPeer;
    } else {
        return (coreSource.peer = createSourcePeer(coreSource))->javaPeer;
    }
}

Source::Source(jni::JNIEnv&, std::unique_ptr<mbgl::style::Source> coreSource)
    : ownedSource(std::move(coreSource))
    , source(*ownedSource)
    , frontend(nullptr) {
}

Source::Source(jni::JNIEnv&, mbgl::style::Source& coreSource, jni::Object<Source> javaPeer_, AndroidRendererFrontend& frontend_)
    : source(coreSource)
    , javaPeer(javaPeer_.NewGlobalRef())
    , frontend(&frontend_) {
}

Source::~Source() {
    // Before being added to a map, the Java peer owns this C++ peer and cleans
    //  up after itself correctly through the jni native peer bindings.
    // After being added to the map, the ownership is flipped and the C++ peer has a strong reference
    //  to it's Java peer, preventing the Java peer from being GC'ed.
    //  In this case, the core source initiates the destruction, which requires releasing the Java peer,
    //  while also resetting it's nativePtr to 0 to prevent the subsequent GC of the Java peer from
    // re-entering this dtor.
    if (!ownedSource) {
        assert(javaPeer);
        android::UniqueEnv env = android::AttachEnv();
        static auto nativePtrField = javaClass.GetField<jlong>(*env, "nativePtr");
        javaPeer->Set<jlong>(*env, nativePtrField, 0);
        javaPeer.reset();
    }
}

style::Source& Source::getCoreSource() {
    return source;
}

jni::String Source::getId(jni::JNIEnv& env) {
    return jni::Make<jni::String>(env, source.getID());
}

jni::String Source::getAttribution(jni::JNIEnv& env) {
    auto attribution = source.getAttribution();
    return attribution ? jni::Make<jni::String>(env, attribution.value()) : jni::Make<jni::String>(env,"");
}

void Source::addToMap(JNIEnv& env, jni::Object<Source> obj, mbgl::Map& map, AndroidRendererFrontend& frontend_) {
    // Check to see if we own the source first
    if (!ownedSource) {
        throw std::runtime_error("Cannot add source twice");
    }

    // Add source to map and release ownership.
    map.getStyle().addSource(std::move(ownedSource));

    // Transfer C++ peer ownership from Java peer to core source.
    source.peer = std::unique_ptr<Source>(this);

    // Add strong reference to Java peer.
    javaPeer = obj.NewGlobalRef(env);

    rendererFrontend = &frontend_;
}

void Source::removeFromMap(JNIEnv&, jni::Object<Source>, mbgl::Map& map) {
    // Cannot remove if not attached yet
    if (ownedSource) {
        throw std::runtime_error("Cannot remove source detached source");
    }

    // Remove the source from the map and take ownership.
    ownedSource = map.getStyle().removeSource(source.getID());

    // Transfer C++ peer ownership from core source to Java peer.
    assert(ownedSource->peer.has_value());
    util::any_cast<std::unique_ptr<Source>&>(ownedSource->peer).release();
    ownedSource->peer.reset();

    // Release strong reference to Java peer.
    assert(javaPeer);
    javaPeer.release();

    rendererFrontend = nullptr;
}

jni::Class<Source> Source::javaClass;

void Source::registerNative(jni::JNIEnv& env) {
    // Lookup the class
    Source::javaClass = *jni::Class<Source>::Find(env).NewGlobalRef(env).release();

    #define METHOD(MethodPtr, name) jni::MakeNativePeerMethod<decltype(MethodPtr), (MethodPtr)>(name)

    // Register the peer
    jni::RegisterNativePeer<Source>(env, Source::javaClass, "nativePtr",
        METHOD(&Source::getId, "nativeGetId"),
        METHOD(&Source::getAttribution, "nativeGetAttribution")
    );

    // Register subclasses
    GeoJSONSource::registerNative(env);
    ImageSource::registerNative(env);
    RasterSource::registerNative(env);
    UnknownSource::registerNative(env);
    VectorSource::registerNative(env);
    CustomGeometrySource::registerNative(env);
}

} // namespace android
} // namespace mbgl
