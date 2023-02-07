// galaxy.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include <fmt/printf.h>

#include <celmath/ellipsoid.h>
#include <celmath/intersect.h>
#include <celmath/randutils.h>
#include <celmath/ray.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "galaxy.h"
#include "glsupport.h"
#include "image.h"
#include "pixelformat.h"
#include "render.h"
#include "texture.h"
#include "vecgl.h"

namespace vecgl = celestia::vecgl;

namespace
{
constexpr int GALAXY_TEXTURE_SIZE        = 128;
constexpr unsigned int IRR_GALAXY_POINTS = 3500;

// TODO: This value is just a guess.
// To be optimal, it should actually be computed:
constexpr float RADIUS_CORRECTION     = 0.025f;
constexpr float MAX_SPIRAL_THICKNESS  = 0.06f;

constexpr float spriteScaleFactor = 1.0f / 1.55f;

struct Blob
{
    Eigen::Vector3f position;
    std::uint8_t    colorIndex; // color index [0; 255]
    std::uint8_t    brightness; // blob brightness [0.0; 1.0] packed as normalized byte
    std::uint8_t    power;      // we scale blob by spriteScaleFactor**power
};
static_assert(std::is_standard_layout_v<Blob>);

bool operator<(const Blob& b1, const Blob& b2)
{
    return (b1.position.squaredNorm() < b2.position.squaredNorm());
}

using BlobVector = std::vector<Blob>;

class GalacticForm
{
public:
    BlobVector blobs;
    Eigen::Vector3f scale;
};

constexpr unsigned int RequiredIndexCount(unsigned int vCount)
{
    return ((vCount + 3) / 4) * 6;
}

struct GalaxyTypeName
{
    const char* name;
    GalaxyType type;
};

constexpr const GalaxyTypeName GalaxyTypeNames[] =
{
    { "Irr", GalaxyType::Irr },
    { "S0",  GalaxyType::S0 },
    { "Sa",  GalaxyType::Sa },
    { "Sb",  GalaxyType::Sb },
    { "Sc",  GalaxyType::Sc },
    { "SBa", GalaxyType::SBa },
    { "SBb", GalaxyType::SBb },
    { "SBc", GalaxyType::SBc },
    { "E0",  GalaxyType::E0 },
    { "E1",  GalaxyType::E1 },
    { "E2",  GalaxyType::E2 },
    { "E3",  GalaxyType::E3 },
    { "E4",  GalaxyType::E4 },
    { "E5",  GalaxyType::E5 },
    { "E6",  GalaxyType::E6 },
    { "E7",  GalaxyType::E7 },
};

void galaxyTextureEval(float u, float v, float /*w*/, std::uint8_t *pixel)
{
    float r = std::max(0.0f, 0.9f - std::hypot(u, v));
    *pixel = static_cast<std::uint8_t>(r * 255.99f);
}

void colorTextureEval(float u, float /*v*/, float /*w*/, std::uint8_t *pixel)
{
    int i = static_cast<int>((u * 0.5f + 0.5f) * 255.99f); // [-1, 1] -> [0, 255]

    // generic Hue profile as deduced from true-color imaging for spirals
    // Hue in degrees
    float hue = (i < 28)
        ? 25.0f * std::tanh(0.0615f * static_cast<float>(27 - i))
        : 245.0f;
    //convert Hue to RGB
    float r, g, b;
    DeepSkyObject::hsv2rgb(&r, &g, &b, hue, 0.20f, 1.0f);
    pixel[0] = static_cast<std::uint8_t>(r * 255.99f);
    pixel[1] = static_cast<std::uint8_t>(g * 255.99f);
    pixel[2] = static_cast<std::uint8_t>(b * 255.99f);
}

void updatePow2(BlobVector &points)
{
    for (unsigned int i = 0, pow2 = 1, power = 0; i < static_cast<unsigned int>(points.size()); ++i)
    {
        if ((i & pow2) != 0)
        {
            pow2 <<= 1;
            ++power;
        }
        points[i].power = static_cast<std::uint8_t>(power);
    }
}

struct GalaxyVertex
{
    Eigen::Vector3f position;
    Eigen::Matrix<std::uint8_t, 4, 1> texCoord; // texCoord.x = x, texCoord.y = y, texCoord.z = color index, texCoord.w = alpha
};
static_assert(std::is_standard_layout_v<GalaxyVertex>);

constexpr unsigned int MAX_VERTICES = 1024*1024 / sizeof(GalaxyVertex); // 1MB buffer
constexpr unsigned int MAX_INDICES = RequiredIndexCount(MAX_VERTICES);

void bindVMemBuffers(unsigned int vCount)
{
    constexpr int nBuffers = 4;
    static std::array<GLuint, nBuffers> vbos = {};
    static std::array<GLuint, nBuffers> vios = {};
    static std::array<unsigned int, nBuffers> counts = {MAX_VERTICES, MAX_VERTICES / 16 , MAX_VERTICES / 64, MAX_VERTICES / 256}; // 1MB, 64kB, 16kB, 4kB

    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        glGenBuffers(nBuffers, vbos.data());
        glGenBuffers(nBuffers, vios.data());
    }

