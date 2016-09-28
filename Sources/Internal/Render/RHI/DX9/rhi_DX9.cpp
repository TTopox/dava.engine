#include "rhi_DX9.h"
    #include "../Common/rhi_Impl.h"
    
    #include "Debug/DVAssert.h"
    #include "Logger/Logger.h"
    #include "Core/Core.h"
using DAVA::Logger;

    #include "_dx9.h"
    #include "../rhi_Type.h"
    #include "../Common/dbg_StatSet.h"

    #include <vector>


#define E_MINSPEC (-3) // Error code for gfx-card that doesn't meet min.spec

namespace rhi
{
//==============================================================================

Dispatch DispatchDX9 = {};

//==============================================================================

struct
DisplayMode
{
    unsigned width;
    unsigned height;
    //    Texture::Format format;
    unsigned refresh_rate;
};

std::vector<DisplayMode> _DisplayMode;

//------------------------------------------------------------------------------

static Api
dx9_HostApi()
{
    return RHI_DX9;
}

//------------------------------------------------------------------------------

static bool
dx9_TextureFormatSupported(TextureFormat format)
{
    return SUCCEEDED(_D3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, DX9_TextureFormat(format)));
}

//------------------------------------------------------------------------------

static bool
dx9_VertexTextureFormatSupported(TextureFormat format)
{
    return SUCCEEDED(_D3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, DX9_TextureFormat(format)));
}

//------------------------------------------------------------------------------

static bool
_IsValidIntelCardDX9(unsigned vendor_id, unsigned device_id)
{
    return ((vendor_id == 0x8086) && // Intel Architecture

            // These guys are prehistoric :)

            (device_id == 0x2572) || // 865G
            (device_id == 0x3582) || // 855GM
            (device_id == 0x2562) || // 845G
            (device_id == 0x3577) || // 830M

            // These are from 2005 and later

            (device_id == 0x2A02) || // GM965 Device 0
            (device_id == 0x2A03) || // GM965 Device 1
            (device_id == 0x29A2) || // G965 Device 0
            (device_id == 0x29A3) || // G965 Device 1
            (device_id == 0x27A2) || // 945GM Device 0
            (device_id == 0x27A6) || // 945GM Device 1
            (device_id == 0x2772) || // 945G Device 0
            (device_id == 0x2776) || // 945G Device 1
            (device_id == 0x2592) || // 915GM Device 0
            (device_id == 0x2792) || // 915GM Device 1
            (device_id == 0x2582) || // 915G Device 0
            (device_id == 0x2782) // 915G Device 1
            );
}

//------------------------------------------------------------------------------

static void
dx9_Uninitialize()
{
    QueryBufferDX9::ReleaseQueryPool();
    UninitializeRenderThreadDX9();
}

//------------------------------------------------------------------------------

static void
dx9_Reset(const ResetParam& param)
{
    UINT interval = (param.vsyncEnabled) ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

    if (param.width != _DX9_PresentParam.BackBufferWidth
        || param.height != _DX9_PresentParam.BackBufferHeight
        || param.fullScreen != !_DX9_PresentParam.Windowed
        || interval != _DX9_PresentParam.PresentationInterval
        )
    {
        _DX9_PresentParam.BackBufferWidth = param.width;
        _DX9_PresentParam.BackBufferHeight = param.height;
        _DX9_PresentParam.Windowed = !param.fullScreen;
        _DX9_PresentParam.PresentationInterval = interval;

        ScheduleDeviceReset();
    }
}

//------------------------------------------------------------------------------

static bool
dx9_NeedRestoreResources()
{
    uint32 pendingTextures = TextureDX9::NeedRestoreCount();
    uint32 pendingVertexBuffers = VertexBufferDX9::NeedRestoreCount();
    uint32 pendingIndexBuffers = IndexBufferDX9::NeedRestoreCount();

    bool needRestore = (pendingTextures || pendingVertexBuffers || pendingIndexBuffers);
    if (needRestore)
    {
        Logger::Debug("NeedRestore %d TEX, %d VB, %d IB", pendingTextures, pendingVertexBuffers, pendingIndexBuffers);
    }
    return needRestore;
}

//------------------------------------------------------------------------------

void DX9CheckMultisampleSupport()
{
    const _D3DFORMAT formatsToCheck[] = { D3DFMT_A8R8G8B8, D3DFMT_D24S8 };
    const D3DMULTISAMPLE_TYPE samplesToCheck[] = { D3DMULTISAMPLE_2_SAMPLES, D3DMULTISAMPLE_4_SAMPLES, D3DMULTISAMPLE_8_SAMPLES };

    uint32 sampleCount = 2;

    for (uint32 s = 0; (s < countof(samplesToCheck)) && (sampleCount <= 8); ++s, sampleCount *= 2)
    {
        DWORD qualityLevels = 0;
        for (uint32 f = 0; f < countof(formatsToCheck); ++f)
        {
            HRESULT hr = _D3D9->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, formatsToCheck[f], TRUE, samplesToCheck[s], &qualityLevels);
            if (FAILED(hr))
            {
                break;
            }
        }

        if (qualityLevels == 0)
        {
            DAVA::Logger::Info("DX9 max multisample samples: %u", sampleCount);
            break;
        }
    }

    MutableDeviceCaps::Get().maxSamples = sampleCount / 2;
}

