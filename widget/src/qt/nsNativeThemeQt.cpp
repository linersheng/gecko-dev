/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * David Hyatt (hyatt@netscape.com).
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Lars Knoll <knoll@kde.org>
 *   Zack Rusin <zack@kde.org>
 *   Tim Hill (tim@prismelite.com)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <QComboBox>
#include <QRect>
#include <QPainter>
#include <QStyleOption>
 #include <QStyleOptionFrameV2>

#include "nsNativeThemeQt.h"
#include "nsRect.h"
#include "nsSize.h"
#include "nsTransform2D.h"
#include "nsThemeConstants.h"
#include "nsILookAndFeel.h"
#include "nsIServiceManager.h"
#include "nsIEventStateManager.h"
#include <malloc.h>

#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxQPainterSurface.h"
#include "nsIRenderingContext.h"

nsNativeThemeQt::nsNativeThemeQt()
{
    combo = new QComboBox((QWidget *)0);
    combo->resize(0, 0);
    ThemeChanged();
}

nsNativeThemeQt::~nsNativeThemeQt()
{
    delete combo;
}

NS_IMPL_ISUPPORTS1(nsNativeThemeQt, nsITheme)

static QRect qRect(const nsRect &aRect, const nsTransform2D *aTrans)
{
    int x = aRect.x;
    int y = aRect.y;
    int w = aRect.width;
    int h = aRect.height;
    aTrans->TransformCoord(&x,&y,&w,&h);
    return QRect(x, y, w, h);
}