    // find appropriate buffer size
    int buffer = 0;
    for(int i = nBuffers - 1; i > 0; i--)
    {
        if (vCount <= counts[i])
        {
            buffer = i;
            break;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbos[buffer]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vios[buffer]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GalaxyVertex) * counts[buffer], nullptr, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * RequiredIndexCount(counts[buffer]), nullptr, GL_STREAM_DRAW);
}

void draw(std::size_t vCount, const GalaxyVertex *v, std::size_t iCount, const GLushort *indices)
{
    bindVMemBuffers(vCount);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GalaxyVertex) * vCount, v);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLushort) * iCount, indices);

    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          3, GL_FLOAT, GL_FALSE,
                          sizeof(GalaxyVertex), reinterpret_cast<const void*>(offsetof(GalaxyVertex, position)));
    glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex,
                          4, GL_UNSIGNED_BYTE, GL_TRUE,
                          sizeof(GalaxyVertex), reinterpret_cast<const void*>(offsetof(GalaxyVertex, texCoord)));
    glDrawElements(GL_TRIANGLES, iCount, GL_UNSIGNED_SHORT, nullptr);
}

void renderOldWay(const Eigen::Matrix3f &viewMat, const Eigen::Matrix4f &m, float size, const BlobVector& points, unsigned nPoints, float minimumFeatureSize, float brightness, Renderer &renderer, const Eigen::Matrix4f &pr, const Eigen::Matrix4f &mv)
{
    auto *prog = renderer.getShaderManager().getShader("galaxy");
    if (prog == nullptr)
        return;

    static GalaxyVertex *g_vertices = nullptr;
    static GLushort *g_indices = nullptr;

    if (g_vertices == nullptr)
        g_vertices = new GalaxyVertex[MAX_VERTICES];
    if (g_indices == nullptr)
        g_indices = new GLushort[MAX_INDICES];

    Eigen::Vector3f v0 = viewMat * Eigen::Vector3f(-1, -1, 0) * size;
    Eigen::Vector3f v1 = viewMat * Eigen::Vector3f( 1, -1, 0) * size;
    Eigen::Vector3f v2 = viewMat * Eigen::Vector3f( 1,  1, 0) * size;
    Eigen::Vector3f v3 = viewMat * Eigen::Vector3f(-1,  1, 0) * size;

    prog->use();
    prog->setMVPMatrices(pr, mv);
    prog->samplerParam("galaxyTex") = 0;
    prog->samplerParam("colorTex") = 1;

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    renderer.setPipelineState(ps);

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);

    std::size_t vertex = 0, index = 0;
    GLushort    j = 0;
    for (unsigned int i = 0, pow2 = 1; i < nPoints; ++i)
    {
        if ((i & pow2) != 0)
        {
            pow2 <<= 1;
            size *= spriteScaleFactor;
            v0 *= spriteScaleFactor;
            v1 *= spriteScaleFactor;
            v2 *= spriteScaleFactor;
            v3 *= spriteScaleFactor;
            if (size < minimumFeatureSize)
                break;
        }

        const Blob     &b = points[i];
        Eigen::Vector3f p = (m * Eigen::Vector4f(b.position.x(), b.position.y(), b.position.z(), 1.0f)).head(3);

        float screenFrac = size / p.norm();
        if (screenFrac < 0.1f)
        {
            float    a = std::min(255.0f, (0.1f - screenFrac) * b.brightness * brightness);
            auto alpha = static_cast<std::uint8_t>(a); // encode as byte
            auto color = static_cast<std::uint8_t>(b.colorIndex);
            g_vertices[vertex++] = { p + v0, { std::uint8_t(0),   std::uint8_t(0),   color, alpha } };
            g_vertices[vertex++] = { p + v1, { std::uint8_t(255), std::uint8_t(0),   color, alpha } };
            g_vertices[vertex++] = { p + v2, { std::uint8_t(255), std::uint8_t(255), color, alpha } };
            g_vertices[vertex++] = { p + v3, { std::uint8_t(0),   std::uint8_t(255), color, alpha } };

            g_indices[index++] = j;
            g_indices[index++] = j + 1;
            g_indices[index++] = j + 2;
            g_indices[index++] = j;
            g_indices[index++] = j + 2;
            g_indices[index++] = j + 3;
            j += 4;

            if (vertex + 4 > MAX_VERTICES)
            {
                draw(vertex, g_vertices, index, g_indices);
                index  = 0;
                vertex = 0;
                j      = 0;
            }
        }
    }

    if (index > 0)
        draw(vertex, g_vertices, index, g_indices);

    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    glActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void renderNewWay(std::size_t form, const Eigen::Matrix3f &viewMat, const Eigen::Matrix4f &m, float size, const BlobVector& points, unsigned nPoints, float minimumFeatureSize, float brightness, Renderer &renderer, const Eigen::Matrix4f &pr, const Eigen::Matrix4f &mv)
{
    auto *prog = renderer.getShaderManager().getShaderGL3("galaxy150");
    if (prog == nullptr)
        return;

    GLuint sizeLoc = prog->attribIndex("in_Size");
    GLuint colorLoc = prog->attribIndex("in_ColorIndex");
    GLuint brightLoc = prog->attribIndex("in_Brightness");

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    renderer.setPipelineState(ps);

    prog->use();
    prog->setMVPMatrices(pr, mv);
    prog->samplerParam("galaxyTex")        = 0;
    prog->samplerParam("colorTex")         = 1;
    prog->floatParam("size")               = size;
    prog->floatParam("brightness")         = brightness;
    prog->floatParam("minimumFeatureSize") = minimumFeatureSize;
    prog->mat4Param("m")                   = m;
    prog->mat3Param("viewMat")             = viewMat;

    // find proper nPoints count
    if (minimumFeatureSize > 0.0f)
        nPoints = std::min(nPoints, 1u << static_cast<unsigned int>(logf(minimumFeatureSize/size)/logf(spriteScaleFactor)));

    static std::array<GLuint, 32> vboPoints = {};
    if (form >= vboPoints.size())
    {
        fmt::print("FIXME: Too high form# value = {}! Skipping...\n", form);
        return;
    }
    GLuint vboPoint = vboPoints[form];
    if (vboPoint == 0)
    {
        glGenBuffers(1, &vboPoint);
        vboPoints[form] = vboPoint;
        glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Blob) * points.size(), points.data(), GL_STATIC_DRAW);
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
    }

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glEnableVertexAttribArray(sizeLoc);
    glEnableVertexAttribArray(colorLoc);
    glEnableVertexAttribArray(brightLoc);

    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
        3, GL_FLOAT,         GL_FALSE, sizeof(Blob), reinterpret_cast<const void*>(offsetof(Blob, position)));
    glVertexAttribPointer(colorLoc,
        1, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Blob), reinterpret_cast<const void*>(offsetof(Blob, colorIndex)));
    glVertexAttribPointer(brightLoc,
        1, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Blob), reinterpret_cast<const void*>(offsetof(Blob, brightness)));
    glVertexAttribPointer(sizeLoc,
        1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Blob), reinterpret_cast<const void*>(offsetof(Blob, power)));

    glDrawArrays(GL_POINTS, 0, nPoints);

    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glDisableVertexAttribArray(sizeLoc);
    glDisableVertexAttribArray(colorLoc);
    glDisableVertexAttribArray(brightLoc);
    glActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

