#ifndef __DAVAENGINE_DXT_HELPER_H__
#define __DAVAENGINE_DXT_HELPER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/BaseObject.h"
#include "Render/Image.h"

namespace DAVA 
{

class Texture;
class Sprite;
class Image;


class LibDxtHelper
{
public:
	
	static bool IsDxtFile(const char *fileName);

	static bool IsDxtFile(DAVA::File * file);

	//input data only in RGBA8888
	static bool WriteDxtFile(const char* fileName, int32 width, int32 height, uint8 * data, PixelFormat compressionFormat, uint32 mipmupNumber);

	static bool ReadDxtFile(const char *fileName, Vector<DAVA::Image*> &imageSet, bool forseSoftwareConvertation = false);
	
	static bool ReadDxtFile(DAVA::File * file, Vector<DAVA::Image*> &imageSet, bool forseSoftwareConvertation = false);

	static bool DecompressImageToRGBA(const DAVA::Image & image, Vector<DAVA::Image*> &imageSet, bool forseSoftwareConvertation = false);

	static PixelFormat GetPixelFormat(const char* fileName);
	
	static PixelFormat GetPixelFormat(DAVA::File * file);
	
	static bool GetTextureSize(const char *fileName, uint32 & width, uint32 & height);

	static bool GetTextureSize(DAVA::File * file, uint32 & width, uint32 & height);

	static uint32 GetMipMapLevelsCount(const char *fileName);

	static uint32 GetMipMapLevelsCount(DAVA::File * file);

	static uint32 GetDataSize(const char *fileName);
	
	static uint32 GetDataSize(DAVA::File * file);

	//static void Test();
};

};

#endif // __DXT_HELPER_H__