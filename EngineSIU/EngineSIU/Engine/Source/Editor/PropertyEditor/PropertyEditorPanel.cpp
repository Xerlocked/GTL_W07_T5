#include "PropertyEditorPanel.h"

#include "UnrealClient.h"
#include "World/World.h"
#include "Actors/Player.h"
#include "Components/Light/LightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "Components/Light/AmbientLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextComponent.h"
#include "Engine/EditorEngine.h"
#include "Engine/FLoaderOBJ.h"
#include "UnrealEd/ImGuiWidget.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"
#include "Engine/Engine.h"
#include "Components/HeightFogComponent.h"
#include "Components/ProjectileMovementComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/AssetManager.h"
#include "LevelEditor/SLevelEditor.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"

void PropertyEditorPanel::Render()
{
    /* Pre Setup */
    float PanelWidth = (Width) * 0.2f - 6.0f;
    float PanelHeight = (Height) * 0.65f;

    float PanelPosX = (Width) * 0.8f + 5.0f;
    float PanelPosY = (Height) * 0.3f + 15.0f;

    ImVec2 MinSize(140, 370);
    ImVec2 MaxSize(FLT_MAX, 900);

    FViewportResource* ViewportResource = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetViewportResource();

    /* Min, Max Size */
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

    /* Panel Position */
    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);

    /* Panel Size */
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    /* Render Start */
    ImGui::Begin("Detail", nullptr, PanelFlags);

    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
        return;
    AEditorPlayer* player = Engine->GetEditorPlayer();
    AActor* PickedActor = Engine->GetSelectedActor();
    if (PickedActor)
    {
        ImGui::SetItemDefaultFocus();
        // TreeNode 배경색을 변경 (기본 상태)
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
        {
            Location = PickedActor->GetActorLocation();
            Rotation = PickedActor->GetActorRotation();
            Scale = PickedActor->GetActorScale();

            FImGuiWidget::DrawVec3Control("Location", Location, 0, 85);
            ImGui::Spacing();

            FImGuiWidget::DrawRot3Control("Rotation", Rotation, 0, 85);
            ImGui::Spacing();

            FImGuiWidget::DrawVec3Control("Scale", Scale, 0, 85);
            ImGui::Spacing();

            PickedActor->SetActorLocation(Location);
            PickedActor->SetActorRotation(Rotation);
            PickedActor->SetActorScale(Scale);

            std::string coordiButtonLabel;
            if (player->GetCoordMode() == ECoordMode::CDM_WORLD)
                coordiButtonLabel = "World";
            else if (player->GetCoordMode() == ECoordMode::CDM_LOCAL)
                coordiButtonLabel = "Local";

            if (ImGui::Button(coordiButtonLabel.c_str(), ImVec2(ImGui::GetWindowContentRegionMax().x * 0.9f, 32)))
            {
                player->AddCoordiMode();
            }
            ImGui::TreePop(); // 트리 닫기
        }
        ImGui::PopStyleColor();
    }

    if (PickedActor)
    {
        if (ImGui::Button("Duplicate"))
        {
            UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
            AActor* NewActor = Engine->ActiveWorld->DuplicateActor(Engine->GetSelectedActor());
            Engine->SelectActor(NewActor);
        }
    }

    //// TODO: 추후에 RTTI를 이용해서 프로퍼티 출력하기
    //if (PickedActor)
    //    if (ULightComponent* lightObj = PickedActor->GetComponentByClass<ULightComponent>())
    //    {
    //        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

    //        if (ImGui::TreeNodeEx("Light Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    //        {
    //            /*  DrawColorProperty("Ambient Color",
    //                  [&]() { return lightObj->GetAmbientColor(); },
    //                  [&](FVector4 c) { lightObj->SetAmbientColor(c); });
    //              */
    //            DrawColorProperty("Base Color",
    //                [&]() { return lightObj->GetDiffuseColor(); },
    //                [&](FLinearColor c) { lightObj->SetDiffuseColor(c); });

    //            float Intensity = lightObj->GetIntensity();
    //            if (ImGui::SliderFloat("Intensity", &Intensity, 0.0f, 100.0f, "%1.f"))
    //                lightObj->SetIntensity(Intensity);

    //             /*  
    //            float falloff = lightObj->GetFalloff();
    //            if (ImGui::SliderFloat("Falloff", &falloff, 0.1f, 10.0f, "%.2f")) {
    //                lightObj->SetFalloff(falloff);
    //            }

    //            TODO : For SpotLight
    //            */

    //            float attenuation = lightObj->GetAttenuation();
    //            if (ImGui::SliderFloat("Attenuation", &attenuation, 0.01f, 10000.f, "%.1f")) {
    //                lightObj->SetAttenuation(attenuation);
    //            }

    //            float AttenuationRadius = lightObj->GetAttenuationRadius();
    //            if (ImGui::SliderFloat("Attenuation Radius", &AttenuationRadius, 0.01f, 10000.f, "%.1f")) {
    //                lightObj->SetAttenuationRadius(AttenuationRadius);
    //            }

    //            ImGui::TreePop();
    //        }

    //        ImGui::PopStyleColor();
    //    }

    if(PickedActor)
        if (UPointLightComponent* pointlightObj = PickedActor->GetComponentByClass<UPointLightComponent>())
        {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

            if (ImGui::TreeNodeEx("PointLight Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawColorProperty("Light Color",
                    [&]() { return pointlightObj->GetLightColor(); },
                    [&](FLinearColor c) { pointlightObj->SetLightColor(c); });

                float Intensity = pointlightObj->GetIntensity();
                if (ImGui::DragFloat("Intensity", &Intensity, 0.01f, 0.0f, 10000.0f, "%.3f"))
                    pointlightObj->SetIntensity(Intensity);

                float Radius = pointlightObj->GetAttenuationRadius();
                if (ImGui::DragFloat("Radius", &Radius, 0.01f, 0.01f, 2000.f, "%.3f")) {
                    pointlightObj->SetAttenuationRadius(Radius);
                }

                float Falloff = pointlightObj->GetFalloff();
                if (ImGui::DragFloat("Falloff", &Falloff, 0.01f, 0.01f, 10.f, "%.3f")) {
                    pointlightObj->SetFalloff(Falloff);
                }

                if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Spacing();

                    
                    if (ImGui::Checkbox("Cast Shadow##Point", &pointlightObj->bCastShadow))
                    {
                        
                    }

                    int ShadowResolutionScale = pointlightObj->ShadowResolutionScale;
                    if (ImGui::DragInt("Resolution##Point", &ShadowResolutionScale, 1024, 1024, 4096, "%d")) {
                        pointlightObj->ShadowResolutionScale = ShadowResolutionScale;
                    }

                    float ShadowBias = pointlightObj->ShadowBias;
                    if (ImGui::DragFloat("Bias##Point", &ShadowBias, 0.1f, 0.0f, 10.f, "%.1f")) {
                        pointlightObj->ShadowBias = ShadowBias;
                    }

                    float ShadowSlopeBias = pointlightObj->ShadowSlopeBias;
                    if (ImGui::DragFloat("Slope Bias##Point", &ShadowSlopeBias, 0.1f, 0.0f, 10.f, "%.1f")) {
                        pointlightObj->ShadowSlopeBias = ShadowSlopeBias;
                    }

                    int32 ShadowSharpen = pointlightObj->ShadowSharpen;
                    if (ImGui::DragInt("Sharpen##Point", &ShadowSharpen, 1, 0, 12, "%d")) {
                        pointlightObj->ShadowSharpen = ShadowSharpen;
                    }
                    
                    ImGui::Columns(3, nullptr, false);
                    for (int i = 0; i < 6; ++i)
                    {
                        ImGui::Text("Face %d", i);
                        ID3D11ShaderResourceView* SRV = ViewportResource->GetPointShadowMapFaceSRV(pointlightObj->index *6 + i);
                        ImTextureID TexID = reinterpret_cast<ImTextureID>(SRV);
                        if (ImGui::ImageButton("PointTexture##" + i, TexID, ImVec2(96, 96)))
                        {
                            // do something if clicked the texture.
                        }
                        ImGui::NextColumn();
                    }
                    ImGui::Columns(1);
                }
                
                ImGui::TreePop();
            }

            ImGui::PopStyleColor();
        }

    if(PickedActor)
        if (USpotLightComponent* spotlightObj = PickedActor->GetComponentByClass<USpotLightComponent>())
        {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

            if (ImGui::TreeNodeEx("SpotLight Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImVec2 ParentSize = ImGui::GetItemRectSize();
                
                DrawColorProperty("Light Color",
                    [&]() { return spotlightObj->GetLightColor(); },
                    [&](FLinearColor c) { spotlightObj->SetLightColor(c); });

                float Intensity = spotlightObj->GetIntensity();
                if (ImGui::DragFloat("Intensity", &Intensity, 0.1f, 0.0f, 10000.0f, "%.3f"))
                    spotlightObj->SetIntensity(Intensity);

                float Radius = spotlightObj->GetAttenuationRadius();
                if (ImGui::DragFloat("Radius", &Radius, 0.1f, 0.01f, 2000.f, "%.3f")) {
                    spotlightObj->SetAttenuationRadius(Radius);
                }

                LightDirection = spotlightObj->GetDirection();
                FImGuiWidget::DrawVec3Control("Direction", LightDirection, 0, 85);
                
                float InnerDegree = spotlightObj->GetInnerDegree();
                if (ImGui::DragFloat("InnerDegree", &InnerDegree, 0.1f, 0.0f, 80.f, "%.3f")) {
                    spotlightObj->SetInnerDegree(InnerDegree);
                }

                float OuterDegree = spotlightObj->GetOuterDegree();
                if (ImGui::DragFloat("OuterDegree", &OuterDegree, 0.1f, 0.0f, 80.f, "%.3f")) {
                    spotlightObj->SetOuterDegree(OuterDegree);
                }

                float Falloff = spotlightObj->GetFalloff();
                if (ImGui::DragFloat("Falloff", &Falloff, 0.01f, 0.01f, 10.f, "%.2f")) {
                    spotlightObj->SetFalloff(Falloff);
                }


                if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Spacing();

                    
                    if (ImGui::Checkbox("Cast Shadow##Spot", &spotlightObj->bCastShadow))
                    {
                        
                    }

                    int ShadowResolutionScale = spotlightObj->ShadowResolutionScale;
                    if (ImGui::DragInt("Resolution##Spot", &ShadowResolutionScale, 1024, 1024, 4096, "%d")) {
                        spotlightObj->ShadowResolutionScale = ShadowResolutionScale;
                    }

                    float ShadowBias = spotlightObj->ShadowBias;
                    if (ImGui::DragFloat("Bias##Spot", &ShadowBias, 0.1f, 0.0f, 10.f, "%.1f")) {
                        spotlightObj->ShadowBias = ShadowBias;
                    }

                    float ShadowSlopeBias = spotlightObj->ShadowSlopeBias;
                    if (ImGui::DragFloat("Slope Bias##Spot", &ShadowSlopeBias, 0.1f, 0.0f, 10.f, "%.1f")) {
                        spotlightObj->ShadowSlopeBias = ShadowSlopeBias;
                    }

                    int32 ShadowSharpen = spotlightObj->ShadowSharpen;
                    if (ImGui::DragInt("Sharpen##Spot", &ShadowSharpen, 1, 0, 12, "%d")) {
                        spotlightObj->ShadowSharpen = ShadowSharpen;
                    }

                    ID3D11ShaderResourceView* SpotSRV = ViewportResource->GetSpotShadowMapSRV(); 

                    ImTextureID SRVID = reinterpret_cast<ImTextureID>(SpotSRV);

                    ImGui::Image(SRVID, ImVec2(ParentSize.x - 154, ParentSize.x - 154));

                    if (ImGui::Button("Show the light's perspective"))
                    {
                        const FVector SpotDirection = spotlightObj->GetDirection();
                        const FVector SpotLocation = spotlightObj->GetWorldLocation() + SpotDirection * 1.5f;
                    
                        GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->SetCameraLocation(SpotLocation);
                        GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->CameraLookAt(SpotDirection);
                    }
                }
                
                ImGui::TreePop();
            }

            ImGui::PopStyleColor();
        }

    if (PickedActor)
        if (UDirectionalLightComponent* dirlightObj = PickedActor->GetComponentByClass<UDirectionalLightComponent>())
        {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

            if (ImGui::TreeNodeEx("DirectionalLight Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImVec2 ParentSize = ImGui::GetItemRectSize();
                
                DrawColorProperty("Light Color",
                    [&]() { return dirlightObj->GetLightColor(); },
                    [&](FLinearColor c) { dirlightObj->SetLightColor(c); });

                float Intensity = dirlightObj->GetIntensity();
                if (ImGui::DragFloat("Intensity", &Intensity,0.03f, 0.0f, 150.0f, "%.3f"))
                    dirlightObj->SetIntensity(Intensity);

                LightDirection = dirlightObj->GetDirection();
                FImGuiWidget::DrawVec3Control("Direction", LightDirection, 0, 85);


                if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Spacing();

                    
                    if (ImGui::Checkbox("Cast Shadow##Directional", &dirlightObj->bCastShadow))
                    {
                        
                    }

                    int ShadowResolutionScale = dirlightObj->ShadowResolutionScale;
                    if (ImGui::DragInt("Resolution##Directional", &ShadowResolutionScale, 1024, 1024, 4096, "%d")) {
                        dirlightObj->ShadowResolutionScale = ShadowResolutionScale;
                    }

                    float ShadowBias = dirlightObj->ShadowBias;
                    if (ImGui::DragFloat("Bias##Directional", &ShadowBias, 0.1f, 0.0f, 10.f, "%.1f")) {
                        dirlightObj->ShadowBias = ShadowBias;
                    }

                    float ShadowSlopeBias = dirlightObj->ShadowSlopeBias;
                    if (ImGui::DragFloat("Slope Bias##Directional", &ShadowSlopeBias, 0.1f, 0.0f, 10.f, "%.1f")) {
                        dirlightObj->ShadowSlopeBias = ShadowSlopeBias;
                    }

                    int32 ShadowSharpen = dirlightObj->ShadowSharpen;
                    if (ImGui::DragInt("Sharpen##Directional", &ShadowSharpen, 1, 0, 12, "%d")) {
                        dirlightObj->ShadowSharpen = ShadowSharpen;
                    }
                                    
                    ID3D11ShaderResourceView* DirectionalSRV = ViewportResource->GetDirectionalShadowMapSRV(); 

                    ImTextureID SRVID = reinterpret_cast<ImTextureID>(DirectionalSRV);
                
                    ImGui::Image(SRVID, ImVec2(ParentSize.x - 154, ParentSize.x - 154));
                
                }

                ImGui::TreePop();
            }

            ImGui::PopStyleColor();
        }

    if(PickedActor)
        if (UAmbientLightComponent* ambientLightObj = PickedActor->GetComponentByClass<UAmbientLightComponent>())
        {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

            if (ImGui::TreeNodeEx("AmbientLight Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawColorProperty("Light Color",
                    [&]() { return ambientLightObj->GetLightColor(); },
                    [&](FLinearColor c) { ambientLightObj->SetLightColor(c); });
                ImGui::TreePop();
            }

            ImGui::PopStyleColor();
        }

    if (PickedActor)
        if (UProjectileMovementComponent* ProjectileComp = (PickedActor->GetComponentByClass<UProjectileMovementComponent>()))
        {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

            if (ImGui::TreeNodeEx("Projectile Movement Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
            {
                float InitialSpeed = ProjectileComp->GetInitialSpeed();
                if (ImGui::InputFloat("InitialSpeed", &InitialSpeed, 0.f, 10000.0f, "%.1f"))
                    ProjectileComp->SetInitialSpeed(InitialSpeed);

                float MaxSpeed = ProjectileComp->GetMaxSpeed();
                if (ImGui::InputFloat("MaxSpeed", &MaxSpeed, 0.f, 10000.0f, "%.1f"))
                    ProjectileComp->SetMaxSpeed(MaxSpeed);

                float Gravity = ProjectileComp->GetGravity();
                if (ImGui::InputFloat("Gravity", &Gravity, 0.f, 10000.f, "%.1f"))
                    ProjectileComp->SetGravity(Gravity); 
                
                float ProjectileLifetime = ProjectileComp->GetLifetime();
                if (ImGui::InputFloat("Lifetime", &ProjectileLifetime, 0.f, 10000.f, "%.1f"))
                    ProjectileComp->SetLifetime(ProjectileLifetime);

                FVector currentVelocity = ProjectileComp->GetVelocity();

                float velocity[3] = { currentVelocity.X, currentVelocity.Y, currentVelocity.Z };

                if (ImGui::InputFloat3("Velocity", velocity, "%.1f")) {
                    ProjectileComp->SetVelocity(FVector(velocity[0], velocity[1], velocity[2]));
                }
                
                ImGui::TreePop();
            }

            ImGui::PopStyleColor();
        }
    // TODO: 추후에 RTTI를 이용해서 프로퍼티 출력하기
    if (PickedActor)
        if (UTextComponent* textOBj = Cast<UTextComponent>(PickedActor->GetRootComponent()))
        {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            if (ImGui::TreeNodeEx("Text Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
            {
                if (textOBj) {
                    textOBj->SetTexture(L"Assets/Texture/font.png");
                    textOBj->SetRowColumnCount(106, 106);
                    FWString wText = textOBj->GetText();
                    int len = WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, nullptr, 0, nullptr, nullptr);
                    std::string u8Text(len, '\0');
                    WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, u8Text.data(), len, nullptr, nullptr);

                    static char buf[256];
                    strcpy_s(buf, u8Text.c_str());

                    ImGui::Text("Text: ", buf);
                    ImGui::SameLine();
                    ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
                    if (ImGui::InputText("##Text", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        textOBj->ClearText();
                        int wlen = MultiByteToWideChar(CP_UTF8, 0, buf, -1, nullptr, 0);
                        FWString newWText(wlen, L'\0');
                        MultiByteToWideChar(CP_UTF8, 0, buf, -1, newWText.data(), wlen);
                        textOBj->SetText(newWText.c_str());
                    }
                    ImGui::PopItemFlag();
                }
                ImGui::TreePop();
            }
            ImGui::PopStyleColor();
        }

    // TODO: 추후에 RTTI를 이용해서 프로퍼티 출력하기
    if (PickedActor)
        if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PickedActor->GetRootComponent()))
        {
            RenderForStaticMesh(StaticMeshComponent);
            RenderForMaterial(StaticMeshComponent);
        }

    if (PickedActor)
        if (UHeightFogComponent* FogComponent = Cast<UHeightFogComponent>(PickedActor->GetRootComponent()))
        {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            if (ImGui::TreeNodeEx("Exponential Height Fog", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
            {
                FLinearColor currColor = FogComponent->GetFogColor();

                float r = currColor.R;
                float g = currColor.G;
                float b = currColor.B;
                float a = currColor.A;
                float h, s, v;
                float lightColor[4] = { r, g, b, a };

                // Fog Color
                if (ImGui::ColorPicker4("##Fog Color", lightColor,
                    ImGuiColorEditFlags_DisplayRGB |
                    ImGuiColorEditFlags_NoSidePreview |
                    ImGuiColorEditFlags_NoInputs |
                    ImGuiColorEditFlags_Float))

                {

                    r = lightColor[0];
                    g = lightColor[1];
                    b = lightColor[2];
                    a = lightColor[3];
                    FogComponent->SetFogColor(FLinearColor(r, g, b, a));
                }
                RGBToHSV(r, g, b, h, s, v);
                // RGB/HSV
                bool changedRGB = false;
                bool changedHSV = false;

                // RGB
                ImGui::PushItemWidth(50.0f);
                if (ImGui::DragFloat("R##R", &r, 0.001f, 0.f, 1.f)) changedRGB = true;
                ImGui::SameLine();
                if (ImGui::DragFloat("G##G", &g, 0.001f, 0.f, 1.f)) changedRGB = true;
                ImGui::SameLine();
                if (ImGui::DragFloat("B##B", &b, 0.001f, 0.f, 1.f)) changedRGB = true;
                ImGui::Spacing();

                // HSV
                if (ImGui::DragFloat("H##H", &h, 0.1f, 0.f, 360)) changedHSV = true;
                ImGui::SameLine();
                if (ImGui::DragFloat("S##S", &s, 0.001f, 0.f, 1)) changedHSV = true;
                ImGui::SameLine();
                if (ImGui::DragFloat("V##V", &v, 0.001f, 0.f, 1)) changedHSV = true;
                ImGui::PopItemWidth();
                ImGui::Spacing();

                if (changedRGB && !changedHSV)
                {
                    // RGB -> HSV
                    RGBToHSV(r, g, b, h, s, v);
                    FogComponent->SetFogColor(FLinearColor(r, g, b, a));
                }
                else if (changedHSV && !changedRGB)
                {
                    // HSV -> RGB
                    HSVToRGB(h, s, v, r, g, b);
                    FogComponent->SetFogColor(FLinearColor(r, g, b, a));
                }

                float FogDensity = FogComponent->GetFogDensity();
                if (ImGui::SliderFloat("Density", &FogDensity, 0.00f, 3.0f))
                {
                    FogComponent->SetFogDensity(FogDensity);
                }

                float FogDistanceWeight = FogComponent->GetFogDistanceWeight();
                if (ImGui::SliderFloat("Distance Weight", &FogDistanceWeight, 0.00f, 3.0f))
                {
                    FogComponent->SetFogDistanceWeight(FogDistanceWeight);
                }

                float FogHeightFallOff = FogComponent->GetFogHeightFalloff();
                if (ImGui::SliderFloat("Height Fall Off", &FogHeightFallOff, 0.001f, 0.15f))
                {
                    FogComponent->SetFogHeightFalloff(FogHeightFallOff);
                }

                float FogStartDistance = FogComponent->GetStartDistance();
                if (ImGui::SliderFloat("Start Distance", &FogStartDistance, 0.00f, 50.0f))
                {
                    FogComponent->SetStartDistance(FogStartDistance);
                }

                float FogEndtDistance = FogComponent->GetEndDistance();
                if (ImGui::SliderFloat("End Distance", &FogEndtDistance, 0.00f, 50.0f))
                {
                    FogComponent->SetEndDistance(FogEndtDistance);
                }

                ImGui::TreePop();
            }
            ImGui::PopStyleColor();
        }
    ImGui::End();
}

void PropertyEditorPanel::RGBToHSV(float r, float g, float b, float& h, float& s, float& v) const
{
    float mx = FMath::Max(r, FMath::Max(g, b));
    float mn = FMath::Min(r, FMath::Min(g, b));
    float delta = mx - mn;

    v = mx;

    if (mx == 0.0f) {
        s = 0.0f;
        h = 0.0f;
        return;
    }
    else {
        s = delta / mx;
    }

    if (delta < 1e-6) {
        h = 0.0f;
    }
    else {
        if (r >= mx) {
            h = (g - b) / delta;
        }
        else if (g >= mx) {
            h = 2.0f + (b - r) / delta;
        }
        else {
            h = 4.0f + (r - g) / delta;
        }
        h *= 60.0f;
        if (h < 0.0f) {
            h += 360.0f;
        }
    }
}

void PropertyEditorPanel::HSVToRGB(float h, float s, float v, float& r, float& g, float& b) const
{
    // h: 0~360, s:0~1, v:0~1
    float c = v * s;
    float hp = h / 60.0f;             // 0~6 구간
    float x = c * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
    float m = v - c;

    if (hp < 1.0f) { r = c;  g = x;  b = 0.0f; }
    else if (hp < 2.0f) { r = x;  g = c;  b = 0.0f; }
    else if (hp < 3.0f) { r = 0.0f; g = c;  b = x; }
    else if (hp < 4.0f) { r = 0.0f; g = x;  b = c; }
    else if (hp < 5.0f) { r = x;  g = 0.0f; b = c; }
    else { r = c;  g = 0.0f; b = x; }

    r += m;  g += m;  b += m;
}

void PropertyEditorPanel::RenderForStaticMesh(UStaticMeshComponent* StaticMeshComp) const
{
    if (StaticMeshComp->GetStaticMesh() == nullptr)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Static Mesh", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        ImGui::Text("StaticMesh");
        ImGui::SameLine();

        FString PreviewName = StaticMeshComp->GetStaticMesh()->GetRenderData()->DisplayName;
        const TMap<FName, FAssetInfo> Assets = UAssetManager::Get().GetAssetRegistry();

        if (ImGui::BeginCombo("##StaticMesh", GetData(PreviewName), ImGuiComboFlags_None))
        {
            for (const auto& Asset : Assets)
            {
                if (ImGui::Selectable(GetData(Asset.Value.AssetName.ToString()), false))
                {
                    FString MeshName = Asset.Value.PackagePath.ToString() + "/" + Asset.Value.AssetName.ToString();
                    UStaticMesh* StaticMesh = FManagerOBJ::GetStaticMesh(MeshName.ToWideString());
                    if (StaticMesh)
                    {
                        StaticMeshComp->SetStaticMesh(StaticMesh);
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        ImGui::Text("Add");
        ImGui::SameLine();

        TArray<UClass*> CompClasses;
        GetChildOfClass(UActorComponent::StaticClass(), CompClasses);

        if (ImGui::BeginCombo("##AddComponent", "Components", ImGuiComboFlags_None))
        {
            for (UClass* Class : CompClasses)
            {
                if (ImGui::Selectable(GetData(Class->GetName()), false))
                {
                    USceneComponent* NewComp = Cast<USceneComponent>(StaticMeshComp->GetOwner()->AddComponent(Class));
                    if (NewComp)
                    {
                        NewComp->SetupAttachment(StaticMeshComp);
                    }
                    // 추후 Engine으로부터 SelectedComponent 받아서 선택된 Comp 아래로 붙일 수있으면 붙이기.
                }
            }
            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}


void PropertyEditorPanel::RenderForMaterial(UStaticMeshComponent* StaticMeshComp)
{
    if (StaticMeshComp->GetStaticMesh() == nullptr)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        for (uint32 i = 0; i < StaticMeshComp->GetNumMaterials(); ++i)
        {
            if (ImGui::Selectable(GetData(StaticMeshComp->GetMaterialSlotNames()[i].ToString()), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    std::cout << GetData(StaticMeshComp->GetMaterialSlotNames()[i].ToString()) << std::endl;
                    SelectedMaterialIndex = i;
                    SelectedStaticMeshComp = StaticMeshComp;
                }
            }
        }

        if (ImGui::Button("    +    ")) {
            IsCreateMaterial = true;
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("SubMeshes", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        auto subsets = StaticMeshComp->GetStaticMesh()->GetRenderData()->MaterialSubsets;
        for (uint32 i = 0; i < subsets.Num(); ++i)
        {
            std::string temp = "subset " + std::to_string(i);
            if (ImGui::Selectable(temp.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    StaticMeshComp->SetselectedSubMeshIndex(i);
                    SelectedStaticMeshComp = StaticMeshComp;
                }
            }
        }
        std::string temp = "clear subset";
        if (ImGui::Selectable(temp.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
        {
            if (ImGui::IsMouseDoubleClicked(0))
                StaticMeshComp->SetselectedSubMeshIndex(-1);
        }

        ImGui::TreePop();
    }

    ImGui::PopStyleColor();

    if (SelectedMaterialIndex != -1)
    {
        RenderMaterialView(SelectedStaticMeshComp->GetMaterial(SelectedMaterialIndex));
    }
    if (IsCreateMaterial) {
        RenderCreateMaterialView();
    }
}

void PropertyEditorPanel::RenderMaterialView(UMaterial* Material)
{
    ImGui::SetNextWindowSize(ImVec2(380, 400), ImGuiCond_Once);
    ImGui::Begin("Material Viewer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

    static ImGuiSelectableFlags BaseFlag = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_None | ImGuiColorEditFlags_NoAlpha;

    FVector MatDiffuseColor = Material->GetMaterialInfo().Diffuse;
    FVector MatSpecularColor = Material->GetMaterialInfo().Specular;
    FVector MatAmbientColor = Material->GetMaterialInfo().Ambient;
    FVector MatEmissiveColor = Material->GetMaterialInfo().Emissive;

    float dr = MatDiffuseColor.X;
    float dg = MatDiffuseColor.Y;
    float db = MatDiffuseColor.Z;
    float da = 1.0f;
    float DiffuseColorPick[4] = { dr, dg, db, da };

    ImGui::Text("Material Name |");
    ImGui::SameLine();
    ImGui::Text(*Material->GetMaterialInfo().MaterialName);
    ImGui::Separator();

    ImGui::Text("  Diffuse Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Diffuse##Color", (float*)&DiffuseColorPick, BaseFlag))
    {
        FVector NewColor = { DiffuseColorPick[0], DiffuseColorPick[1], DiffuseColorPick[2] };
        Material->SetDiffuse(NewColor);
    }

    float sr = MatSpecularColor.X;
    float sg = MatSpecularColor.Y;
    float sb = MatSpecularColor.Z;
    float sa = 1.0f;
    float SpecularColorPick[4] = { sr, sg, sb, sa };

    ImGui::Text("Specular Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Specular##Color", (float*)&SpecularColorPick, BaseFlag))
    {
        FVector NewColor = { SpecularColorPick[0], SpecularColorPick[1], SpecularColorPick[2] };
        Material->SetSpecular(NewColor);
    }


    float ar = MatAmbientColor.X;
    float ag = MatAmbientColor.Y;
    float ab = MatAmbientColor.Z;
    float aa = 1.0f;
    float AmbientColorPick[4] = { ar, ag, ab, aa };

    ImGui::Text("Ambient Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Ambient##Color", (float*)&AmbientColorPick, BaseFlag))
    {
        FVector NewColor = { AmbientColorPick[0], AmbientColorPick[1], AmbientColorPick[2] };
        Material->SetAmbient(NewColor);
    }


    float er = MatEmissiveColor.X;
    float eg = MatEmissiveColor.Y;
    float eb = MatEmissiveColor.Z;
    float ea = 1.0f;
    float EmissiveColorPick[4] = { er, eg, eb, ea };

    ImGui::Text("Emissive Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Emissive##Color", (float*)&EmissiveColorPick, BaseFlag))
    {
        FVector NewColor = { EmissiveColorPick[0], EmissiveColorPick[1], EmissiveColorPick[2] };
        Material->SetEmissive(NewColor);
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("Choose Material");
    ImGui::Spacing();

    ImGui::Text("Material Slot Name |");
    ImGui::SameLine();
    ImGui::Text(GetData(SelectedStaticMeshComp->GetMaterialSlotNames()[SelectedMaterialIndex].ToString()));

    ImGui::Text("Override Material |");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160);
    // 메테리얼 이름 목록을 const char* 배열로 변환
    std::vector<const char*> materialChars;
    for (const auto& material : FManagerOBJ::GetMaterials()) {
        materialChars.push_back(*material.Value->GetMaterialInfo().MaterialName);
    }

    //// 드롭다운 표시 (currentMaterialIndex가 범위를 벗어나지 않도록 확인)
    //if (currentMaterialIndex >= FManagerOBJ::GetMaterialNum())
    //    currentMaterialIndex = 0;

    if (ImGui::Combo("##MaterialDropdown", &CurMaterialIndex, materialChars.data(), FManagerOBJ::GetMaterialNum())) {
        UMaterial* material = FManagerOBJ::GetMaterial(materialChars[CurMaterialIndex]);
        SelectedStaticMeshComp->SetMaterial(SelectedMaterialIndex, material);
    }

    if (ImGui::Button("Close"))
    {
        SelectedMaterialIndex = -1;
        SelectedStaticMeshComp = nullptr;
    }

    ImGui::End();
}

void PropertyEditorPanel::RenderCreateMaterialView()
{
    ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);
    ImGui::Begin("Create Material Viewer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

    static ImGuiSelectableFlags BaseFlag = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_None | ImGuiColorEditFlags_NoAlpha;

    ImGui::Text("New Name");
    ImGui::SameLine();
    static char materialName[256] = "New Material";
    // 기본 텍스트 입력 필드
    ImGui::SetNextItemWidth(128);
    if (ImGui::InputText("##NewName", materialName, IM_ARRAYSIZE(materialName))) {
        tempMaterialInfo.MaterialName = materialName;
    }

    FVector MatDiffuseColor = tempMaterialInfo.Diffuse;
    FVector MatSpecularColor = tempMaterialInfo.Specular;
    FVector MatAmbientColor = tempMaterialInfo.Ambient;
    FVector MatEmissiveColor = tempMaterialInfo.Emissive;

    float dr = MatDiffuseColor.X;
    float dg = MatDiffuseColor.Y;
    float db = MatDiffuseColor.Z;
    float da = 1.0f;
    float DiffuseColorPick[4] = { dr, dg, db, da };

    ImGui::Text("Set Property");
    ImGui::Indent();

    ImGui::Text("  Diffuse Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Diffuse##Color", (float*)&DiffuseColorPick, BaseFlag))
    {
        FVector NewColor = { DiffuseColorPick[0], DiffuseColorPick[1], DiffuseColorPick[2] };
        tempMaterialInfo.Diffuse = NewColor;
    }

    float sr = MatSpecularColor.X;
    float sg = MatSpecularColor.Y;
    float sb = MatSpecularColor.Z;
    float sa = 1.0f;
    float SpecularColorPick[4] = { sr, sg, sb, sa };

    ImGui::Text("Specular Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Specular##Color", (float*)&SpecularColorPick, BaseFlag))
    {
        FVector NewColor = { SpecularColorPick[0], SpecularColorPick[1], SpecularColorPick[2] };
        tempMaterialInfo.Specular = NewColor;
    }


    float ar = MatAmbientColor.X;
    float ag = MatAmbientColor.Y;
    float ab = MatAmbientColor.Z;
    float aa = 1.0f;
    float AmbientColorPick[4] = { ar, ag, ab, aa };

    ImGui::Text("Ambient Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Ambient##Color", (float*)&AmbientColorPick, BaseFlag))
    {
        FVector NewColor = { AmbientColorPick[0], AmbientColorPick[1], AmbientColorPick[2] };
        tempMaterialInfo.Ambient = NewColor;
    }


    float er = MatEmissiveColor.X;
    float eg = MatEmissiveColor.Y;
    float eb = MatEmissiveColor.Z;
    float ea = 1.0f;
    float EmissiveColorPick[4] = { er, eg, eb, ea };

    ImGui::Text("Emissive Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Emissive##Color", (float*)&EmissiveColorPick, BaseFlag))
    {
        FVector NewColor = { EmissiveColorPick[0], EmissiveColorPick[1], EmissiveColorPick[2] };
        tempMaterialInfo.Emissive = NewColor;
    }
    ImGui::Unindent();

    ImGui::NewLine();
    if (ImGui::Button("Create Material")) {
        FManagerOBJ::CreateMaterial(tempMaterialInfo);
    }

    ImGui::NewLine();
    if (ImGui::Button("Close"))
    {
        IsCreateMaterial = false;
    }

    ImGui::End();
}

void PropertyEditorPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    Width = clientRect.right - clientRect.left;
    Height = clientRect.bottom - clientRect.top;
}