std::optional<GalacticForm> buildGalacticForm(const fs::path& filename)
{
    Blob b;
    BlobVector galacticPoints;

    // Load templates in standard .png format
    int width, height, rgb, j = 0, kmin = 9;
    unsigned char value;
    float h = 0.75f;
    std::unique_ptr<Image> img = LoadImageFromFile(filename);
    if (img == nullptr)
    {
        celestia::util::GetLogger()->error("The galaxy template *** {} *** could not be loaded!\n", filename);
        return std::nullopt;
    }
    width  = img->getWidth();
    height = img->getHeight();
    rgb    = img->getComponents();

    auto& rng = celmath::getRNG();
    rng.seed(1312);
    for (int i = 0; i < width * height; i++)
    {
        value = img->getPixels()[rgb * i];
        if (value > 10)
        {
            float idxf = static_cast<float>(i);
            float wf = static_cast<float>(width);
            float hf = static_cast<float>(height);
            float z = std::floor(idxf / wf);
            float x = (idxf - wf * z - 0.5f * (wf - 1.0f)) / wf;
            z  = (0.5f * (hf - 1.0f) - z) / hf;
            x  += celmath::RealDists<float>::SignedUnit(rng) * 0.008f;
            z  += celmath::RealDists<float>::SignedUnit(rng) * 0.008f;
            float r2 = x * x + z * z;

            float y;
            if (filename != "models/E0.png")
            {
                float y0 = 0.5f * MAX_SPIRAL_THICKNESS * std::sqrt(static_cast<float>(value)/256.0f) * std::exp(- 5.0f * r2);
                float B = (r2 > 0.35f) ? 1.0f: 0.75f; // the darkness of the "dust lane", 0 < B < 1
                float p0 = 1.0f - B * std::exp(-h * h); // the uniform reference probability, envelopping prob*p0.
                float yr, prob;
                do
                {
                    // generate "thickness" y of spirals with emulation of a dust lane
                    // in galctic plane (y=0)
                    yr = celmath::RealDists<float>::SignedUnit(rng) * h;
                    prob = (1.0f - B * exp(-yr * yr))/p0;
                } while (celmath::RealDists<float>::Unit(rng) > prob);
                b.brightness = static_cast<std::uint8_t>(value * prob);
                y = y0 * yr / h;
            }
            else
            {
                // generate spherically symmetric distribution from E0.png
                float yy, prob;
                do
                {
                    yy = celmath::RealDists<float>::SignedUnit(rng);
                    float ry2 = 1.0f - yy * yy;
                    prob = ry2 > 0 ? std::sqrt(ry2): 0.0f;
                } while (celmath::RealDists<float>::Unit(rng) > prob);
                y = yy * std::sqrt(0.25f - r2) ;
                b.brightness = value;
                kmin = 12;
            }

            b.position = Eigen::Vector3f(x, y, z);
            b.colorIndex = static_cast<std::uint8_t>(std::min(b.position.norm() * 511.0f, 255.0f));
            galacticPoints.push_back(b);
            j++;
        }
    }


    // sort to start with the galaxy center region (x^2 + y^2 + z^2 ~ 0), such that
    // the biggest (brightest) sprites will be localized there!

    std::sort(galacticPoints.begin(), galacticPoints.end());

    // reshuffle the galaxy points randomly...except the first kmin+1 in the center!
    // the higher that number the stronger the central "glow"

    std::shuffle(galacticPoints.begin() + kmin, galacticPoints.end(), celmath::getRNG());

    updatePow2(galacticPoints);

    std::optional<GalacticForm> galacticForm(std::in_place);
    galacticForm->blobs = std::move(galacticPoints);
    galacticForm->scale = Eigen::Vector3f::Ones();

    return galacticForm;
}

