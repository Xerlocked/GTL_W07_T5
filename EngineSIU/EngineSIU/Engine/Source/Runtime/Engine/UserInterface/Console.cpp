#include "Console.h"
#include <cstdarg>
#include <cstdio>

#include "Components/Light/DirectionalLightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorViewportClient.h"
#include "World/World.h"


void StatOverlay::ToggleStat(const std::string& command)
{
    if (command == "stat fps")
    {
        showFPS = true;
        showRender = true;
    }
    else if (command == "stat memory")
    {
        showMemory = true;
        showRender = true;
    }
    else if (command == "stat none")
    {
        showFPS = false;
        showMemory = false;
        showRender = false;
        AllActorCount = 0;
    }
    else if (command == "stat light")
    {
        showLight = true;
        showRender = true;
    }
}

void StatOverlay::Render(ID3D11DeviceContext* context, UINT width, UINT height)
{

    if (!showRender)
        return;
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    // 창 크기를 화면의 50%로 설정합니다.
    ImVec2 windowSize(displaySize.x * 0.5f, displaySize.y * 0.5f);
    // 창을 중앙에 배치하기 위해 위치를 계산합니다.
    ImVec2 windowPos((displaySize.x - windowSize.x) * 0.5f, (displaySize.y - windowSize.y) * 0.5f);

    
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImGui::Begin("Stat Overlay", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoScrollbar);
    if (showFPS) {
        static float lastTime = ImGui::GetTime();
        static int frameCount = 0;
        static float fps = 0.0f;

        frameCount++;
        float currentTime = ImGui::GetTime();
        float deltaTime = currentTime - lastTime;

        if (deltaTime >= 1.0f) { // 1초마다 FPS 업데이트
            fps = frameCount / deltaTime;
            frameCount = 0;
            lastTime = currentTime;
        }
        ImGui::Text("FPS: %.2f", fps);
    }


    if (showMemory)
    {
        ImGui::Text("Allocated Object Count: %llu", FPlatformMemory::GetAllocationCount<EAT_Object>());
        ImGui::Text("Allocated Object Memory: %llu B", FPlatformMemory::GetAllocationBytes<EAT_Object>());
        ImGui::Text("Allocated Container Count: %llu", FPlatformMemory::GetAllocationCount<EAT_Container>());
        ImGui::Text("Allocated Container memory: %llu B", FPlatformMemory::GetAllocationBytes<EAT_Container>());
    }

    if (showLight)
    {
        DirectionalLightCount = 0;
        PointLightCount = 0;
        SpotLightCount = 0;

        // Todo: 매 프레임 반복을 하는데 이걸 좀 고쳐줘
        for (const auto& Actor : GEngine->ActiveWorld->GetActiveLevel()->Actors )
        {
            if (Actor->GetComponentByClass<USpotLightComponent>())
            {
                SpotLightCount++;
            }

            if (Actor->GetComponentByClass<UPointLightComponent>())
            {
                PointLightCount++;
            }

            if (Actor->GetComponentByClass<UDirectionalLightComponent>())
            {
                DirectionalLightCount++;
            }
        }
        
        ImGui::Text("Directional Light Count: %d", DirectionalLightCount);
        ImGui::Text("Point Light Count: %d", PointLightCount);
        ImGui::Text("Spot Light Count: %d", SpotLightCount);
    }
    ImGui::PopStyleColor();
    ImGui::End();
}

float StatOverlay::CalculateFPS() const
{
    static int frameCount = 0;
    static float elapsedTime = 0.0f;
    static float lastTime = 0.0f;

    float currentTime = GetTickCount64() / 1000.0f;
    elapsedTime += (currentTime - lastTime);
    lastTime = currentTime;
    frameCount++;

    if (elapsedTime > 1.0f) {
        float fps = frameCount / elapsedTime;
        frameCount = 0;
        elapsedTime = 0.0f;
        return fps;
    }
    return 0.0f;
}

void StatOverlay::DrawTextOverlay(const std::string& text, int x, int y) const
{
    // ImGui 사용 시
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::Text("%s", text.c_str());
}

// 싱글톤 인스턴스 반환
Console& Console::GetInstance() {
    static Console instance;
    return instance;
}

// 로그 초기화
void Console::Clear() {
    items.Empty();
}

// 로그 추가
void Console::AddLog(LogLevel level, const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    items.Add({ level, std::string(buf) });
    scrollToBottom = true;
}


