/*
 *   Copyright © 2015 Robert Metsäranta <therealestrob@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; see the file COPYING.  if not, write to
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA 02110-1301, USA.
 */

#include "lightlyshaders.h"
#include "lightlyshaders_config.h"

#include <core/renderviewport.h>
#include <effect/effecthandler.h>
#include <opengl/glshader.h>
#include <opengl/glshadermanager.h>
#include <window.h>

#include <QVector2D>

Q_STATIC_LOGGING_CATEGORY(LIGHTLYSHADERS, "kwin_effect_lightlyshaders", QtWarningMsg)

namespace KWin
{

KWIN_EFFECT_FACTORY_SUPPORTED(LightlyShadersEffect, "lightlyshaders.json", return LightlyShadersEffect::supported();)

// The shader blends the outlines with straight alpha, but KWin premultiplies QColor uniforms
static QVector4D straightColor(const QColor &color)
{
    return QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

LightlyShadersEffect::LightlyShadersEffect()
{
    m_shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                 QString(),
                                                                 QStringLiteral(":/effects/lightlyshaders/shaders/lightlyshaders.frag"));
    if (!m_shader) {
        qCWarning(LIGHTLYSHADERS) << "Failed to load shader, LightlyShaders will not work";
        return;
    }

    m_uniforms = {
        .frameSize = m_shader->uniformLocation("frame_size"),
        .expandedSize = m_shader->uniformLocation("expanded_size"),
        .shadowOrigin = m_shader->uniformLocation("shadow_origin"),
        .radius = m_shader->uniformLocation("radius"),
        .shadowOffset = m_shader->uniformLocation("shadow_sample_offset"),
        .innerOutlineColor = m_shader->uniformLocation("inner_outline_color"),
        .outerOutlineColor = m_shader->uniformLocation("outer_outline_color"),
        .innerOutlineWidth = m_shader->uniformLocation("inner_outline_width"),
        .outerOutlineWidth = m_shader->uniformLocation("outer_outline_width"),
        .squircleRatio = m_shader->uniformLocation("squircle_ratio"),
        .isSquircle = m_shader->uniformLocation("is_squircle"),
    };

    // Decides which windows are managed and redirects them
    reconfigure(ReconfigureAll);
    const auto stackingOrder = effects->stackingOrder();
    for (EffectWindow *window : stackingOrder) {
        watchWindow(window);
    }

    connect(effects, &EffectsHandler::windowAdded, this, &LightlyShadersEffect::windowAdded);
    connect(effects, &EffectsHandler::windowDeleted, this, &LightlyShadersEffect::windowDeleted);
    connect(effects, &EffectsHandler::windowActivated, this, &LightlyShadersEffect::windowActivated);
    m_activeWindow = effects->activeWindow();
}

LightlyShadersEffect::~LightlyShadersEffect() = default;

void LightlyShadersEffect::windowAdded(EffectWindow *w)
{
    m_helper.reevaluate(w);
    watchWindow(w);
    updateRedirect(w);
}

void LightlyShadersEffect::watchWindow(EffectWindow *w)
{
    // Connected for every window, a reconfigure can change which ones are managed
    connect(w, &EffectWindow::windowFullScreenChanged, this, &LightlyShadersEffect::updateRedirect);
    connect(w, &EffectWindow::windowMaximizedStateChanged, this, &LightlyShadersEffect::updateRedirect);
    // These come from the Window, which can outlive its EffectWindow
    connect(w->window(), &Window::tileChanged, this, [this, window = QPointer<EffectWindow>(w)]() {
        if (window) {
            updateRedirect(window);
        }
    });

    m_helper.watch(w, this, [this](EffectWindow *window) {
        updateRedirect(window);
    });
}

void LightlyShadersEffect::windowDeleted(EffectWindow *w)
{
    // OffscreenEffect unredirects deleted windows by itself
    m_redirected.remove(w);
    m_helper.windowDeleted(w);
}

// Only keep a window offscreen while its corners are actually drawn, so fullscreen
// windows (and maximized or tiled ones, if disabled for them) render directly again
void LightlyShadersEffect::updateRedirect(EffectWindow *w)
{
    const bool shouldRedirect = m_shader && m_helper.shouldRound(w);
    if (shouldRedirect == m_redirected.contains(w)) {
        return;
    }

    if (shouldRedirect) {
        redirect(w);
        setShader(w, m_shader.get());
        m_redirected.insert(w);
    } else {
        unredirect(w);
        m_redirected.remove(w);
    }
}

void LightlyShadersEffect::reconfigure(ReconfigureFlags)
{
    m_helper.reconfigure();

    // A disabled outline has no width, the shader skips it
    m_innerOutlineWidth = LightlyShadersConfig::innerOutline() ? LightlyShadersConfig::innerOutlineWidth() : 0;
    m_outerOutlineWidth = LightlyShadersConfig::outerOutline() ? LightlyShadersConfig::outerOutlineWidth() : 0;
    m_innerOutlineColor = LightlyShadersConfig::innerOutlineColor();
    m_outerOutlineColor = LightlyShadersConfig::outerOutlineColor();
    m_inactiveOutlines = LightlyShadersConfig::inactiveOutlines();
    m_inactiveInnerOutlineColor = LightlyShadersConfig::inactiveInnerOutlineColor();
    m_inactiveOuterOutlineColor = LightlyShadersConfig::inactiveOuterOutlineColor();

    // Which windows are managed and the maximized and tiled options may have changed
    const auto stackingOrder = effects->stackingOrder();
    for (EffectWindow *window : stackingOrder) {
        updateRedirect(window);
    }

    effects->addRepaintFull();
}

void LightlyShadersEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data)
{
    // The rounded corners are see-through, so the window must not occlude what is below it.
    // KWin no longer lets effects carve parts out of the opaque region, it's all or nothing.
    if (drawsCorners(w)) {
        data.setTranslucent();
    }

    effects->prePaintWindow(view, w, data);
}

