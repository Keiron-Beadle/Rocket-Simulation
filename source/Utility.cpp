#include "Utility.h"

using namespace DirectX;

MRT_MODE& operator++(MRT_MODE& mode) {
    switch (mode) {
    case MRT_MODE::DEFAULT:
        return mode = MRT_MODE::DIFFUSE;
    case MRT_MODE::DIFFUSE:
        return mode = MRT_MODE::NORM;
    case MRT_MODE::NORM:
        return mode = MRT_MODE::SUNLIGHT_DEPTH;
    case MRT_MODE::SUNLIGHT_DEPTH:
        return mode = MRT_MODE::LUMINANCE;
    case MRT_MODE::LUMINANCE:
        return mode = MRT_MODE::BLUR;
    case MRT_MODE::BLUR:
        return mode = MRT_MODE::LIGHT;
    case MRT_MODE::LIGHT:
        return mode = MRT_MODE::MOONLIGHT_DEPTH;
    case MRT_MODE::MOONLIGHT_DEPTH:
        return mode = MRT_MODE::DEFAULT;
    default:
        return mode = MRT_MODE::DEFAULT;
    }
}

RENDER_MODE& operator++(RENDER_MODE& mode) {
    switch (mode) {
    case RENDER_MODE::WIREFRAME:
        return mode = RENDER_MODE::DIFFUSE_NON_TEXTURED;
    case RENDER_MODE::DIFFUSE_NON_TEXTURED:
        return mode = RENDER_MODE::DIFFUSE_TEXTURED;
    case RENDER_MODE::DIFFUSE_TEXTURED:
        return mode = RENDER_MODE::DIFFUSE_TEXTURED_DISPLACEMENT;
    case RENDER_MODE::DIFFUSE_TEXTURED_DISPLACEMENT:
        return mode = RENDER_MODE::WIREFRAME;
    default:
        return mode = RENDER_MODE::WIREFRAME;
    }
}

VertexAttribute operator|=(VertexAttribute& va1, const VertexAttribute& va2) {
    return va1 = static_cast<VertexAttribute>(va1 | va2);
}

HRESULT compileShaderFromFile(LPCWSTR szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	auto dwShaderFlags = static_cast<DWORD>(D3DCOMPILE_ENABLE_STRICTNESS);
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	CComPtr <ID3DBlob> pErrorBlob;
	const auto hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr)) {
		if (pErrorBlob)
			OutputDebugStringA(static_cast<const char*>(pErrorBlob->GetBufferPointer()));
		return hr;
	}

	return S_OK;
}

std::vector<D3D11_INPUT_ELEMENT_DESC> createInputElementDesc(const uint8_t elements) {
    std::vector<D3D11_INPUT_ELEMENT_DESC> finalDesc;
    UINT vOffset = 0;
    if ((elements & VertexAttribute::POSITION) != 0) {
        finalDesc.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, vOffset, D3D11_INPUT_PER_VERTEX_DATA,0 });
        vOffset += 12;
    }
    if ((elements & VertexAttribute::TEXCOORD) != 0) {
        finalDesc.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, vOffset, D3D11_INPUT_PER_VERTEX_DATA,0 });
        vOffset += 8;
    }
    if ((elements & VertexAttribute::NORMAL) != 0) {
        finalDesc.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, vOffset, D3D11_INPUT_PER_VERTEX_DATA,0 });
        vOffset += 12;
    }
    if ((elements & VertexAttribute::BINORMAL) != 0) {
        finalDesc.push_back({ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, vOffset, D3D11_INPUT_PER_VERTEX_DATA,0 });
        vOffset += 12;
    }
    if ((elements & VertexAttribute::TANGENT) != 0) {
        finalDesc.push_back({ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, vOffset, D3D11_INPUT_PER_VERTEX_DATA,0 });
        vOffset += 12;
    }
    if ((elements & VertexAttribute::ICOORDS) != 0) {
        finalDesc.push_back({ "ICOORDS", 0, DXGI_FORMAT_R32_UINT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 });
    }
    return finalDesc;
}

void createGBuffer(const CComPtr<ID3D11Device>& d, const int w, const int h, RenderTarget& rt)
{
    D3D11_TEXTURE2D_DESC rtd{};
    ZeroMemory(&rtd, sizeof(rtd));
    rtd.Width = w;
    rtd.Height = h;
    rtd.MipLevels = 0;
    rtd.ArraySize = 1;
    rtd.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    rtd.SampleDesc.Count = 1;
    rtd.Usage = D3D11_USAGE_DEFAULT;
    rtd.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    rtd.CPUAccessFlags = 0;
    rtd.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    HRESULT hr = d->CreateTexture2D(&rtd, nullptr, &rt.getTexture().p);
    if (FAILED(hr)) { throw std::exception("Failed to create Texture in GBuffer."); }

    D3D11_RENDER_TARGET_VIEW_DESC rtvd{};
    ZeroMemory(&rtvd, sizeof(rtvd));
    rtvd.Format = rtd.Format;
    rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvd.Texture2D.MipSlice = 0;
    hr = d->CreateRenderTargetView(rt.getTexture(), &rtvd, &rt.getRTV().p);
    if (FAILED(hr)) { throw std::exception("Failed to create Render Target View in GBuffer."); }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
    ZeroMemory(&srvd, sizeof(srvd));
    srvd.Format = rtd.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MostDetailedMip = 0;
    srvd.Texture2D.MipLevels = -1;
    hr = d->CreateShaderResourceView(rt.getTexture(), &srvd, &rt.getSRV().p);
    if (FAILED(hr)) { throw std::exception("Failed to create Shader Resource View in GBuffer."); }

}