class GalacticFormManager
{
 private:
    static constexpr std::size_t GalacticFormsReserve = 32;

 public:
    GalacticFormManager()
    {
        initializeStandardForms();
    }

    const GalacticForm* getForm(std::size_t) const;
    std::size_t getCustomForm(const fs::path& path);

 private:
    void initializeStandardForms();

    std::vector<std::optional<GalacticForm>> galacticForms{ };
    std::map<fs::path, std::size_t> customForms{ };
};

const GalacticForm* GalacticFormManager::getForm(std::size_t form) const
{
    assert(form < galacticForms.size());
    return galacticForms[form].has_value()
        ? &*galacticForms[form]
        : nullptr;
}

std::size_t GalacticFormManager::getCustomForm(const fs::path& path)
{
    auto iter = customForms.find(path);
    if (iter != customForms.end())
    {
        return iter->second;
    }

    std::size_t result = galacticForms.size();
    customForms[path] = result;
    galacticForms.push_back(buildGalacticForm(path));
    return result;
}

void GalacticFormManager::initializeStandardForms()
{
    // Irregular Galaxies
    unsigned int galaxySize = IRR_GALAXY_POINTS, ip = 0;
    Blob b;
    Eigen::Vector3f p;

    BlobVector irregularPoints;
    irregularPoints.reserve(galaxySize);

    auto& rng = celmath::getRNG();
    while (ip < galaxySize)
    {
        p = Eigen::Vector3f(celmath::RealDists<float>::SignedUnit(rng),
                            celmath::RealDists<float>::SignedUnit(rng),
                            celmath::RealDists<float>::SignedUnit(rng));
        float r  = p.norm();
        if (r < 1)
        {
            Eigen::Vector3f p1(p.array() + 5.0f);
            float prob = (1.0f - r) * (celmath::fractalsum(p1, 8.0f) + 1.0f) * 0.5f;
            if (celmath::RealDists<float>::Unit(rng) < prob)
            {
                b.position   = p;
                b.brightness = std::uint8_t(64);
                b.colorIndex = static_cast<std::uint8_t>(std::min(r * 511.0f, 255.0f));
                irregularPoints.push_back(b);
                ++ip;
            }
        }
    }

    updatePow2(irregularPoints);

    std::optional<GalacticForm>& irregularForm = galacticForms.emplace_back(std::in_place);
    irregularForm->blobs = std::move(irregularPoints);
    irregularForm->scale = Eigen::Vector3f::Constant(0.5f);

    // Spiral Galaxies, 7 classical Hubble types

    galacticForms.push_back(buildGalacticForm("models/S0.png"));
    galacticForms.push_back(buildGalacticForm("models/Sa.png"));
    galacticForms.push_back(buildGalacticForm("models/Sb.png"));
    galacticForms.push_back(buildGalacticForm("models/Sc.png"));
    galacticForms.push_back(buildGalacticForm("models/SBa.png"));
    galacticForms.push_back(buildGalacticForm("models/SBb.png"));
    galacticForms.push_back(buildGalacticForm("models/SBc.png"));

    // Elliptical Galaxies , 8 classical Hubble types, E0..E7,
    //
    // To save space: generate spherical E0 template from S0 disk
    // via rescaling by (1.0f, 3.8f, 1.0f).

    for (unsigned int eform = 0; eform <= 7; ++eform)
    {
        float ell = 1.0f - static_cast<float>(eform) / 8.0f;

        // note the correct x,y-alignment of 'ell' scaling!!
        // build all elliptical templates from rescaling E0
        std::optional<GalacticForm> ellipticalForm = buildGalacticForm("models/E0.png");
        if (ellipticalForm.has_value())
        {
            ellipticalForm->scale = Eigen::Vector3f(ell, ell, 1.0f);

            for (Blob& blob : ellipticalForm->blobs)
            {
                blob.colorIndex = static_cast<std::uint8_t>(std::ceil(0.76f * static_cast<float>(blob.colorIndex)));
            }
        }

        galacticForms.push_back(std::move(ellipticalForm));
    }
}