//------------------------------------------------------------------------------

void _InitDX9()
{
    _D3D9 = Direct3DCreate9(D3D_SDK_VERSION);

    if (_D3D9)
    {
        HRESULT hr;
        HWND wnd = (HWND)_DX9_InitParam.window;
        unsigned backbuf_width = _DX9_InitParam.width;
        unsigned backbuf_height = _DX9_InitParam.height;
        bool use_vsync = _DX9_InitParam.vsyncEnabled;
        D3DADAPTER_IDENTIFIER9 info = { 0 };
        D3DCAPS9 caps;
        DWORD vertex_processing = E_FAIL;

        hr = _D3D9->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &info);

        // check if running on Intel card

        Logger::Info("vendor-id : %04X  device-id : %04X\n", info.VendorId, info.DeviceId);

        hr = _D3D9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

        if (SUCCEEDED(hr))
        {
            if (caps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY)
            {
                MutableDeviceCaps::Get().maxAnisotropy = caps.MaxAnisotropy;
            }

            if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
            {
                vertex_processing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
            }
            else
            {
                // check vendor and device ID and enable SW vertex processing
                // for Intel(R) Extreme Graphics cards

                if (SUCCEEDED(hr)) // if GetAdapterIdentifier SUCCEEDED
                {
                    if (_IsValidIntelCardDX9(info.VendorId, info.DeviceId))
                    {
                        vertex_processing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
                    }
                    else
                    {
                        // this is something else
                        vertex_processing = E_MINSPEC;
                        Logger::Error("GPU does not meet minimum specs: Intel(R) 845G or Hardware T&L chip required\n");
                        ///                        return false;
                        return;
                    }
                }
            }
        }
        else
        {
            Logger::Error("failed to get device caps:\n%s\n", D3D9ErrorText(hr));

            if (_IsValidIntelCardDX9(info.VendorId, info.DeviceId))
                vertex_processing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        if (vertex_processing == E_FAIL)
        {
            Logger::Error("failed to identify GPU\n");
            ///            return false;
            return;
        }

        // detect debug DirectX runtime
        /*
        _debug_dx_runtime = false;

        HKEY    key_direct3d;

        if( ::RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Direct3D", &key_direct3d ) == ERROR_SUCCESS )
        {
            DWORD   type;
            DWORD   data    = 0;
            DWORD   data_sz = sizeof(data);

            if( ::RegQueryValueExA( key_direct3d, "LoadDebugRuntime", UNUSED_PARAM, &type, (BYTE*)(&data), &data_sz ) == ERROR_SUCCESS )
            {
                _debug_dx_runtime = (data == 1) ? true : false;
            }
        }
        note( "using %s DirectX runtime\n", (_debug_dx_runtime) ? "debug" : "retail" );
*/

        // create device

        // CRAP: hardcoded params

        _DX9_PresentParam.hDeviceWindow = wnd;
        _DX9_PresentParam.Windowed = TRUE;
        _DX9_PresentParam.BackBufferFormat = D3DFMT_UNKNOWN;
        _DX9_PresentParam.BackBufferWidth = backbuf_width;
        _DX9_PresentParam.BackBufferHeight = backbuf_height;
        _DX9_PresentParam.SwapEffect = D3DSWAPEFFECT_DISCARD;
        _DX9_PresentParam.BackBufferCount = use_vsync ? 2 : 1;
        _DX9_PresentParam.EnableAutoDepthStencil = TRUE;
        _DX9_PresentParam.AutoDepthStencilFormat = D3DFMT_D24S8;
        _DX9_PresentParam.PresentationInterval = (use_vsync) ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

        // TODO: check z-buf formats and create most suitable

        // check if specified display-mode supported

        ///        _DetectVideoModes();
        /*
        if( !_PresentParam.Windowed )
        {
            bool    found = false;

            for( unsigned f=0; f<countof(_VideomodeFormat); ++f )
            {            
                D3DFORMAT   fmt = _VideomodeFormat[f];
                
                for( unsigned m=0; m<_DisplayMode.count(); ++m )
                {
                    if(     _DisplayMode[m].width == _PresentParam.BackBufferWidth 
                        &&  _DisplayMode[m].height == _PresentParam.BackBufferHeight 
                        &&  _DisplayMode[m].format == fmt
                      )
                    {
                        found = true;
                        break;
                    }
                }

                if( found )
                {
                    _PresentParam.BackBufferFormat = (D3DFORMAT)fmt;
                    break;
                }
            }

            if( !found )
            {
                Log::Error( "rhi.DX9", "invalid/unsuported display mode %ux%u\n", _PresentParam.BackBufferWidth, _PresentParam.BackBufferHeight );
///                return false;
                return;
            }
        }
*/

        // create device

        if (SUCCEEDED(hr = _D3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, vertex_processing, &_DX9_PresentParam, &_D3D9_Device)))
        {
            if (SUCCEEDED(_D3D9->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &info)))
            {
                Memcpy(MutableDeviceCaps::Get().deviceDescription, info.Description,
                       DAVA::Min(countof(MutableDeviceCaps::Get().deviceDescription), strlen(info.Description) + 1));

                Logger::Info("Adapter[%u]:\n  %s \"%s\"\n", D3DADAPTER_DEFAULT, info.DeviceName, info.Description);
                Logger::Info("  Driver %u.%u.%u.%u\n",
                             HIWORD(info.DriverVersion.HighPart),
                             LOWORD(info.DriverVersion.HighPart),
                             HIWORD(info.DriverVersion.LowPart),
                             LOWORD(info.DriverVersion.LowPart));
            }
        }
        else
        {
            Logger::Error("failed to create device:\n%s\n", D3D9ErrorText(hr));
        }
    }
    else
    {
        Logger::Error("failed to create Direct3D object\n");
    }
}

