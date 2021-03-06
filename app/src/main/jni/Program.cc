﻿///////////////////////////////////////
//
//	Computer Graphics TSBK03
//	Conrad Wahlén - conwa099
//
///////////////////////////////////////

#include "Program.h"
#include "../../../../../../../../Android/android-sdk/ndk-bundle/platforms/android-23/arch-arm/usr/include/android/asset_manager.h"


Program::Program() {
    // Set all pointers to null
    cam = NULL;

    // Window init size
    winWidth = 0;
    winHeight = 0;

    // Time init
    time.startTimer();

    // Set program parameters
    cameraStartPos = glm::vec3(0.0f, 0.0f, 3.0f);
    cameraFrustumFar = 500.0f;

    sceneSelect = 0;
    useOrtho = false;
    drawVoxelOverlay = false;
}

Program::~Program() {
    Clean();
}

void Program::Step() {
    //LOGD("Step called");
//    glTime.startTimer();
    Update();
//    glTime.endTimer();
//    LOGD("Update time: %f ms", glTime.getTimeMS());
//    glTime.startTimer();
    Render();
//    glTime.endTimer();
//    LOGD("Draw time: %f ms", glTime.getTimeMS());
}

void Program::Resize(int width, int height) {
    LOGD("Resize called, width: %d, height: %d", width, height);

    winWidth = width;
    winHeight = height;
    glViewport(0,0,width,height);
    cam->SetFrustum();
    GetCurrentScene()->SetupSceneTextures();
//    GetCurrentScene()->Voxelize();
}

/*
void Program::timeUpdate() {
    time.endTimer();
    param.deltaT = time.getLapTime();
    param.currentT = time.getTime();
    FPS = 1.0f / time.getLapTime();
}
*/

bool Program::Init() {
    LOGD("Init called");

    // Activate depth test and blend for masking textures
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_CULL_FACE));
    //glEnable(GL_TEXTURE_3D);
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GL_CHECK(glClearColor(0.3f, 0.0f, 0.3f, 1.0f));

    dumpInfo();

    GL_CHECK(glGenBuffers(1, &programBuffer));
    GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, PROGRAM, programBuffer));
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, sizeof(ProgramStruct), &param, GL_STREAM_DRAW));

    // Load shaders for drawing
    shaders.drawScene = loadShaders(SHADER_PATH("drawModel.vert"), SHADER_PATH("drawModel.frag"));
    shaders.drawData = loadShaders(SHADER_PATH("drawData.vert"), SHADER_PATH("drawData.frag"));

    // Load shaders for voxelization
    shaders.voxelize = loadShadersG(SHADER_PATH("voxelization.vert"),
                                    SHADER_PATH("voxelization.frag"),
                                    SHADER_PATH("voxelization.geom"));

    // Single triangle shader for deferred shading etc.
    shaders.singleTriangle = loadShaders(SHADER_PATH("drawTriangle.vert"),
                                         SHADER_PATH("drawTriangle.frag"));

    // Draw voxels from 3D texture
    shaders.voxel = loadShaders(SHADER_PATH("drawVoxel.vert"), SHADER_PATH("drawVoxel.frag"));

    // Calculate mipmaps
    shaders.mipmap = CompileComputeShader(SHADER_PATH("mipmap.comp"));

    // Create shadowmap
    shaders.shadowMap = loadShaders(SHADER_PATH("shadowMap.vert"), SHADER_PATH("shadowMap.frag"));

    InitTimer();

    // Set up the camera
    cam = new Camera(cameraStartPos, &winWidth, &winHeight, cameraFrustumFar);
    if (!cam->Init()) {
        LOGE("Camera not initialized!");
        return false;
    }

    // Load Asset folders
    LoadAssetFolder("models");

    // Load scenes
    Scene* cornell = new Scene();
    if(!cornell->Init(MODEL_PATH("cornell.obj"), &shaders)){
        LOGE("Error loading scene: Cornell!");
        return false;
    }
    scenes.push_back(cornell);

//    Scene *sponza = new Scene();
//    if (!sponza->Init(MODEL_PATH("sponza.obj"), &shaders)) return false;
//    scenes.push_back(sponza);

    // Initial Voxelization of the scenes
    cornell->CreateShadow();
    cornell->RenderData();
    cornell->Voxelize();
    cornell->MipMap();

//    sponza->CreateShadow();
//    sponza->RenderData();
//    sponza->Voxelize();
//    sponza->MipMap();

    return true;
}

void Program::Update() {
    // Upload program params (incl time update)
    UploadParams();

    // Update the camera
    cam->UpdateCamera();
}

void Program::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GetCurrentScene()->CreateShadow();
    GetCurrentScene()->RenderData();
//    GetCurrentScene()->Voxelize();
//    GetCurrentScene()->MipMap();
    GetCurrentScene()->Draw();
}

void Program::Clean() {
    GL_CHECK(glDeleteBuffers(1, &programBuffer));
}

void Program::UploadParams() {
    // Update program parameters
    GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, PROGRAM, programBuffer));
    GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, NULL, sizeof(ProgramStruct), &param));
}

void Program::SetAssetMgr(AAssetManager *mgr) {
    SetAssetsManager(mgr);
}