GalacticFormManager* getGalacticFormManager()
{
    static GalacticFormManager* galacticFormManager = new GalacticFormManager();
    return galacticFormManager;
}

} // end unnamed namespace


float Galaxy::lightGain = 0.0f;

float Galaxy::getDetail() const
{
    return detail;
}

void Galaxy::setDetail(float d)
{
    detail = d;
}

const char* Galaxy::getType() const
{
    return GalaxyTypeNames[static_cast<std::size_t>(type)].name;
}

void Galaxy::setType(const std::string& typeStr)
{
    type = GalaxyType::Irr;
    auto iter = std::find_if(std::begin(GalaxyTypeNames), std::end(GalaxyTypeNames),
                             [&](const GalaxyTypeName& g) { return g.name == typeStr; });
    if (iter != std::end(GalaxyTypeNames))
        type = iter->type;
}

void Galaxy::setForm(const std::string& customTmpName)
{
    if (customTmpName.empty())
    {
        form = static_cast<std::size_t>(type);
    }
    else
    {
        form = getGalacticFormManager()->getCustomForm(fs::path("models") / customTmpName);
    }
}

std::string Galaxy::getDescription() const
{
    return fmt::sprintf(_("Galaxy (Hubble type: %s)"), getType());
}

const char* Galaxy::getObjTypeName() const
{
    return "galaxy";
}

