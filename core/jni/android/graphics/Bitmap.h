/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef BITMAP_H_
#define BITMAP_H_

#include <jni.h>
#include <SkBitmap.h>
#include <SkColorTable.h>
#include <SkImageInfo.h>
#include <SkPixelRef.h>

namespace android {

enum class PixelStorageType {
    External,
    Heap,
    Ashmem,
};

typedef void (*FreeFunc)(void* addr, void* context);

class PixelRef;

namespace bitmap {

enum BitmapCreateFlags {
    kBitmapCreateFlag_None = 0x0,
    kBitmapCreateFlag_Mutable = 0x1,
    kBitmapCreateFlag_Premultiplied = 0x2,
};

jobject createBitmap(JNIEnv* env, PixelRef* bitmap,
            int bitmapCreateFlags, jbyteArray ninePatchChunk = NULL,
            jobject ninePatchInsets = NULL, int density = -1);


void toSkBitmap(jlong bitmapHandle, SkBitmap* outBitmap);

PixelRef* toPixelRef(JNIEnv* env, jobject bitmap);

/** Reinitialize a bitmap. bitmap must already have its SkAlphaType set in
    sync with isPremultiplied
*/
void reinitBitmap(JNIEnv* env, jobject javaBitmap, const SkImageInfo& info,
        bool isPremultiplied);

int getBitmapAllocationByteCount(JNIEnv* env, jobject javaBitmap);

} // namespace bitmap

class PixelRef : public SkPixelRef {
public:
    PixelRef(void* address, size_t allocSize, const SkImageInfo& info, size_t rowBytes,
            SkColorTable* ctable);
    PixelRef(void* address, void* context, FreeFunc freeFunc,
            const SkImageInfo& info, size_t rowBytes, SkColorTable* ctable);
    PixelRef(void* address, int fd, size_t mappedSize, const SkImageInfo& info,
            size_t rowBytes, SkColorTable* ctable);

    int width() const { return info().width(); }
    int height() const { return info().height(); }

    // Can't mark as override since SkPixelRef::rowBytes isn't virtual
    // but that's OK since we just want Bitmap to be able to rely
    // on calling rowBytes() on an unlocked pixelref, which it will be
    // doing on a PixelRef type, not a SkPixelRef, so static
    // dispatching will do what we want.
    size_t rowBytes() const { return mRowBytes; }
    void reconfigure(const SkImageInfo& info, size_t rowBytes, SkColorTable* ctable);
    void reconfigure(const SkImageInfo& info);
    void setAlphaType(SkAlphaType alphaType);

    void getSkBitmap(SkBitmap* outBitmap);

    int getAshmemFd() const;
    size_t getAllocationByteCount() const;

protected:
    virtual bool onNewLockPixels(LockRec* rec) override;
    virtual void onUnlockPixels() override { };
    virtual size_t getAllocatedSizeInBytes() const override;
private:
    friend class Bitmap;
    virtual ~PixelRef();
    void doFreePixels();
    void* getStorage() const;
    void setHasHardwareMipMap(bool hasMipMap);
    bool hasHardwareMipMap() const;

    PixelStorageType mPixelStorageType;

    size_t mRowBytes = 0;
    sk_sp<SkColorTable> mColorTable;
    bool mHasHardwareMipMap = false;

    union {
        struct {
            void* address;
            void* context;
            FreeFunc freeFunc;
        } external;
        struct {
            void* address;
            int fd;
            size_t size;
        } ashmem;
        struct {
            void* address;
            size_t size;
        } heap;
    } mPixelStorage;
};

} // namespace android

#endif /* BITMAP_H_ */
