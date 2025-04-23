#pragma once
#include <memory>
#include "Texture.h"
#include "Container/Map.h"

enum EShadowFilter
{
    ESF_VSM = 0,
    ESF_PCF,
};

class FRenderer;
class FGraphicsDevice;
class FResourceMgr
{

public:
    void Initialize(FRenderer* renderer, FGraphicsDevice* device);
    void Release(FRenderer* renderer);
    HRESULT LoadTextureFromFile(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename);
    HRESULT LoadTextureFromDDS(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename);

    std::shared_ptr<FTexture> GetTexture(const FWString& name) const;

    /**
     * Todo: 미안합니다. 그림자 필터 모드 여기다 썼어요.
     */
    int ShaderFilterMode = ESF_VSM;
    
private:
    TMap<FWString, std::shared_ptr<FTexture>> textureMap;
};
