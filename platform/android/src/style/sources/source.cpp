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

#include <string>

namespace mbgl {
namespace android {

    /**
     * Invoked when the construction is initiated from the jvm through a subclass
     */
    Source::Source(jni::JNIEnv&, std::unique_ptr<mbgl::style::Source> coreSource)
        : ownedSource(std::move(coreSource))
        , source(*ownedSource) {
    }

    Source::Source(mbgl::style::Source& coreSource)
            : source(coreSource) {
    }

    Source::~Source() {
        // Before being added to a map, the Java peer owns this C++ peer and cleans
        //  up after itself correctly through the jni native peer bindings.
        // After being added to the map, the ownership is flipped and the C++ peer has a strong reference
        //  to it's Java peer, preventing the Java peer from being GC'ed.
        //  In this case, the core source initiates the destruction, which requires releasing the Java peer,
        //  while also resetting it's nativePtr to 0 to prevent the subsequent GC of the Java peer from
        // re-entering this dtor.
        if (ownedSource.get() == nullptr && javaPeer.get() != nullptr) {
            //Manually clear the java peer
            android::UniqueEnv env = android::AttachEnv();
            static auto nativePtrField = javaClass.GetField<jlong>(*env, "nativePtr");
            javaPeer->Set(*env, nativePtrField, (jlong) 0);
            javaPeer.reset();
        }
    }

    style::Source& Source::get() {
        return source;
    }

    jni::String Source::getId(jni::JNIEnv& env) {
        return jni::Make<jni::String>(env, source.getID());
    }

    jni::String Source::getAttribution(jni::JNIEnv& env) {
        auto attribution = source.getAttribution();
        return attribution ? jni::Make<jni::String>(env, attribution.value()) : jni::Make<jni::String>(env,"");
    }

    void Source::addToMap(JNIEnv& env, jni::Object<Source> obj, mbgl::Map& map) {
        // Check to see if we own the source first
        if (!ownedSource) {
            throw std::runtime_error("Cannot add source twice");
        }

        // Add source to map and release ownership
        map.getStyle().addSource(releaseCoreSource());
        attachJavaSource(env, obj);
    }

    void Source::removeFromMap(JNIEnv&, jni::Object<Source>, mbgl::Map& map) {
        //Cannot remove if not attached yet
        if (ownedSource) {
            throw std::runtime_error("Cannot remove source detached source");
        }

        // Remove the source from the map
        std::unique_ptr<mbgl::style::Source> coreSource = map.getStyle().removeSource(source.getID());
        attachCoreSource(std::move(coreSource));
    }

    jni::jobject* Source::attachCoreSource(std::unique_ptr<style::Source> coreSource) {
        // Take over the source
        ownedSource = std::move(coreSource);

        // Release the peer relationships. These will be re-established when the source is added to a map
        if (ownedSource->peer.has_value()) {
            util::any_cast<std::unique_ptr<Source>>(&(ownedSource->peer))->release();
            ownedSource->peer = nullptr;
        }

        return (jni::jobject *) javaPeer.release()->Get();
    }

    void Source::attachJavaSource(JNIEnv& env, jni::Object<Source> obj) {
        // Add peer to core source
        source.peer = std::unique_ptr<Source>(this);

        // Add strong reference to java source
        javaPeer = obj.NewGlobalRef(env);
    }

    void Source::setRendererFrontend(AndroidRendererFrontend& frontend_) {
        rendererFrontend = &frontend_;
    }

    std::unique_ptr<mbgl::style::Source> Source::releaseCoreSource() {
        assert(ownedSource != nullptr);
        return std::move(ownedSource);
    }

    jni::jobject* Source::getJavaPeer(jni::JNIEnv& env) {
        if(!javaPeer) {
            // Create Java peer for sources already in the map
            auto jp = jni::Object<Source>(createJavaPeer(env));
            attachJavaSource(env, jp);
            jni::DeleteLocalRef(env, jp);
        }
        return (jni::jobject *) javaPeer->Get();
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

    }

} // namespace android
} // namespace mbgl
