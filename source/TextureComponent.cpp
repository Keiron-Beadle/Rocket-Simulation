#include "TextureComponent.h"

TextureComponent::TextureComponent() : AComponent(COMPONENT_TEXTURE)
{
}

TextureComponent::TextureComponent(const char* albedoFile, const char* normalFile, const char* displacementFile, bool shouldMips,
	const CComPtr<ID3D11Device>& device, const CComPtr<ID3D11DeviceContext>& context)
	: AComponent(COMPONENT_TEXTURE) {

	const size_t albedoCSize = strlen(albedoFile)+1;
	wchar_t* albedoWC = new wchar_t[albedoCSize];
	size_t outSize;
	mbstowcs_s(&outSize, albedoWC, albedoCSize, albedoFile, albedoCSize-1);
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
	srvd.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	auto albedoRV = CComPtr<ID3D11ShaderResourceView>();
	HRESULT hr = DirectX::CreateDDSTextureFromFile(device, albedoWC, nullptr, &_albedo.p);
	delete[] albedoWC;
	if (FAILED(hr)) { throw std::exception("[E] Creating albedo texture from file in Texture Component."); }

	if (normalFile) {
		const size_t normalCSize = strlen(normalFile) + 1;
		wchar_t* normalWC = new wchar_t[normalCSize];
		mbstowcs_s(&outSize, normalWC, normalCSize,normalFile, normalCSize-1);
		auto normalRV = CComPtr<ID3D11ShaderResourceView>();
		HRESULT hr = DirectX::CreateDDSTextureFromFile(device, normalWC, nullptr, &_normal.p);
		delete[] normalWC;
		if (FAILED(hr)) { throw std::exception("[E] Creating normal texture from file in Texture Component."); }
	}

	if (displacementFile) {
		const size_t displacementCSize = strlen(displacementFile) + 1;
		wchar_t* displacementWC = new wchar_t[displacementCSize];
		mbstowcs_s(&outSize, displacementWC, displacementCSize,displacementFile, displacementCSize-1);
		auto displacementRV = CComPtr<ID3D11ShaderResourceView>();
		HRESULT hr = DirectX::CreateDDSTextureFromFile(device, displacementWC, nullptr, &_displacement.p);
		delete[] displacementWC;
		if (FAILED(hr)) { throw std::exception("[E] Creating displacement texture from file in Texture Component."); }
	}

}

TextureComponent::TextureComponent(const TextureComponent& tc) : AComponent(COMPONENT_TEXTURE)
{
	deepCopy(tc);
}

TextureComponent& TextureComponent::operator=(const TextureComponent& tc)
{
	if (this == &tc) return *this;
	deepCopy(tc);
	return *this;
}

TextureComponent::TextureComponent(TextureComponent&& tc) noexcept : AComponent(COMPONENT_TEXTURE)
{
	moveCopy(std::move(tc));
}

TextureComponent& TextureComponent::operator=(TextureComponent&& tc) noexcept
{
	if (this == &tc) return *this;
	moveCopy(std::move(tc));
	return *this;
}

void TextureComponent::onAwake(Entity& e, const CComPtr<ID3D11Device>& device)
{
}

void TextureComponent::onPropertyChanged(const AComponent& component)
{
}

void TextureComponent::deepCopy(const TextureComponent& tc)
{
	_albedo = tc._albedo;
	_normal = tc._normal;
	_displacement = tc._displacement;
}

void TextureComponent::moveCopy(TextureComponent&& tc) noexcept
{
	_albedo.Attach(tc._albedo);
	_normal.Attach(tc._normal);
	_displacement.Attach(tc._displacement);
	tc._albedo.Detach();
	tc._normal.Detach();
	tc._displacement.Detach();
}