#include "RenderProgram3DG1.h"


RenderProgram3DG1::RenderProgram3DG1() {

    ComputeProgram = ComputeProgramWrapper::create();
    ComputeProgram->createProgram("Samples/GeometricFields/Shaders/Compute/g13d.cs.slang");

    bn = 2;
    field = 1;
}


void RenderProgram3DG1::renderGui(Gui::Window* w) {
    if (w->button("start")) {
        retexture = true;
        resolution = sliderRes;
        boundingBox = sliderboundingBox;
    }
    (w->radioButtons(bg_texsize, texturesize));

    w->slider("resolution", sliderRes, 1, 100);

    (w->slider("boundingBox", sliderboundingBox, 2.f, 20.f));
    w->separator();
    w->radioButtons(bg_sdf3d, sdf3d);
    w->separator();
    w->radioButtons(bg_interp, interp);

    if (w->button(debugging ? "debug on" : "debug off")) {
        debugging = !debugging;
    }
    if (debugging) {
        w->slider("position", debugpos_3d, -1.0f, 1.0f);
    }
   
   
}


std::vector<Texture::SharedPtr> RenderProgram3DG1::generateTexture(RenderContext* pRenderContext) {
    Texture::SharedPtr pTexfp = nullptr;
    Texture::SharedPtr pTexn = nullptr;

    if (texturesize == 0) {
        pTexfp = Texture::create3D(resolution, resolution, resolution, ResourceFormat::RGBA16Float, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
        pTexn = Texture::create3D(resolution, resolution, resolution, ResourceFormat::RGBA16Float, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);

    }
    if (texturesize == 1) {
        pTexfp = Texture::create3D(resolution, resolution, resolution, ResourceFormat::RGBA32Float, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
        pTexn = Texture::create3D(resolution, resolution, resolution, ResourceFormat::RGBA32Float, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
    }

    auto& comp = *ComputeProgram;

    comp["texfp"].setUav(pTexfp->getUAV(0));
    comp["texn"].setUav(pTexn->getUAV(0));
    comp["csCb"]["res"] = resolution;
    comp["csCb"]["boundingBox"] = boundingBox;
    comp.getProgram()->addDefine("SDF", std::to_string(sdf3d));
    comp.allocateStructuredBuffer("data1", resolution * resolution * resolution);
    comp.allocateStructuredBuffer("data2", resolution * resolution * resolution);
   comp.allocateStructuredBuffer("posdata", resolution * resolution * resolution);


    comp.runProgram(pRenderContext, resolution, resolution, resolution);


    auto dataptr = comp.mapBuffer<const float4>("data1");
    data1.resize(resolution * resolution * resolution);
    data1.assign(dataptr, dataptr + resolution * resolution * resolution);
    comp.unmapBuffer("data1");
    dataptr = comp.mapBuffer<const float4>("data2");
    data2.resize(resolution * resolution * resolution);
    data2.assign(dataptr, dataptr + resolution * resolution * resolution);
    comp.unmapBuffer("data2");
    dataptr = comp.mapBuffer<const float4>("posdata");
    posdata.resize(resolution * resolution * resolution);
    posdata.assign(dataptr, dataptr + resolution * resolution * resolution);
    comp.unmapBuffer("posdata");

    std::vector<Texture::SharedPtr> textures;
    textures.push_back(pTexfp);
    textures.push_back(pTexn);
    textures.push_back(pTexn);

    return textures;
}

std::vector<Texture::SharedPtr> RenderProgram3DG1::readFromFile(RenderContext* pRenderContext) {
    fileerror = false;

    std::ifstream tfile(filename);

    if (tfile.fail()) {
        fileerror = true;
        msg = "No such file";
        return textures;
    }

    int r;
    float b;

    tfile >> r;
    tfile >> b;

    int d, f;
    tfile >> d;
    tfile >> f;

    if (d != dimension || f != field) {
        fileerror = true;
        msg = "Wrong dimension/order";
        tfile.close();
        return textures;
    }

    resolution = r;
    boundingBox = b;
    sliderRes = r;
    sliderboundingBox = b;

    data1.clear(); data2.clear();  posdata.clear();

    for (int i = 0; i < r * r * r; i++) {
        float4 data;
        tfile >> data.x >> data.y >> data.z >> data.w;
        data1.push_back(data);
        tfile >> data.x >> data.y >> data.z >> data.w;
        data2.push_back(data);
        tfile >> data.x >> data.y >> data.z;
        posdata.push_back(float4(data.xyz, 0.f));
    }

    tfile.close();

    Texture::SharedPtr pTexfpr = nullptr; //footpoint and smaller cuvature
    Texture::SharedPtr pTexdR = nullptr;  //axis and bigger curvature
     if (texturesize == 0) {
        pTexfpr = Texture::create3D(resolution, resolution, resolution, ResourceFormat::RGBA16Float, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
        pTexdR = Texture::create3D(resolution, resolution, resolution, ResourceFormat::RGBA16Float, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
      }
    if (texturesize == 1) {
        pTexfpr = Texture::create3D(resolution, resolution, resolution, ResourceFormat::RGBA32Float, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
        pTexdR = Texture::create3D(resolution, resolution, resolution, ResourceFormat::RGBA32Float, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
       }

    auto& comp = *readProgram;

    comp.getProgram()->addDefine("FIELD", std::to_string(field));
    comp["texfp"].setUav(pTexfpr->getUAV(0));
    comp["texn"].setUav(pTexdR->getUAV(0));
    comp["csCb"]["res"] = resolution;
    comp.allocateStructuredBuffer("data1", r * r * r, data1.data(), sizeof(float4) * r * r * r);
    comp.allocateStructuredBuffer("data2", r * r * r, data2.data(), sizeof(float4) * r * r * r);
    comp.allocateStructuredBuffer("posdata", r * r * r, posdata.data(), sizeof(float4) * r * r * r);

    comp.runProgram(pRenderContext, resolution, resolution, resolution);

    std::vector<Texture::SharedPtr> textures;
    textures.push_back(pTexfpr);
    textures.push_back(pTexdR);
    textures.push_back(pTexdR);


    return textures;
}





