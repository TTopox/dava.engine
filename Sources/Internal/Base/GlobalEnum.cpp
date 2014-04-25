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



#include "Base/GlobalEnum.h"
#include "Render/Texture.h"
#include "Render/Highlevel/Light.h"
#include "Scene3D/Entity.h"


using namespace DAVA;

ENUM_DECLARE(eGPUFamily)
{
	ENUM_ADD(GPU_POWERVR_IOS);
	ENUM_ADD(GPU_POWERVR_ANDROID);
	ENUM_ADD(GPU_TEGRA);
	ENUM_ADD(GPU_MALI);
	ENUM_ADD(GPU_ADRENO);
}

ENUM_DECLARE(Texture::TextureWrap)
{
	ENUM_ADD_DESCR(Texture::WRAP_CLAMP_TO_EDGE, "WRAP_CLAMP_TO_EDGE");
	ENUM_ADD_DESCR(Texture::WRAP_REPEAT, "WRAP_REPEAT");
}

ENUM_DECLARE(Texture::TextureFilter)
{
	ENUM_ADD_DESCR(Texture::FILTER_LINEAR, "LINEAR");
	ENUM_ADD_DESCR(Texture::FILTER_NEAREST, "NEAREST");
	ENUM_ADD_DESCR(Texture::FILTER_NEAREST_MIPMAP_NEAREST, "NEAREST_MIPMAP_NEAREST");
	ENUM_ADD_DESCR(Texture::FILTER_LINEAR_MIPMAP_NEAREST, "LINEAR_MIPMAP_NEAREST");
	ENUM_ADD_DESCR(Texture::FILTER_NEAREST_MIPMAP_LINEAR, "NEAREST_MIPMAP_LINEAR");
	ENUM_ADD_DESCR(Texture::FILTER_LINEAR_MIPMAP_LINEAR, "LINEAR_MIPMAP_LINEAR");
}

ENUM_DECLARE(PixelFormat)
{
	ENUM_ADD_DESCR(FORMAT_RGBA8888, "RGBA8888");
	ENUM_ADD_DESCR(FORMAT_RGBA5551, "RGBA5551");
	ENUM_ADD_DESCR(FORMAT_RGBA4444, "RGBA4444");
	ENUM_ADD_DESCR(FORMAT_RGB888, "RGB888");
	ENUM_ADD_DESCR(FORMAT_RGB565, "RGB565");
	ENUM_ADD_DESCR(FORMAT_A8, "A8");
	ENUM_ADD_DESCR(FORMAT_A16, "A16");
	ENUM_ADD_DESCR(FORMAT_PVR4, "PVR4");
	ENUM_ADD_DESCR(FORMAT_PVR2, "PVR2");
	ENUM_ADD_DESCR(FORMAT_RGBA16161616, "RGBA16161616");
	ENUM_ADD_DESCR(FORMAT_RGBA32323232, "RGBA32323232");
	ENUM_ADD_DESCR(FORMAT_DXT1, "DXT1");
	ENUM_ADD_DESCR(FORMAT_DXT1NM, "DXT1NM");
	ENUM_ADD_DESCR(FORMAT_DXT1A, "DXT1A");
	ENUM_ADD_DESCR(FORMAT_DXT3, "DXT3");
	ENUM_ADD_DESCR(FORMAT_DXT5, "DXT5");
	ENUM_ADD_DESCR(FORMAT_DXT5NM, "DXT5NM");
	ENUM_ADD_DESCR(FORMAT_ETC1, "ETC1");
	ENUM_ADD_DESCR(FORMAT_ATC_RGB, "ATC_RGB");
	ENUM_ADD_DESCR(FORMAT_ATC_RGBA_EXPLICIT_ALPHA, "ATC_RGBA_EXPLICIT_ALPHA");
	ENUM_ADD_DESCR(FORMAT_ATC_RGBA_INTERPOLATED_ALPHA, "ATC_RGBA_INTERPOLATED_ALPHA");
}

ENUM_DECLARE(Light::eType)
{
	ENUM_ADD_DESCR(Light::TYPE_DIRECTIONAL, "Directional");
	ENUM_ADD_DESCR(Light::TYPE_SPOT, "Spot");
	ENUM_ADD_DESCR(Light::TYPE_POINT, "Point");
	ENUM_ADD_DESCR(Light::TYPE_SKY, "Sky");
}

ENUM_DECLARE(Entity::EntityFlags)
{
    ENUM_ADD_DESCR(Entity::NODE_WORLD_MATRIX_ACTUAL, "NODE_WORLD_MATRIX_ACTUAL");
    ENUM_ADD_DESCR(Entity::NODE_VISIBLE, "NODE_VISIBLE");
    ENUM_ADD_DESCR(Entity::NODE_UPDATABLE, "NODE_UPDATABLE");
    ENUM_ADD_DESCR(Entity::NODE_IS_LOD_PART, "NODE_IS_LOD_PART");
    ENUM_ADD_DESCR(Entity::NODE_LOCAL_MATRIX_IDENTITY, "NODE_LOCAL_MATRIX_IDENTITY");
    ENUM_ADD_DESCR(Entity::BOUNDING_VOLUME_AABB, "BOUNDING_VOLUME_AABB");
    ENUM_ADD_DESCR(Entity::BOUNDING_VOLUME_OOB, "BOUNDING_VOLUME_OOB");
    ENUM_ADD_DESCR(Entity::BOUNDING_VOLUME_SPHERE, "BOUNDING_VOLUME_SPHERE");
    ENUM_ADD_DESCR(Entity::NODE_CLIPPED_PREV_FRAME, "NODE_CLIPPED_PREV_FRAME");
    ENUM_ADD_DESCR(Entity::NODE_CLIPPED_THIS_FRAME, "NODE_CLIPPED_THIS_FRAME");
    ENUM_ADD_DESCR(Entity::NODE_INVALID, "NODE_INVALID");
    ENUM_ADD_DESCR(Entity::TRANSFORM_NEED_UPDATE, "TRANSFORM_NEED_UPDATE");
    ENUM_ADD_DESCR(Entity::TRANSFORM_DIRTY, "TRANSFORM_DIRTY");
    ENUM_ADD_DESCR(Entity::NODE_DELETED, "NODE_DELETED");
    ENUM_ADD_DESCR(Entity::SCENE_LIGHTS_MODIFIED, "SCENE_LIGHTS_MODIFIED");
}

/*
void f()
{
}

GlobalEnum *globalEnum = GlobalEnum::Instance();
f();
*/
/*
->Add(DAVA::MetaInfo::Instance<DAVA::Texture::TextureWrap>(), DAVA::Texture::WRAP_CLAMP_TO_EDGE, "WRAP_CLAMP_TO_EDGE");

ENUM_ADD(DAVA::Texture::TextureWrap, DAVA::Texture::WRAP_CLAMP_TO_EDGE, "WRAP_CLAMP_TO_EDGE");
ENUM_ADD(DAVA::Texture::TextureWrap, DAVA::Texture::WRAP_REPEAT, "WRAP_REPEAT");
*/
