#pragma once
#include "AComponent.h"
#include "../BoxCollider.h"
#include "../Utility.h"

class TerrainComponent : public AComponent {
private:
	static constexpr UINT _stride = sizeof(uint32_t);
	static constexpr UINT _offset = 0;
	std::vector<std::vector<std::vector<uint8_t>>> _activeVoxels;
	std::vector<BoxCollider> _colliders;
	XMFLOAT3 _instanceDimensions;
	XMFLOAT3 _instanceOffsets;
	size_t _instanceCount;
	CComPtr<ID3D11Buffer> _instanceBuffer, _gcVoxelBuffer;
	MiscCBuffer _cVoxelBuffer;
public:
	TerrainComponent(const XMFLOAT3 dimensions, const XMFLOAT3 offsets);
	~TerrainComponent();
	TerrainComponent(const TerrainComponent&);
	TerrainComponent operator=(const TerrainComponent&);
	TerrainComponent(TerrainComponent&&) noexcept;
	TerrainComponent operator=(TerrainComponent&&) noexcept;
	void onAwake(Entity& e, const CComPtr<ID3D11Device>& device) override;
	void setInstanceBuffer(const CComPtr<ID3D11DeviceContext>&, UINT,UINT);
	void updateInstanceBuffer(const CComPtr<ID3D11DeviceContext>& context);
	void updateGrid(const XMFLOAT3& collisionPosition, const float radius);
	const size_t getInstanceCount() const { return _instanceCount; }
	const std::vector<BoxCollider> getColliders() const { return _colliders; }
protected:
	void onPropertyChanged(const AComponent& component) override;
private:
	void checkVoxel(const uint8_t i, const uint8_t j, const uint8_t k, const XMFLOAT3& collisionPos, const float radius);
	void checkNeighbourVoxels(const uint8_t i, const uint8_t j, const uint8_t k, const XMFLOAT3& collisionPos, const float radius);
	void deepCopy(const TerrainComponent&);
	void moveCopy(TerrainComponent&&) noexcept;
};