//------------------------------------------------------------------------------

void dx9_Initialize(const InitParam& param)
{
    _DX9_InitParam = param;
    InitializeRenderThreadDX9((param.threadedRenderEnabled) ? param.threadedRenderFrameCount : 0);

    VertexBufferDX9::SetupDispatch(&DispatchDX9);
    IndexBufferDX9::SetupDispatch(&DispatchDX9);
    QueryBufferDX9::SetupDispatch(&DispatchDX9);
    PerfQuerySetDX9::SetupDispatch(&DispatchDX9);
    TextureDX9::SetupDispatch(&DispatchDX9);
    PipelineStateDX9::SetupDispatch(&DispatchDX9);
    ConstBufferDX9::SetupDispatch(&DispatchDX9);
    DepthStencilStateDX9::SetupDispatch(&DispatchDX9);
    SamplerStateDX9::SetupDispatch(&DispatchDX9);
    RenderPassDX9::SetupDispatch(&DispatchDX9);
    CommandBufferDX9::SetupDispatch(&DispatchDX9);

    DispatchDX9.impl_Uninitialize = &dx9_Uninitialize;
    DispatchDX9.impl_Reset = &dx9_Reset;
    DispatchDX9.impl_HostApi = &dx9_HostApi;
    DispatchDX9.impl_NeedRestoreResources = &dx9_NeedRestoreResources;
    DispatchDX9.impl_TextureFormatSupported = &dx9_TextureFormatSupported;
    DispatchDX9.impl_VertexTextureFormatSupported = &dx9_VertexTextureFormatSupported;

    SetDispatchTable(DispatchDX9);

    if (param.maxVertexBufferCount)
        VertexBufferDX9::Init(param.maxVertexBufferCount);
    if (param.maxIndexBufferCount)
        IndexBufferDX9::Init(param.maxIndexBufferCount);
    if (param.maxConstBufferCount)
        ConstBufferDX9::Init(param.maxConstBufferCount);
    if (param.maxTextureCount)
        TextureDX9::Init(param.maxTextureCount);

    uint32 ringBufferSize = 4 * 1024 * 1024;
    if (param.shaderConstRingBufferSize)
        ringBufferSize = param.shaderConstRingBufferSize;
    ConstBufferDX9::InitializeRingBuffer(ringBufferSize);

    stat_DIP = StatSet::AddStat("rhi'dip", "dip");
    stat_DP = StatSet::AddStat("rhi'dp", "dp");
    stat_SET_PS = StatSet::AddStat("rhi'set-ps", "set-ps");
    stat_SET_TEX = StatSet::AddStat("rhi'set-tex", "set-tex");
    stat_SET_CB = StatSet::AddStat("rhi'set-cb", "set-cb");

    MutableDeviceCaps::Get().is32BitIndicesSupported = true;
    MutableDeviceCaps::Get().isFramebufferFetchSupported = true;
    MutableDeviceCaps::Get().isVertexTextureUnitsSupported = true;
    MutableDeviceCaps::Get().isInstancingSupported = true;
    MutableDeviceCaps::Get().isUpperLeftRTOrigin = true;
    MutableDeviceCaps::Get().isZeroBaseClipRange = true;
    MutableDeviceCaps::Get().isCenterPixelMapping = true;
    DX9CheckMultisampleSupport();
}

//==============================================================================
} // namespace rhi