// 콘솔 창 렌더링
void Console::Draw() {
    if (!bWasOpen) return;
    // 창 크기 및 위치 계산
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    
    // 콘솔 창의 크기와 위치 설정
    float expandedHeight = displaySize.y * 0.4f; // 확장된 상태일 때 높이 (예: 화면의 40%)
    float collapsedHeight = 30.0f;               // 축소된 상태일 때 높이
    float currentHeight = bExpand ? expandedHeight : collapsedHeight;
    
    // 왼쪽 하단에 위치하도록 계산 (창의 좌측 하단이 화면의 좌측 하단에 위치)
    ImVec2 windowSize(displaySize.x * 0.5f, currentHeight); // 폭은 화면의 40%
    ImVec2 windowPos(0, displaySize.y - currentHeight);
    
    // 창 위치와 크기를 고정
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    // 창을 표시하고 닫힘 여부 확인
    overlay.Render(FEngineLoop::GraphicDevice.DeviceContext, width, height);
    bExpand = ImGui::Begin("Console", &bWasOpen);
    if (!bExpand) {
        ImGui::End();
        return;
    }

    // 창을 접었을 경우 UI를 표시하지 않음
    if (!bExpand) {
        ImGui::End();
        return;
    }
    
    // 버튼 UI (로그 수준별 추가)
    if (ImGui::Button("Clear")) { Clear(); }
    ImGui::SameLine();
    if (ImGui::Button("Copy")) { ImGui::LogToClipboard(); }

    ImGui::Separator();

    // 필터 입력 창
    ImGui::Text("Filter:");
    ImGui::SameLine();

    filter.Draw("##Filter", 100);
    
    ImGui::SameLine();

    // 로그 수준을 선택할 체크박스
    ImGui::Checkbox("Show Display", &showLogTemp);
    ImGui::SameLine();
    ImGui::Checkbox("Show Warning", &showWarning);
    ImGui::SameLine();
    ImGui::Checkbox("Show Error", &showError);

    ImGui::Separator();
    // 로그 출력 (필터 적용)
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& entry : items) {
        if (!filter.PassFilter(*entry.message)) continue;

        // 로그 수준에 맞는 필터링
        if ((entry.level == LogLevel::Display && !showLogTemp) ||
            (entry.level == LogLevel::Warning && !showWarning) ||
            (entry.level == LogLevel::Error && !showError)) {
            continue;
        }

        // 색상 지정
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        switch (entry.level) {
        case LogLevel::Display: color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;  // 기본 흰색
        case LogLevel::Warning: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break; // 노란색
        case LogLevel::Error:   color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break; // 빨간색
        }

        ImGui::TextColored(color, "%s", *entry.message);
    }
    if (scrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
    }
    ImGui::EndChild();

    ImGui::Separator();

    // 입력창
    bool reclaimFocus = false;
    if (ImGui::InputText("Input", inputBuf, IM_ARRAYSIZE(inputBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (inputBuf[0]) {
            AddLog(LogLevel::Display, ">> %s", inputBuf);
            std::string command(inputBuf);
            ExecuteCommand(command);
            history.Add(std::string(inputBuf));
            historyPos = -1;
            scrollToBottom = true; // 자동 스크롤
        }
        inputBuf[0] = '\0';
        reclaimFocus = true;
    }

    // 입력 필드에 자동 포커스
    if (reclaimFocus) {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

void Console::ExecuteCommand(const std::string& command)
{
    AddLog(LogLevel::Display, "Executing command: %s", command.c_str());

    if (command == "clear")
    {
        Clear();
    }
    else if (command == "help")
    {
        AddLog(LogLevel::Display, "Available commands:");
        AddLog(LogLevel::Display, " - clear: Clears the console");
        AddLog(LogLevel::Display, " - help: Shows available commands");
        AddLog(LogLevel::Display, " - stat fps: Toggle FPS display");
        AddLog(LogLevel::Display, " - stat memory: Toggle Memory display");
        AddLog(LogLevel::Display, " - stat none: Hide all stat overlays");
    }
    else if (command.starts_with("stat ")) { // stat 명령어 처리
        overlay.ToggleStat(command);
    }
    else if (command.starts_with("shadow_filter "))
    {
        if (command == "shadow_filter VSM")
        {
            FEngineLoop::ResourceManager.ShaderFilterMode = 0;
        }

        if (command == "shadow_filter PCF")
        {
            FEngineLoop::ResourceManager.ShaderFilterMode = 1;
        }
    }
    else {
        AddLog(LogLevel::Error, "Unknown command: %s", command.c_str());
    }
}

void Console::OnResize(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    width = clientRect.right - clientRect.left;
    height = clientRect.bottom - clientRect.top;
}

