/*==================================================================================
Copyright (c) 2008, binaryzebra
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the binaryzebra nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/

#include "Render/Image/DDS/QualcommHelper.h"
#include "Render/Image/DDS/NvttHelper.h"
#include "Render/Image/Image.h"
#include "Render/PixelFormatDescriptor.h"

#include <libdxt/nvtt_extra.h>

#if defined(__DAVAENGINE_WIN_UAP__)

//disabling of warning 
#pragma warning(push)
#pragma warning(disable : 4091)
#include <libatc/TextureConverter.h>
#pragma warning(pop)

#else
#include <libatc/TextureConverter.h>
#endif

namespace DAVA {
namespace QualcommHelper {

namespace Internal {
int32 GetQualcommFromDava(PixelFormat format)
{
    switch (format)
    {
    case FORMAT_ATC_RGB: return Q_FORMAT_ATC_RGB;
    case FORMAT_ATC_RGBA_EXPLICIT_ALPHA: return Q_FORMAT_ATC_RGBA_EXPLICIT_ALPHA;
    case FORMAT_ATC_RGBA_INTERPOLATED_ALPHA: return Q_FORMAT_ATC_RGBA_INTERPOLATED_ALPHA;
    case FORMAT_RGBA8888: return Q_FORMAT_RGBA_8UI;
    default: DVASSERT_MSG(false, "Unsupported pixel format"); return Q_FORMAT_ATC_RGB;
    }
}
} // namespace Internal

bool IsAtcFormat(PixelFormat format)
{
    return (format == FORMAT_ATC_RGB ||
            format == FORMAT_ATC_RGBA_EXPLICIT_ALPHA ||
            format == FORMAT_ATC_RGBA_INTERPOLATED_ALPHA);
}

bool DecompressAtcToRgba(const Image* srcImage, Image* dstImage)
{
#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)

    DVASSERT_MSG(false, "No need to decompress on mobile platforms");
    return false;

#elif defined(__DAVAENGINE_WIN_UAP__)
    __DAVAENGINE_WIN_UAP_INCOMPLETE_IMPLEMENTATION__
        return false;

#else
#if defined(__DAVAENGINE_MACOS__)
    if (srcImage->format == FORMAT_ATC_RGBA_INTERPOLATED_ALPHA)
    {
        Logger::Error("Decompressing of FORMAT_ATC_RGBA_INTERPOLATED_ALPHA is disabled on OSX platform, because of bug in qualcomm library");
        return ScopedPtr<Image>(nullptr);
    }
#endif

    DVASSERT(srcImage);
    DVASSERT(dstImage);
    DVASSERT(dstImage->format == FORMAT_RGBA8888);

    TQonvertImage srcImg = { 0 };
    TQonvertImage dstImg = { 0 };

    srcImg.nWidth = srcImage->width;
    srcImg.nHeight = srcImage->height;
    srcImg.nFormat = Internal::GetQualcommFromDava(srcImage->format);
    srcImg.nDataSize = srcImage->dataSize;
    srcImg.pData = srcImage->data;

    dstImg.nWidth = srcImage->width;
    dstImg.nHeight = srcImage->height;
    dstImg.nFormat = Q_FORMAT_RGBA_8UI;
    dstImg.nDataSize = 0;
    dstImg.pData = nullptr;

    if (Qonvert(&srcImg, &dstImg) != Q_SUCCESS)
    {
        Logger::Error("[DecompressAtcToRGBA] Failed to get dst data size");
        return false;
    }

    if (dstImg.nDataSize != dstImage->dataSize)
    {
        Logger::Error("[DecompressAtcToRGBA] dst data size is %d, expected is %d", dstImg.nDataSize, dstImage->dataSize);
        return false;
    }

    static_assert(sizeof(uint8) == sizeof(unsigned char), ""); // qualcomm uses unsigned char to store image data, whereas we use uint8
    dstImg.pData = dstImage->data;
    if (Qonvert(&srcImg, &dstImg) != Q_SUCCESS)
    {
        Logger::Error("[DecompressAtc] Failed to convert data");
        return false;
    }

    dstImage->mipmapLevel = srcImage->mipmapLevel;
    dstImage->cubeFaceID = srcImage->cubeFaceID;

    return true;
#endif
}

bool CompressRgbaToAtc(const Image* srcImage, Image* dstImage)
{
#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
    DVASSERT_MSG(false, "Qualcomm doesn't provide texture converter library for ios/android");
    return false;

#elif defined(__DAVAENGINE_WIN_UAP__)
    __DAVAENGINE_WIN_UAP_INCOMPLETE_IMPLEMENTATION__
        return false;

#else
    DVASSERT(srcImage);
    DVASSERT(dstImage);
    DVASSERT(srcImage->format == FORMAT_RGBA8888);
    DVASSERT(IsAtcFormat(dstImage->format));

    TQonvertImage srcImg = { 0 };
    TQonvertImage dstImg = { 0 };

    srcImg.nWidth = srcImage->width;
    srcImg.nHeight = srcImage->height;
    srcImg.nFormat = Internal::GetQualcommFromDava(srcImage->format);
    srcImg.nDataSize = srcImage->dataSize;
    srcImg.pData = srcImage->data;

    dstImg.nWidth = dstImage->width;
    dstImg.nHeight = dstImage->height;
    dstImg.nFormat = Internal::GetQualcommFromDava(dstImage->format);
    dstImg.nDataSize = 0;
    dstImg.pData = nullptr;

    if (Qonvert(&srcImg, &dstImg) != Q_SUCCESS || dstImg.nDataSize == 0)
    {
        Logger::Error("[QualcommHelper::CompressRgbaToAtc] Convert error");
        return false;
    }

    if (dstImg.nDataSize != dstImage->dataSize)
    {
        Logger::Error("[QualcommHelper::CompressRgbaToAtc] dst data size is %d, expected is %d", dstImg.nDataSize, dstImage->dataSize);
        return false;
    }

    dstImg.pData = dstImage->data;
    if (Qonvert(&srcImg, &dstImg) != Q_SUCCESS || dstImg.nDataSize == 0)
    {
        Logger::Error("[QualcommHelper::CompressRgbaToAtc] Convert error");
        return false;
    }

    return true;
#endif
}

} // namespace QualcommHelper
} // namespace DAVA
