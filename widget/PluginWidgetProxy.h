/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_widget_RemotePlugin_h__
#define mozilla_widget_RemotePlugin_h__

#include "PuppetWidget.h"
#include "mozilla/dom/TabChild.h"

/*
 * PluginWidgetProxy is a nsIWidget wrapper we hand around in plugin and layout
 * code. It wraps a native widget it creates in the chrome process. Since this
 * is for plugins, only a limited set of the widget apis need to be overridden,
 * the rest of the implementation is in PuppetWidget or nsBaseWidget.
 */

namespace mozilla {
namespace plugins {
class PluginWidgetChild;
}
namespace widget {

class PluginWidgetProxy MOZ_FINAL : public PuppetWidget
{
public:
  explicit PluginWidgetProxy(dom::TabChild* aTabChild,
                             mozilla::plugins::PluginWidgetChild* aChannel);

protected:
  virtual ~PluginWidgetProxy();

public:
  NS_DECL_ISUPPORTS_INHERITED

  // nsIWidget
  NS_IMETHOD Create(nsIWidget* aParent, nsNativeWidget aNativeParent,
                    const nsIntRect& aRect, nsDeviceContext* aContext,
                    nsWidgetInitData* aInitData = nullptr) MOZ_OVERRIDE;
  NS_IMETHOD Destroy() MOZ_OVERRIDE;
  NS_IMETHOD Show(bool aState) MOZ_OVERRIDE;
  NS_IMETHOD Invalidate(const nsIntRect& aRect) MOZ_OVERRIDE;
  NS_IMETHOD Resize(double aWidth, double aHeight, bool aRepaint) MOZ_OVERRIDE;
  NS_IMETHOD Resize(double aX, double aY, double aWidth,
                    double aHeight, bool aRepaint)  MOZ_OVERRIDE;
  NS_IMETHOD Move(double aX, double aY) MOZ_OVERRIDE;
  NS_IMETHOD SetFocus(bool aRaise = false) MOZ_OVERRIDE;
  NS_IMETHOD SetParent(nsIWidget* aNewParent) MOZ_OVERRIDE;

  virtual nsIWidget* GetParent(void) MOZ_OVERRIDE;
  virtual void* GetNativeData(uint32_t aDataType) MOZ_OVERRIDE;
  virtual nsresult SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                                       bool aIntersectWithExisting) MOZ_OVERRIDE;

  // nsBaseWidget
  virtual nsTransparencyMode GetTransparencyMode() MOZ_OVERRIDE
  { return eTransparencyOpaque; }

public:
  /**
   * When tabs are closed PPluginWidget can terminate before plugin code is
   * finished tearing us down. When this happens plugin calls over mActor
   * fail triggering an abort in the content process. To protect against this
   * the connection tells us when it is torn down here so we can avoid making
   * calls while content finishes tearing us down.
   */
  void ChannelDestroyed() { mActor = nullptr; }

private:
  // Our connection with the chrome widget, created on PBrowser.
  mozilla::plugins::PluginWidgetChild* mActor;
  // PuppetWidget does not implement parent apis, but we need
  // them for plugin widgets.
  nsCOMPtr<nsIWidget> mParent;
};

}  // namespace widget
}  // namespace mozilla
#endif