NS_IMETHODIMP
nsNativeThemeQt::DrawWidgetBackground(nsIRenderingContext* aContext,
                                      nsIFrame* aFrame,
                                      PRUint8 aWidgetType,
                                      const nsRect& aRect,
                                      const nsRect& aClipRect)
{
//    qDebug("%s", __PRETTY_FUNCTION__);

    gfxContext* context = aContext->ThebesContext();
    nsRefPtr<gfxASurface> surface =  context->CurrentSurface();
    gfxASurface* raw = surface;
    gfxQPainterSurface* qsurface = (gfxQPainterSurface*)raw;
    QPainter* qpainter = qsurface->GetQPainter();

//     qDebug("aWidgetType = %d", aWidgetType);
    if (!qpainter)
        return NS_OK;

    QStyle* style = qApp->style();
    const QPalette::ColorGroup cg = qApp->palette().currentColorGroup();

    nsTransform2D* curTrans;
    aContext->GetCurrentTransform(curTrans);

    QRect r = qRect(aRect, curTrans);
    QRect cr = qRect(aClipRect, curTrans);
//    context->UpdateGC();
//     qDebug("rect=%d %d %d %d", aRect.x, aRect.y, aRect.width, aRect.height);
    qpainter->save();
    qpainter->translate(r.x(), r.y());
    r.translate(-r.x(), -r.y());

//     QStyleOption option;
//     option.direction = QApplication::layoutDirection();
//     option.rect = r;

    QStyle::PrimitiveElement pe = QStyle::PE_CustomBase;

//    QStyle::ControlElement ce = QStyle::CE_CustomBase;

    QStyle::State flags = IsDisabled(aFrame) ?
                            QStyle::State_None :
                            QStyle::State_Enabled;

    PRInt32 eventState = GetContentState(aFrame, aWidgetType);
//     qDebug("eventState = %d",  eventState);

    if (eventState & NS_EVENT_STATE_HOVER)
        flags |= QStyle::State_MouseOver;
    if (eventState & NS_EVENT_STATE_FOCUS)
        flags |= QStyle::State_HasFocus;
    if (eventState & NS_EVENT_STATE_ACTIVE)
        flags |= QStyle::State_DownArrow;

    switch (aWidgetType) {
    case NS_THEME_RADIO:
//         flags |= (IsChecked(aFrame) ? QStyle::State_On : QStyle::State_Off);
//         pe = QStyle::PE_IndicatorRadioButton;
        break;
    case NS_THEME_CHECKBOX:
//         flags |= (IsChecked(aFrame) ? QStyle::State_On : QStyle::State_Off);
//         pe = QStyle::PE_IndicatorCheckBox;
        break;
    case NS_THEME_SCROLLBAR:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
//         qpainter->fillRect(r, qApp->palette().brush(QPalette::Active, QPalette::Background));
        break;
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
//         flags |= QStyle::State_Horizontal;
        // fall through
    case NS_THEME_SCROLLBAR_BUTTON_UP:
//         ce = QStyle::CE_ScrollBarSubLine;
        break;
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
//         flags |= QStyle::State_Horizontal;
        // fall through
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
//         ce = QStyle::CE_ScrollBarAddLine;
        break;
    case NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
//         flags |= QStyle::State_Horizontal;
        // fall through
    case NS_THEME_SCROLLBAR_GRIPPER_VERTICAL:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
//         ce = QStyle::CE_ScrollBarSlider;
        break;
    case NS_THEME_BUTTON_BEVEL:
//         ce = QStyle::CE_PushButtonBevel;
//         flags |= QStyle::State_Raised;
        break;
    case NS_THEME_BUTTON:
// 	    pe = IsDefaultButton(aFrame) ? QStyle::PE_FrameDefaultButton : QStyle::PE_PanelButtonCommand;
//         flags |= QStyle::State_Raised;
        break;
    case NS_THEME_DROPDOWN:
//        style.drawComplexControl(QStyle::CC_ComboBox, qpainter, combo, r, cg, flags, QStyle::SC_ComboBoxFrame);
        break;
    case NS_THEME_DROPDOWN_BUTTON:
//         r.translate(frameWidth, -frameWidth);
//         r.setHeight(r.height() + 2*frameWidth);
//        style.drawComplexControl(QStyle::CC_ComboBox, qpainter, combo, r, cg, flags, QStyle::SC_ComboBoxArrow);
        break;
    case NS_THEME_DROPDOWN_TEXT:
    case NS_THEME_DROPDOWN_TEXTFIELD:
        break;
    case NS_THEME_TEXTFIELD:
    case NS_THEME_LISTBOX: {
        qDebug("NS_THEME_TEXTFIELD || NS_THEME_LISTBOX");
        
        pe = QStyle::PE_PanelLineEdit;

        QStyleOptionFrameV2 option;
        option.direction = QApplication::layoutDirection();
        option.rect = r;
        option.type = QStyleOption::SO_Frame;
        option.state = flags;
        option.lineWidth = 10;
        option.midLineWidth = 10;
        option.features = QStyleOptionFrameV2::Flat;
        style->drawPrimitive(pe, &option, qpainter, NULL);
        break;
    }
    default:
        break;
    }
//     if (pe != QStyle::PE_CustomBase) {
//         //style->drawPrimitive(pe, qpainter, r, cg, flags);
//         option.state = flags;
//         style->drawPrimitive(pe, &option, qpainter, NULL);
//     }
    qpainter->restore();
    return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeQt::GetWidgetBorder(nsIDeviceContext* aContext,
                                 nsIFrame* aFrame,
                                 PRUint8 aWidgetType,
                                 nsMargin* aResult)
{
//     qDebug("%s", __PRETTY_FUNCTION__);

    (*aResult).top = (*aResult).bottom = (*aResult).left = (*aResult).right = 0;

    switch(aWidgetType) {
    case NS_THEME_TEXTFIELD:
    case NS_THEME_LISTBOX:
        (*aResult).top = (*aResult).bottom = (*aResult).left = (*aResult).right =
                         frameWidth;
    }

    return NS_OK;
}

PRBool
nsNativeThemeQt::GetWidgetPadding(nsIDeviceContext* ,
                                  nsIFrame*, PRUint8 aWidgetType,
                                  nsMargin* aResult)
{
//     qDebug("%s", __PRETTY_FUNCTION__);

    //XXX: is this really correct?
    if (aWidgetType == NS_THEME_BUTTON_FOCUS ||
        aWidgetType == NS_THEME_TOOLBAR_BUTTON ||
        aWidgetType == NS_THEME_TOOLBAR_DUAL_BUTTON) {
        aResult->SizeTo(0, 0, 0, 0);
        return PR_TRUE;
    }

    return PR_FALSE;
}

NS_IMETHODIMP
nsNativeThemeQt::GetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                      PRUint8 aWidgetType,
                                      nsSize* aResult, PRBool* aIsOverridable)
{
//     qDebug("%s", __PRETTY_FUNCTION__);

    (*aResult).width = (*aResult).height = 0;
    *aIsOverridable = PR_TRUE;

    QStyle *s = qApp->style();

    switch (aWidgetType) {
    case NS_THEME_RADIO:
    case NS_THEME_CHECKBOX: {
        QRect rect = s->subElementRect(aWidgetType == NS_THEME_CHECKBOX
                               ? QStyle::SE_CheckBoxIndicator
                               : QStyle::SE_RadioButtonIndicator,
                               0);
        (*aResult).width = rect.width();
        (*aResult).height = rect.height();
        break;
    }
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
        (*aResult).width = s->pixelMetric(QStyle::PM_ScrollBarExtent);
        (*aResult).height = (*aResult).width;
        *aIsOverridable = PR_FALSE;
        break;
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
        (*aResult).width = s->pixelMetric(QStyle::PM_ScrollBarExtent);
        (*aResult).height = s->pixelMetric(QStyle::PM_ScrollBarSliderMin);
        *aIsOverridable = PR_FALSE;
        break;
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
        (*aResult).width = s->pixelMetric(QStyle::PM_ScrollBarSliderMin);
        (*aResult).height = s->pixelMetric(QStyle::PM_ScrollBarExtent);
        *aIsOverridable = PR_FALSE;
        break;
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
        break;
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
        break;
    case NS_THEME_DROPDOWN_BUTTON: {
//        QRect r = s->subControlRect(QStyle::CC_ComboBox, combo, QStyle::SC_ComboBoxArrow);
//        (*aResult).width = r.width() - 2*frameWidth;
//        (*aResult).height = r.height() - 2*frameWidth;
        break;
    }
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_TEXT:
    case NS_THEME_DROPDOWN_TEXTFIELD:
    case NS_THEME_TEXTFIELD:
        break;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsNativeThemeQt::WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType,
                                    nsIAtom* aAttribute, PRBool* aShouldRepaint)
{
//     qDebug("%s", __PRETTY_FUNCTION__);

    *aShouldRepaint = TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsNativeThemeQt::ThemeChanged()
{
//     qDebug("%s", __PRETTY_FUNCTION__);

    QStyle *s = qApp->style();
    if (s)
      frameWidth = s->pixelMetric(QStyle::PM_DefaultFrameWidth);
    return NS_OK;
}

PRBool
nsNativeThemeQt::ThemeSupportsWidget(nsPresContext* aPresContext,
                                     nsIFrame* aFrame,
                                     PRUint8 aWidgetType)
{
//     qDebug("%s : return FALSE", __PRETTY_FUNCTION__);
//     return FALSE;

    switch (aWidgetType) {
    case NS_THEME_SCROLLBAR:
    case NS_THEME_SCROLLBAR_BUTTON_UP:
    case NS_THEME_SCROLLBAR_BUTTON_DOWN:
    case NS_THEME_SCROLLBAR_BUTTON_LEFT:
    case NS_THEME_SCROLLBAR_BUTTON_RIGHT:
    case NS_THEME_SCROLLBAR_THUMB_HORIZONTAL:
    case NS_THEME_SCROLLBAR_THUMB_VERTICAL:
    case NS_THEME_SCROLLBAR_GRIPPER_HORIZONTAL:
    case NS_THEME_SCROLLBAR_GRIPPER_VERTICAL:
    case NS_THEME_SCROLLBAR_TRACK_HORIZONTAL:
    case NS_THEME_SCROLLBAR_TRACK_VERTICAL:
    case NS_THEME_RADIO:
    case NS_THEME_CHECKBOX:
    case NS_THEME_BUTTON_BEVEL:
    case NS_THEME_BUTTON:
    case NS_THEME_DROPDOWN:
    case NS_THEME_DROPDOWN_BUTTON:
    case NS_THEME_DROPDOWN_TEXT:
    case NS_THEME_DROPDOWN_TEXTFIELD:
    case NS_THEME_TEXTFIELD:
    case NS_THEME_LISTBOX:
//         qDebug("%s : return PR_TRUE", __PRETTY_FUNCTION__);
        return PR_TRUE;
    default:
        break;
    }
//     qDebug("%s : return PR_FALSE", __PRETTY_FUNCTION__);
    return PR_FALSE;
}

PRBool
nsNativeThemeQt::WidgetIsContainer(PRUint8 aWidgetType)
{
    // XXXdwh At some point flesh all of this out.
    if (aWidgetType == NS_THEME_DROPDOWN_BUTTON ||
        aWidgetType == NS_THEME_RADIO ||
        aWidgetType == NS_THEME_CHECKBOX) {
//         qDebug("%s : return PR_FALSE", __PRETTY_FUNCTION__);
        return PR_FALSE;
    }
//     qDebug("%s : return PR_TRUE", __PRETTY_FUNCTION__);
    return PR_TRUE;
}

PRBool
nsNativeThemeQt::ThemeDrawsFocusForWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType)
{
    if (aWidgetType == NS_THEME_DROPDOWN ||
        aWidgetType == NS_THEME_BUTTON || 
        aWidgetType == NS_THEME_TREEVIEW_HEADER_CELL) { 
//         qDebug("%s : return PR_TRUE", __PRETTY_FUNCTION__);
        return PR_TRUE;
    }
//     qDebug("%s : return PR_FALSE", __PRETTY_FUNCTION__);
    return PR_FALSE;
}

PRBool
nsNativeThemeQt::ThemeNeedsComboboxDropmarker()
{
//     qDebug("%s : return PR_FALSE", __PRETTY_FUNCTION__);
    return PR_FALSE;
}