bool Galaxy::pick(const Eigen::ParametrizedLine<double, 3>& ray,
                  double& distanceToPicker,
                  double& cosAngleToBoundCenter) const
{
    const GalacticForm* galacticForm = getGalacticFormManager()->getForm(form);
    if (galacticForm == nullptr || !isVisible()) { return false; }

    // The ellipsoid should be slightly larger to compensate for the fact
    // that blobs are considered points when galaxies are built, but have size
    // when they are drawn.
    float yscale = (type > GalaxyType::Irr && type < GalaxyType::E0)
        ? MAX_SPIRAL_THICKNESS
        : galacticForm->scale.y() + RADIUS_CORRECTION;
    Eigen::Vector3d ellipsoidAxes(getRadius()*(galacticForm->scale.x() + RADIUS_CORRECTION),
                                  getRadius()* yscale,
                                  getRadius()*(galacticForm->scale.z() + RADIUS_CORRECTION));
    Eigen::Matrix3d rotation = getOrientation().cast<double>().toRotationMatrix();

    return celmath::testIntersection(
        celmath::transformRay(Eigen::ParametrizedLine<double, 3>(ray.origin() - getPosition(), ray.direction()),
                              rotation),
        celmath::Ellipsoidd(ellipsoidAxes),
        distanceToPicker,
        cosAngleToBoundCenter);
}

bool Galaxy::load(const AssociativeArray* params, const fs::path& resPath)
{
    setDetail(params->getNumber<float>("Detail").value_or(1.0f));

    if (const std::string* typeName = params->getString("Type"); typeName == nullptr)
    {
        setType("");
    }
    else
    {
        setType(*typeName);
    }


    if (const std::string* customTmpName = params->getString("CustomTemplate"); customTmpName == nullptr)
    {
        setForm("");
    }
    else
    {
        setForm(*customTmpName);
    }

    return DeepSkyObject::load(params, resPath);
}

