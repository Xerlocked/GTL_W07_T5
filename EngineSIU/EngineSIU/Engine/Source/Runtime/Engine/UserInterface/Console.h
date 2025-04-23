#pragma once
#include "Container/Array.h"
#include "D3D11RHI/GraphicDevice.h"
#include "HAL/PlatformType.h"
#include "ImGUI/imgui.h"
#include "PropertyEditor/IWindowToggleable.h"

#define UE_LOG Console::GetInstance().AddLog


enum class LogLevel : uint8
{
    Display,
    Warning,
    Error
};


class StatOverlay
{
public:
    bool showFPS = false;
    bool showMemory = false;
    bool showRender = false;
    bool showLight = false;

    void ToggleStat(const std::string& command);
    void Render(ID3D11DeviceContext* context, UINT width, UINT height);

private:
    float CalculateFPS() const;

    void DrawTextOverlay(const std::string& text, int x, int y) const;

    int32 AllActorCount = 0;

    int32 DirectionalLightCount = 0;
    int32 PointLightCount = 0;
    int32 SpotLightCount = 0;
};

class Console : public IWindowToggleable
{
private:
    Console() = default;

public:
    static Console& GetInstance(); // 참조 반환으로 변경

    void Clear();
    void AddLog(LogLevel level, const char* fmt, ...);
    void Draw();
    void ExecuteCommand(const std::string& command);
    void OnResize(HWND hWnd);

    virtual void Toggle() override
    {
        if (bWasOpen)
        {
            bWasOpen = false;
        }
        else
        {
            bWasOpen = true;
        }
    } // Toggle() 구현 

public:
    struct LogEntry
    {
        LogLevel level;
        FString message;
    };

    TArray<LogEntry> items;
    TArray<FString> history;
    int32 historyPos = -1;
    char inputBuf[256] = "";
    bool scrollToBottom = false;

    ImGuiTextFilter filter; // 필터링을 위한 ImGuiTextFilter

    // 추가된 멤버 변수들
    bool showLogTemp = true; // LogTemp 체크박스
    bool showWarning = true; // Warning 체크박스
    bool showError = true;   // Error 체크박스

    bool bWasOpen = true;
    bool showFPS = false;
    bool showMemory = false;
    // 복사 방지
    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;
    Console(Console&&) = delete;
    Console& operator=(Console&&) = delete;

    StatOverlay overlay;

private:
    bool bExpand = true;
    UINT width;
    UINT height;
};
