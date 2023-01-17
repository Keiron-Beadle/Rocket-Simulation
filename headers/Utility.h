#pragma once
#include <vector>
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <d3dcommon.h>
#include <stdexcept>
#include <atlbase.h>
#include <assert.h>
#include <algorithm>
#include <memory>
#include <DirectXMath.h>
#include <cmath>
#include "RenderTarget.h"

//------------------------------------
// Enum Definitions
//------------------------------------

enum MRT_MODE {
    DEFAULT,
    DIFFUSE,
    NORM,
    SUNLIGHT_DEPTH,
    LUMINANCE,
    BLUR,
    LIGHT,
    MOONLIGHT_DEPTH

};
MRT_MODE& operator++(MRT_MODE& mode);

enum RENDER_MODE {
    WIREFRAME,
    DIFFUSE_NON_TEXTURED,
    DIFFUSE_TEXTURED,
    DIFFUSE_TEXTURED_DISPLACEMENT
};
RENDER_MODE& operator++(RENDER_MODE& mode);

//------------------------------------
// Vertex Attrib starts at 1 as I can OR them together if ^2
//------------------------------------
enum VertexAttribute { 
    POSITION = 1,
    NORMAL = 2,
    TEXCOORD = 4,
    TANGENT = 8,
    BINORMAL = 16,
    ICOORDS = 32
};

VertexAttribute operator|=(VertexAttribute& va1, const VertexAttribute& va2);

//------------------------------------
// Buffer Definitions
//------------------------------------
struct MiscCBuffer {
    DirectX::XMFLOAT4 misc;
};

struct LightData {
    DirectX::XMFLOAT4 position; // 16
    DirectX::XMFLOAT4 ambient; // 16
    DirectX::XMFLOAT4 diffuse; // 16
    DirectX::XMFLOAT4 specular; // 16
    DirectX::XMFLOAT4 attenuation; // 16
    DirectX::XMFLOAT4X4 viewProj; // 64
};

struct LightCBuffer {
    LightData light[2];
    DirectX::XMFLOAT4 currentLightCount;
};

struct DrawFrameBuffer {
    DirectX::XMFLOAT4X4 m;
    DirectX::XMFLOAT4X4 mvp;
    //X = Exposure Y = HDR? Z = Animation? W = ??
    DirectX::XMFLOAT4 misc;
};

struct UpdateFrameBuffer {
    DirectX::XMFLOAT4 camPos;
    DirectX::XMFLOAT2 t;
    DirectX::XMFLOAT2 dt;
};

struct ParticleBuffer {
    DirectX::XMFLOAT4 startColour;
    DirectX::XMFLOAT4 endColour;
    DirectX::XMFLOAT4 direction;
    DirectX::XMFLOAT4 emitterPosition;
    DirectX::XMFLOAT4 misc; // x = vel, y = lifespan
};

struct ViewProjBuffer {
    DirectX::XMFLOAT4X4 v;
    DirectX::XMFLOAT4X4 p;
    DirectX::XMFLOAT4X4 invV;
    DirectX::XMFLOAT4X4 invP;
};

//------------------------------------
// Descriptor Definitions
//------------------------------------

struct INPUT_ELEMENT_DESC {
    const uint8_t element;
    const uint32_t format;
};

struct OBJ_DESC {
    LPCWSTR shaderFile;
    const char* modelFile;
    LPCWSTR albedoFile;
    LPCWSTR normalFile;
    LPCWSTR dispFile;
    uint32_t optModelFlags;
};

//Ensure these classes are declared
class TextureManager;
class ModelManager;
class ShaderManager;
struct CONST_OBJ_DATA {
    CComPtr<ID3D11Device> device;
    CComPtr<ID3D11DeviceContext> context;
    std::weak_ptr<ShaderManager> shaderManager;
    std::weak_ptr<ModelManager> modelManager;
    std::weak_ptr<TextureManager> textureManager;
};

//------------------------------------
// Instance Data for VBuffer
//------------------------------------

struct InstanceData {
    InstanceData() 
        : Position(DirectX::XMFLOAT3(0,0,0)), Coords(DirectX::XMFLOAT3(0,0,0)) {}
    InstanceData(const DirectX::XMFLOAT3& p, const DirectX::XMFLOAT3& c) 
        : Position(p), Coords(c) {}
    InstanceData(float px, float py, float pz, float cx, float cy, float cz) 
        : Position(px, py, pz), Coords(cx,cy,cz) {}

    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Coords;
};

//------------------------------------
// Vertex Data for VBuffer
//------------------------------------
struct Vertex {
    Vertex() : Position(DirectX::XMFLOAT3(0,0,0)),
               Normal(DirectX::XMFLOAT3(0,0,0)),
               Tangent(DirectX::XMFLOAT3(0,0,0)),
               Binormal(DirectX::XMFLOAT3(0,0,0)),
               Tex(DirectX::XMFLOAT2(0,0)) {}
    Vertex(
        const DirectX::XMFLOAT3& p,
        const DirectX::XMFLOAT3& n,
        const DirectX::XMFLOAT3& t,
        const DirectX::XMFLOAT3& bn,
        const DirectX::XMFLOAT2& uv) :
        Position(p),
        Normal(n),
        Tangent(t),
        Binormal(bn),
        Tex(uv) {}
    Vertex(
        float px, float py, float pz,
        float nx, float ny, float nz,
        float tx, float ty, float tz,
        float bx, float by, float bz,
        float u, float v) :
        Position(px, py, pz),
        Normal(nx, ny, nz),
        Tangent(tx, ty, tz),
        Binormal(bx,by,bz),
        Tex(u, v) {}

    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 Tangent;
    DirectX::XMFLOAT3 Binormal;
    DirectX::XMFLOAT2 Tex;
};


struct SimpleVertex {
    SimpleVertex():Position(DirectX::XMFLOAT3(0,0,0))
        , Tex(DirectX::XMFLOAT2(0,0))
    {}

    SimpleVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 tex)
        : Position(pos), Tex(tex){}

    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 Tex;
};

//------------------------------------
// Mesh Data for Model
//------------------------------------
struct MeshData {
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
};

template <class T>
inline void updateD11Buffer(ID3D11Buffer* gcBuffer, const T& cbuffer, ID3D11DeviceContext* ctx) {
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    if (SUCCEEDED(ctx->Map(gcBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
        int a = sizeof(cbuffer);
        memcpy(mappedResource.pData, &cbuffer, sizeof(cbuffer));
        ctx->Unmap(gcBuffer, 0);
    }
    else {
        throw std::exception("[E] Updating D11 Buffer.");
    }
}

std::vector<D3D11_INPUT_ELEMENT_DESC> createInputElementDesc(const uint8_t elements);

HRESULT compileShaderFromFile(LPCWSTR szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

void createGBuffer(const CComPtr<ID3D11Device>&, const int, const int, RenderTarget&);

constexpr float FLEQ_EPSILON = 0.001f;
inline bool fleq(const float f1, const float f2) { return std::fabs(f1 - f2) < FLEQ_EPSILON; }