bool LightlyShadersEffect::drawWindow(const RenderTarget &renderTarget,
                                      const RenderViewport &viewport,
                                      EffectWindow *w,
                                      int mask,
                                      const Region &deviceRegion,
                                      WindowPaintData &data)
{
    const bool visible = viewport.renderRect().intersects(w->expandedGeometry()) || (mask & PAINT_WINDOW_TRANSFORMED);
    if (!drawsCorners(w) || !visible) {
        return effects->drawWindow(renderTarget, viewport, w, mask, deviceRegion, data);
    }

    // Use the scale of the output being painted, a window can span outputs with different scales
    const qreal scale = viewport.scale();
    const RectF geo = w->frameGeometry().scaled(scale);
    const RectF expGeo = w->expandedGeometry().scaled(scale);

    ShaderManager *sm = ShaderManager::instance();
    sm->pushShader(m_shader.get());

    m_shader->setUniform(m_uniforms.frameSize, QVector2D(geo.width(), geo.height()));
    m_shader->setUniform(m_uniforms.expandedSize, QVector2D(expGeo.width(), expGeo.height()));
    // Where the frame sits in the texture, which is y-down: the left and top shadow size
    m_shader->setUniform(m_uniforms.shadowOrigin, QVector2D(geo.left() - expGeo.left(), geo.top() - expGeo.top()));
    m_shader->setUniform(m_uniforms.radius, float(m_helper.roundness() * scale));
    m_shader->setUniform(m_uniforms.shadowOffset, float(m_helper.shadowOffset() * scale));
    const bool inactive = m_inactiveOutlines && w != effects->activeWindow();
    m_shader->setUniform(m_uniforms.innerOutlineColor, straightColor(inactive ? m_inactiveInnerOutlineColor : m_innerOutlineColor));
    m_shader->setUniform(m_uniforms.outerOutlineColor, straightColor(inactive ? m_inactiveOuterOutlineColor : m_outerOutlineColor));
    m_shader->setUniform(m_uniforms.innerOutlineWidth, float(m_innerOutlineWidth * scale));
    m_shader->setUniform(m_uniforms.outerOutlineWidth, float(m_outerOutlineWidth * scale));
    m_shader->setUniform(m_uniforms.squircleRatio, m_helper.squircleRatio());
    m_shader->setUniform(m_uniforms.isSquircle, int(m_helper.isSquircle()));

    const bool drawn = OffscreenEffect::drawWindow(renderTarget, viewport, w, mask, deviceRegion, data);

    sm->popShader();
    return drawn;
}

void LightlyShadersEffect::windowActivated(EffectWindow *w)
{
    // With different inactive outlines both the window losing and the one gaining focus change
    if (m_inactiveOutlines) {
        if (m_activeWindow) {
            m_activeWindow->addRepaintFull();
        }
        if (w) {
            w->addRepaintFull();
        }
    }
    m_activeWindow = w;
}

bool LightlyShadersEffect::drawsCorners(EffectWindow *w) const
{
    return m_redirected.contains(w) && m_helper.cornersFit(w);
}

bool LightlyShadersEffect::supported()
{
    return effects->isOpenGLCompositing();
}

} // namespace KWin

#include "lightlyshaders.moc"
