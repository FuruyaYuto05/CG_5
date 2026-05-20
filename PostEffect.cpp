#include "PostEffect.h"
#include "DirectXCommon.h" // DirectXCommonの関数を使うために必要
#include "WinApp.h"        // kClientWidth, kClientHeight を使うために必要
#include <cassert>
#include "Math.h"

void PostEffect::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    dxCommon_ = dxCommon;
    ID3D12Device* device = dxCommon_->GetDevice();

    // ==========================================
    // 1. RenderTextureとRTVの作成（裏画面の用意）
    // ==========================================

    // クリアカラー設定（スライド通り、テスト用に一旦分かりやすい赤色）
    Math::Vector4 kRenderTargetClearValue = { 1.0f, 0.0f, 0.0f, 1.0f };

    // RenderTextureのリソースを作成
    renderTextureResource_ = dxCommon_->CreateRenderTextureResource(
        WinApp::kClientWidth,  // Swapchainと同じ幅
        WinApp::kClientHeight, // Swapchainと同じ高さ
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        kRenderTargetClearValue);

    // RTV（レンダーターゲットビュー）の作成準備
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // リソースと同じフォーマット
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // RTV用のヒープから、書き込む場所（CPUハンドル）を取得する
    rtvHandleCPU_ = dxCommon_->GetRTVCPUDescriptorHandle(2);

    // デバイスを使ってRTVを実際に作成
    device->CreateRenderTargetView(
        renderTextureResource_.Get(),
        &rtvDesc,
        rtvHandleCPU_);

    // ==========================================
    // 2. SRVの作成（裏画面をテクスチャとして読み込む用意）
    // ==========================================

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // リソースと同じフォーマット
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    // SRV用のヒープから、読み込む場所（ハンドル）を取得する
    uint32_t srvIndex = 500;
    srvHandleCPU_ = dxCommon_->GetSRVCPUDescriptorHandle(srvIndex);
    srvHandleGPU_ = dxCommon_->GetSRVGPUDescriptorHandle(srvIndex);

    // デバイスを使ってSRVを実際に作成
    device->CreateShaderResourceView(
        renderTextureResource_.Get(),
        &srvDesc,
        srvHandleCPU_
    );

    // 🌟 今後、この下に「RootSignature」や「PipelineState」を作成する処理を書いていきます！
}

void PostEffect::PreDraw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // 1. 描画先を RenderTexture に設定 (Sceneを描くためDepthも使う)
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVCPUDescriptorHandle();
    commandList->OMSetRenderTargets(1, &rtvHandleCPU_, false, &dsvHandle);

    // 2. 画面クリア（初期化時に設定した ClearValue と同じ赤色）
    float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandleCPU_, clearColor, 0, nullptr);

    // 3. 深度バッファクリア
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // 4. ビューポートとシザー矩形の設定
    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, (FLOAT)WinApp::kClientWidth, (FLOAT)WinApp::kClientHeight, 0.0f, 1.0f };
    commandList->RSSetViewports(1, &viewport);
    D3D12_RECT scissorRect{ 0, 0, WinApp::kClientWidth, WinApp::kClientHeight };
    commandList->RSSetScissorRects(1, &scissorRect);
}

void PostEffect::PostDraw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // 1. 描画先を本来の SwapChain に戻す
    UINT bbIndex = dxCommon_->GetBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE swapChainHandle = dxCommon_->GetRTVCPUDescriptorHandle(bbIndex);

    // SwapChain側はDepthを使わないので nullptr にする！
    commandList->OMSetRenderTargets(1, &swapChainHandle, false, nullptr);

    // 2. SwapChainの画面クリア（RenderTextureの赤色と区別するため青色）
    float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
    commandList->ClearRenderTargetView(swapChainHandle, clearColor, 0, nullptr);

    // 3. ビューポートとシザー矩形の設定（SwapChain用）
    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, (FLOAT)WinApp::kClientWidth, (FLOAT)WinApp::kClientHeight, 0.0f, 1.0f };
    commandList->RSSetViewports(1, &viewport);
    D3D12_RECT scissorRect{ 0, 0, WinApp::kClientWidth, WinApp::kClientHeight };
    commandList->RSSetScissorRects(1, &scissorRect);
}

void PostEffect::Draw() {
    // 🌟 今後、この中に「板ポリゴンを描画する処理（DrawInstancedなど）」を書いていきます！
}