void Galaxy::render(const Eigen::Vector3f& offset,
                    const Eigen::Quaternionf& viewerOrientation,
                    float brightness,
                    float pixelSize,
                    const Matrices& ms,
                    Renderer* renderer)
{
    static Texture* galaxyTex = nullptr;
    static Texture* colorTex  = nullptr;

    const GalacticForm* galacticForm = getGalacticFormManager()->getForm(form);
    if (galacticForm == nullptr)
        return;

    /* We'll first see if the galaxy's apparent size is big enough to
       be noticeable on screen; if it's not we'll break right here,
       avoiding all the overhead of the matrix transformations and
       GL state changes: */
    float distanceToDSO = std::max(0.0f, offset.norm() - getRadius());

    float minimumFeatureSize = pixelSize * distanceToDSO;
    float size = 2.0f * getRadius();

    if (size < minimumFeatureSize)
        return;

    if (galaxyTex == nullptr)
    {
        galaxyTex = CreateProceduralTexture(GALAXY_TEXTURE_SIZE,
                                            GALAXY_TEXTURE_SIZE,
                                            celestia::PixelFormat::LUMINANCE,
                                            galaxyTextureEval).release();
    }
    assert(galaxyTex != nullptr);
    glActiveTexture(GL_TEXTURE0);
    galaxyTex->bind();

    if (colorTex == nullptr)
    {
        colorTex = CreateProceduralTexture(256, 1, celestia::PixelFormat::RGBA,
                                           colorTextureEval,
                                           Texture::EdgeClamp,
                                           Texture::NoMipMaps).release();
    }
    assert(colorTex != nullptr);
    glActiveTexture(GL_TEXTURE1);
    colorTex->bind();

    Eigen::Matrix3f viewMat = viewerOrientation.conjugate().toRotationMatrix();

    Eigen::Quaternionf orientation = getOrientation().conjugate();
    Eigen::Matrix3f mScale = galacticForm->scale.asDiagonal() * size;
    Eigen::Matrix3f mLinear = orientation.toRotationMatrix() * mScale;

    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    m.topLeftCorner(3,3) = mLinear;
    m.block<3,1>(0, 3) = offset;

    // corrections to avoid excessive brightening if viewed e.g. edge-on
    float brightness_corr = 1.0f;
    if (type < GalaxyType::E0 || type > GalaxyType::E3) // all galaxies, except ~round elliptics
    {
        float cosi = (orientation * Eigen::Vector3f::UnitY()).dot(offset) / offset.norm();
        brightness_corr = std::max(0.2f, std::sqrt(std::abs(cosi)));
    }
    if (type > GalaxyType::E3) // only elliptics with higher ellipticities
    {
        float cosi = (orientation * Eigen::Vector3f::UnitX()).dot(offset) / offset.norm();
        brightness_corr = std::max(0.45f, brightness_corr * std::abs(cosi));
    }

    const float btot = (type == GalaxyType::Irr || type >= GalaxyType::E0) ? 2.5f : 5.0f;
    brightness = (4.0f * lightGain + 1.0f) * btot * brightness_corr * brightness;

    Eigen::Matrix4f mv = vecgl::translate(*ms.modelview, Eigen::Vector3f(-offset));

    const BlobVector& points = galacticForm->blobs;
    auto nPoints = static_cast<unsigned>(points.size() * std::clamp(getDetail(), 0.0f, 1.0f));

#ifdef GL_ES
    bool gl32 = celestia::gl::checkVersion(celestia::gl::GLES_3_2);
#else
    bool gl32 = celestia::gl::checkVersion(celestia::gl::GL_3_2);
#endif
    if (gl32)
    {
        renderNewWay(form, viewMat, m, size, points, nPoints, minimumFeatureSize, brightness, *renderer, *ms.projection, mv);
    }
    else
    {
        renderOldWay(viewMat, m, size, points, nPoints, minimumFeatureSize, brightness, *renderer, *ms.projection, mv);
    }
}

std::uint64_t Galaxy::getRenderMask() const
{
    return Renderer::ShowGalaxies;
}

unsigned int Galaxy::getLabelMask() const
{
    return Renderer::GalaxyLabels;
}

void Galaxy::increaseLightGain()
{
    lightGain = std::min(1.0f, lightGain + 0.05f);
}

void Galaxy::decreaseLightGain()
{
    lightGain = std::max(0.0f, lightGain - 0.05f);
}

float Galaxy::getLightGain()
{
    return lightGain;
}

void Galaxy::setLightGain(float lg)
{
    lightGain = std::clamp(lg, 0.0f, 1.0f);
}

std::ostream& operator<<(std::ostream& s, const GalaxyType& sc)
{
    return s << GalaxyTypeNames[static_cast<std::size_t>(sc)].name;
}
